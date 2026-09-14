#include "CustomizableLinkCreator.h"

#include "Hasher.h"
#include "tags.h"

using namespace link_creators;

// -------------------------------------------------------------------------------------------------
// Public methods

CustomizableLinkCreator::CustomizableLinkCreator() {}

CustomizableLinkCreator::~CustomizableLinkCreator() {}

LinkCreationStats CustomizableLinkCreator::create(shared_ptr<QueryAnswer> query_answer) {
    STACK_TRACE();
    LinkCreationStats stats;
    for (LinkSpecification& spec : this->link_specification) {
        if ((spec.target_elements.size() == 0) || (spec.link_type == "")) {
            RAISE_ERROR("Invalid empty target elements or link_type");
            return stats;
        }
        vector<string> handles;
        vector<double> strength_components;
        handles.push_back(Hasher::node_handle(SYMBOL, spec.link_type));
        for (QueryAnswerElement& element : spec.target_elements) {
            handles.push_back(query_answer->get(element));
        }
        string key = Utils::join(handles, ' ');
        if (!visited(key)) {
            visit(key);
            stats.visited = true;
            for (QueryAnswerElement& element : spec.strength_elements) {
                strength_components.push_back(get_strength(query_answer->get(element)));
            }
            if (add_or_update_link(handles,
                                   compute_strength(strength_components, spec.strength_composition))) {
                stats.created++;
            } else {
                stats.updated++;
            }
        }
    }
    return stats;
}

void CustomizableLinkCreator::extra_parameters(const string& extra_parameters) {
    if (extra_parameters != "") {
        vector<string> tokens = Utils::split(extra_parameters);
        untokenize(tokens);
    }
}

void CustomizableLinkCreator::add_link_specification(const vector<QueryAnswerElement>& target_elements,
                                                     const vector<QueryAnswerElement>& strength_elements,
                                                     StrengthComposition strength_composition,
                                                     const string& link_type) {
    string trimmed_type = Utils::trim(link_type);
    if ((trimmed_type == "") || (trimmed_type.find(' ') != std::string::npos)) {
        RAISE_ERROR("Invalid link_type: " + link_type);
    }

    link_specification.emplace_back(
        target_elements, strength_elements, strength_composition, trimmed_type);
}

void CustomizableLinkCreator::tokenize(vector<string>& tokens) {
    tokens.push_back(std::to_string(this->link_specification.size()));
    for (LinkSpecification& spec : this->link_specification) {
        tokens.push_back(std::to_string(spec.target_elements.size()));
        for (QueryAnswerElement& element : spec.target_elements) {
            tokens.push_back(element.to_string());
        }
        tokens.push_back(std::to_string(spec.strength_elements.size()));
        for (QueryAnswerElement& element : spec.strength_elements) {
            tokens.push_back(element.to_string());
        }
        tokens.push_back(std::to_string(spec.strength_composition));
        tokens.push_back(spec.link_type);
    }
}

static inline string& safe_get_next_token(vector<string>& tokens, unsigned int& cursor) {
    if (cursor >= tokens.size()) {
        RAISE_ERROR("Invalid tokens for CustomizableLinkCreator");
    }
    return tokens[cursor++];
}

void CustomizableLinkCreator::untokenize(vector<string>& tokens) {
    unsigned int cursor = 0;
    unsigned int num_specs = Utils::string_to_uint(safe_get_next_token(tokens, cursor));
    for (unsigned int i = 0; i < num_specs; i++) {
        vector<QueryAnswerElement> _target_elements;
        vector<QueryAnswerElement> _strength_elements;
        StrengthComposition _strength_composition;
        string _link_type;
        unsigned int num_elements = Utils::string_to_uint(safe_get_next_token(tokens, cursor));
        for (unsigned int j = 0; j < num_elements; j++) {
            _target_elements.push_back(
                QueryAnswerElement::from_string(safe_get_next_token(tokens, cursor)));
        }
        num_elements = Utils::string_to_uint(safe_get_next_token(tokens, cursor));
        for (unsigned int j = 0; j < num_elements; j++) {
            _strength_elements.push_back(
                QueryAnswerElement::from_string(safe_get_next_token(tokens, cursor)));
        }
        _strength_composition =
            (StrengthComposition) Utils::string_to_uint(safe_get_next_token(tokens, cursor));
        _link_type = safe_get_next_token(tokens, cursor);
        add_link_specification(_target_elements, _strength_elements, _strength_composition, _link_type);
    }
    if (cursor != tokens.size()) {
        RAISE_ERROR("Invalid trailing tokens for CustomizableLinkCreator");
    }
}

// -------------------------------------------------------------------------------------------------
// Private methods

double CustomizableLinkCreator::compute_strength(const vector<double>& components,
                                                 StrengthComposition composition) {
    double answer = 0.0;
    switch (composition) {
        case PRODUCT:
            answer = 1.0;
            for (double strength : components) {
                answer *= strength;
            }
            break;
        default:
            RAISE_ERROR("Invalid strength composition: " + std::to_string(composition));
            break;
    }
    return answer;
}
