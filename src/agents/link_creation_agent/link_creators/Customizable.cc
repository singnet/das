#include "Customizable.h"
#include "tags.h"

using namespace link_creators;

// -------------------------------------------------------------------------------------------------
// Public methods

Customizable::Customizable() {
}

Customizable::~Customizable() {
}

LinkCreationStats Customizable::create(shared_ptr<QueryAnswer> query_answer) {
    STACK_TRACE();
    LinkCreationStats stats;
    for (LinkSpecification& spec : this->link_specification) {
        vector<string> handles;
        vector<double> strength_components
            handles.push_back(Hasher::type_handle(spec.link_type));
        for (QueryAnswerElement& element : spec.target_elements) {
            handles.push_back(query_answer->get(element));
        }
        for (QueryAnswerElement& element : spec.strength_elements) {
            strength_components.push_back(get_strength(query_answer->get(element)));
        }
        add_or_update_link(handles, compute_strength(strength_components, spec.strength_composition));
    }
    return stats;
}

// -------------------------------------------------------------------------------------------------
// Private methods

bool Customizable::

}
