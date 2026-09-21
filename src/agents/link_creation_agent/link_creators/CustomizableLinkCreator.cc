#include "CustomizableLinkCreator.h"

#include "AtomDBUtils.h"
#include "Hasher.h"
#include "ServiceBusSingleton.h"
#include "tags.h"

using namespace link_creators;
using namespace atomdb;

char CustomizableLinkCreator::EXTRA_PARAMETERS_SPLIT_CHAR = ',';

// -------------------------------------------------------------------------------------------------
// Public methods

CustomizableLinkCreator::LinkSpecification::LinkSpecification(
    const vector<QueryAnswerElement>& target_elements,
    const vector<QueryAnswerElement>& strength_elements,
    string link_type,
    StrengthComposition strength_composition,
    const vector<string> queries) {
    this->target_elements = target_elements;
    this->strength_elements = strength_elements;
    this->link_type = link_type;
    this->strength_composition = strength_composition;
    this->queries = queries;
    check();
}

void CustomizableLinkCreator::LinkSpecification::check() {
    if (this->target_elements.size() == 0) {
        RAISE_ERROR("Invalid empty target elements");
    }
    if (this->strength_composition > PRODUCT) {
        if ((this->target_elements.size() != 2) || (this->strength_elements.size() != 2) ||
            (this->queries.size() != 2)) {
            RAISE_ERROR(
                "Strength composition = " + std::to_string((unsigned int) this->strength_composition) +
                " requires exactly 2 target elements, 2 strength elements and 2 queries");
        }
    }
}

CustomizableLinkCreator::CustomizableLinkCreator() {}

CustomizableLinkCreator::~CustomizableLinkCreator() {}

LinkCreationStats CustomizableLinkCreator::create(shared_ptr<QueryAnswer> query_answer) {
    STACK_TRACE();
    LinkCreationStats stats;
    for (LinkSpecification& spec : this->link_specification) {
        if ((spec.target_elements.size() == 0) || (spec.link_type == "")) {
            RAISE_ERROR("Invalid empty target elements or link_type");
            break;
        }
        vector<string> handles;
        handles.push_back(Hasher::node_handle(SYMBOL, spec.link_type));
        for (QueryAnswerElement& element : spec.target_elements) {
            handles.push_back(query_answer->get(element));
        }
        string key = Utils::join(handles, ' ');
        if (!visited(key)) {
            visit(key);
            stats.visited = true;
            AddLinkStatus add_status = add_or_update_link(handles, compute_strength(query_answer, spec));
            if (add_status == CREATED) {
                stats.created++;
            } else if (add_status == UPDATED) {
                stats.updated++;
            }
        }
    }
    return stats;
}

void CustomizableLinkCreator::extra_parameters(const string& extra_parameters) {
    STACK_TRACE();
    if (extra_parameters != "") {
        vector<string> tokens = Utils::split(extra_parameters, EXTRA_PARAMETERS_SPLIT_CHAR);
        untokenize(tokens);
    }
}

string CustomizableLinkCreator::extra_parameters() {
    vector<string> tokens;
    tokenize(tokens);
    return Utils::join(tokens, EXTRA_PARAMETERS_SPLIT_CHAR);
}

void CustomizableLinkCreator::add_link_specification(const vector<QueryAnswerElement>& target_elements,
                                                     const vector<QueryAnswerElement>& strength_elements,
                                                     const string& link_type,
                                                     StrengthComposition strength_composition,
                                                     const vector<string>& queries) {
    STACK_TRACE();
    string trimmed_type = Utils::trim(link_type);
    if ((trimmed_type == "") || (trimmed_type.find(' ') != std::string::npos)) {
        RAISE_ERROR("Invalid link_type: " + link_type);
    }

    link_specification.emplace_back(
        target_elements, strength_elements, trimmed_type, strength_composition, queries);
}

void CustomizableLinkCreator::tokenize(vector<string>& tokens) {
    STACK_TRACE();
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
        tokens.push_back(spec.link_type);
        tokens.push_back(std::to_string(spec.strength_composition));
        tokens.push_back(std::to_string(spec.queries.size()));
        for (string& query : spec.queries) {
            tokens.push_back(query);
        }
    }
}

static inline string& safe_get_next_token(vector<string>& tokens, unsigned int& cursor) {
    STACK_TRACE();
    if (cursor >= tokens.size()) {
        RAISE_ERROR("Invalid tokens for CustomizableLinkCreator");
    }
    return tokens[cursor++];
}

void CustomizableLinkCreator::untokenize(vector<string>& tokens) {
    STACK_TRACE();
    unsigned int cursor = 0;
    unsigned int num_specs = Utils::string_to_uint(safe_get_next_token(tokens, cursor));
    for (unsigned int i = 0; i < num_specs; i++) {
        vector<QueryAnswerElement> _target_elements;
        vector<QueryAnswerElement> _strength_elements;
        string _link_type;
        StrengthComposition _strength_composition;
        vector<string> _queries;
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
        _link_type = safe_get_next_token(tokens, cursor);
        _strength_composition =
            (StrengthComposition) Utils::string_to_uint(safe_get_next_token(tokens, cursor));
        num_elements = Utils::string_to_uint(safe_get_next_token(tokens, cursor));
        for (unsigned int j = 0; j < num_elements; j++) {
            _queries.push_back(safe_get_next_token(tokens, cursor));
        }
        add_link_specification(
            _target_elements, _strength_elements, _link_type, _strength_composition, _queries);
    }
    if (cursor != tokens.size()) {
        RAISE_ERROR("Invalid trailing tokens for CustomizableLinkCreator");
    }
}

// -------------------------------------------------------------------------------------------------
// Private methods

shared_ptr<PatternMatchingQueryProxy> CustomizableLinkCreator::issue_link_count_query(
    const string& query_str) {
    vector<string> query_tokens = Utils::split(query_str);
    auto proxy = make_shared<PatternMatchingQueryProxy>(query_tokens, context());
    proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = true;
    proxy->parameters[BaseQueryProxy::ATTENTION_CORRELATION] = (unsigned int) BaseQueryProxy::NONE;
    proxy->parameters[BaseQueryProxy::ATTENTION_UPDATE] = (unsigned int) BaseQueryProxy::NONE;
    proxy->parameters[PatternMatchingQueryProxy::DISREGARD_IMPORTANCE_FLAG] = true;
    proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = false;
    proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = false;
    proxy->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = false;

    ServiceBusSingleton::get_instance()->issue_bus_command(proxy);
    return proxy;
}

void CustomizableLinkCreator::insert_or_update(map<string, double>& count_map,
                                               const string& key,
                                               double value) {
    STACK_TRACE();
    auto iterator = count_map.find(key);
    if (iterator == count_map.end()) {
        count_map[key] = value;
    } else {
        if (value > iterator->second) {
            count_map[key] = value;
        }
    }
}

void CustomizableLinkCreator::compute_counts(shared_ptr<QueryAnswer> base_query_answer,
                                             LinkSpecification& spec,
                                             double& count_A,
                                             double& count_B,
                                             double& count_intersection,
                                             double& count_union) {
    STACK_TRACE();

    LOG_DEBUG("Computing counts for: " +
              AtomDBUtils::handle_to_metta(base_query_answer->get(spec.target_elements[0])) + " and " +
              AtomDBUtils::handle_to_metta(base_query_answer->get(spec.target_elements[1])));
    LOG_DEBUG("Query answer: " + base_query_answer->to_string());
    shared_ptr<PatternMatchingQueryProxy> proxy[2];
    for (unsigned int i = 0; i < 2; i++) {
        string query = spec.queries[i];
        string pattern = "QueryAnswerElement(" + spec.target_elements[i].to_string() + ")";
        LOG_DEBUG("Query element pattern: " + pattern);
        LOG_DEBUG("Query template: <" + query + ">");
        Utils::replace_all(query, pattern, base_query_answer->get(spec.target_elements[i]));
        LOG_DEBUG("  Query string: <" + query + ">");
        proxy[i] = issue_link_count_query(query);
    }

    count_A = 0.0;
    count_B = 0.0;
    count_intersection = 0.0;
    count_union = 0.0;
    map<string, double> count_map[2];
    map<string, double> count_map_union;
    map<string, double> count_map_intersection;
    double d;
    string handle;

    shared_ptr<QueryAnswer> query_answer;
    for (unsigned int i = 0; i < 2; i++) {
        while (!proxy[i]->finished()) {
            if ((query_answer = proxy[i]->pop()) == NULL) {
                Utils::sleep();
            } else {
                d = 1;
                for (string& h : query_answer->get_handles_vector()) {
                    d *= get_strength(h);
                }
                handle = query_answer->get(spec.strength_elements[i]);
                insert_or_update(count_map[i], handle, d);
            }
        }
    }
    count_map_union = count_map[0];
    for (auto pair : count_map[1]) {
        auto iterator = count_map[0].find(pair.first);
        if (iterator == count_map[0].end()) {
            insert_or_update(count_map_union, pair.first, pair.second);
        } else {
            insert_or_update(count_map_union, pair.first, pair.second);
            insert_or_update(count_map_intersection, pair.first, min(pair.second, iterator->second));
        }
    }

    for (auto pair : count_map_intersection) {
        count_intersection += pair.second;
    }
    for (auto pair : count_map_union) {
        count_union += pair.second;
    }
    for (auto pair : count_map[0]) {
        count_A += pair.second;
    }
    for (auto pair : count_map[1]) {
        count_B += pair.second;
    }
    LOG_DEBUG("Counts: " + to_string(count_A) + " " + to_string(count_B) + " " +
              to_string(count_intersection) + " " + to_string(count_union));
}

double CustomizableLinkCreator::compute_strength(shared_ptr<QueryAnswer> query_answer,
                                                 LinkSpecification& spec) {
    STACK_TRACE();
    double answer = 0.0;
    double count_A = 0.0;
    double count_B = 0.0;
    double count_intersection = 0.0;
    double count_union = 0.0;

    if ((spec.strength_composition == INTERSECTION_OVER_UNION) ||
        (spec.strength_composition == INTERSECTION_OVER_A) ||
        (spec.strength_composition == INTERSECTION_OVER_B)) {
        compute_counts(query_answer, spec, count_A, count_B, count_intersection, count_union);
    }

    switch (spec.strength_composition) {
        case PRODUCT:
            answer = 1.0;
            for (QueryAnswerElement& element : spec.strength_elements) {
                answer *= get_strength(query_answer->get(element));
            }
            break;
        case INTERSECTION_OVER_UNION:
            if (!Utils::is_zero(count_union)) {
                answer = count_intersection / count_union;
            }
            break;
        case INTERSECTION_OVER_A:
            if (!Utils::is_zero(count_A)) {
                answer = count_intersection / count_A;
            }
            break;
        case INTERSECTION_OVER_B:
            if (!Utils::is_zero(count_B)) {
                answer = count_intersection / count_B;
            }
            break;
        default:
            RAISE_ERROR("Invalid strength composition: " + std::to_string(spec.strength_composition));
            break;
    }
    return answer;
}
