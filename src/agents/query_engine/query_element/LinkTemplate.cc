#include "LinkTemplate.h"

#include <algorithm>
#include <random>
#include <chrono>

#include "AtomDBSingleton.h"
#include "AttentionBrokerClient.h"
#include "Terminal.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace query_element;
using namespace atomdb;
using namespace attention_broker;

ThreadSafeHashmap<string, shared_ptr<atomdb_api_types::HandleSet>> LinkTemplate::cache;

LinkTemplate::LinkTemplate(const string& type,
                           const vector<shared_ptr<QueryElement>>& targets,
                           const string& context,
                           bool positive_importance_flag,
                           bool use_cache)
    : link_schema(type, targets.size()) {
    this->targets = targets;
    this->context = context;
    this->positive_importance_flag = positive_importance_flag;
    this->use_cache = use_cache;
    this->inner_flag = true;
    this->arity = targets.size();
    this->processor = nullptr;
    this->random_generator = new std::mt19937(std::chrono::system_clock::now().time_since_epoch().count());
}

LinkTemplate::~LinkTemplate() {
    LOG_LOCAL_DEBUG("Deleting LinkTemplate: " + std::to_string((unsigned long) this) + "...");
    if (this->processor != nullptr) {
        this->processor->stop();
    }
    delete this->random_generator;
    LOG_LOCAL_DEBUG("Deleting LinkTemplate: " + std::to_string((unsigned long) this) + "... Done");
}

void LinkTemplate::recursive_build(shared_ptr<QueryElement> element, LinkSchema& link_schema) {
    Terminal* terminal = dynamic_cast<Terminal*>(element.get());
    if (terminal != NULL) {
        // is a Terminal indeed
        if (terminal->is_variable) {
            link_schema.stack_untyped_variable(terminal->name);
        } else if (terminal->is_node) {
            link_schema.stack_node(terminal->type, terminal->name);
        } else if (terminal->is_atom) {
            link_schema.stack_atom(terminal->handle);
        } else if (terminal->is_link) {
            for (auto target : terminal->targets) {
                recursive_build(target, link_schema);
            }
            link_schema.stack_link(terminal->type, terminal->targets.size());
        } else {
            Utils::error("Invalid terminal");
        }
    } else {
        // is an inner LinkTemplate
        LinkTemplate* link_template = dynamic_cast<LinkTemplate*>(element.get());
        if (link_template == NULL) {
            Utils::error("Invalid NULL element");
        } else {
            for (auto target : link_template->targets) {
                recursive_build(target, link_schema);
            }
            link_schema.stack_link_schema(link_template->link_schema.type,
                                          link_template->targets.size());
        }
    }
}

void LinkTemplate::build() {
    if (this->inner_flag) {
        this->inner_flag = false;
        for (auto target : this->targets) {
            recursive_build(target, this->link_schema);
        }
        this->link_schema.build();
        this->id = get_handle() + string("_") + std::to_string(LinkTemplate::next_instance_count());
        this->source_element = make_shared<SourceElement>();
        this->source_element->id = this->id;
        LOG_INFO("LinkTemplate: " + to_string());
        start_thread();
    } else {
        Utils::error("LinkTemplate already built");
    }
}

void LinkTemplate::compute_importance(vector<pair<char*, float>>& handles) {
    vector<string> handle_list;
    vector<float> importance_list;
    handle_list.reserve(handles.size());
    importance_list.reserve(handles.size());
    for (auto pair : handles) {
        handle_list.push_back(string(pair.first));
    }
    AttentionBrokerClient::get_importance(handle_list, this->context, importance_list);
    for (unsigned int i = 0; i < importance_list.size(); i++) {
        handles[i].second = importance_list[i];
    }
    // Sort decreasing by importance value

    std::shuffle(handles.begin(), handles.end(), *this->random_generator);
    std::sort(handles.begin(),
              handles.end(),
              [](const std::pair<char*, float>& left, const std::pair<char*, float>& right) {
                  return left.second > right.second;
              });
}

void LinkTemplate::processor_method(shared_ptr<StoppableThread> monitor) {
    while (!this->source_element->buffers_set_up() && !monitor->stopped()) {
        Utils::sleep();
    }
    if (monitor->stopped()) {
        return;
    }
    auto db = AtomDBSingleton::get_instance();
    string link_schema_handle = this->link_schema.handle();
    shared_ptr<atomdb_api_types::HandleSet> handles;
    if (this->use_cache && LinkTemplate::fetched_links_cache().contains(link_schema_handle)) {
        LOG_INFO("Fetching " + link_schema_handle + " from cache");
        handles = LinkTemplate::fetched_links_cache().get(link_schema_handle);
    } else {
        LOG_INFO("Fetching " + link_schema_handle + " from AtomDB");
        handles = db->query_for_pattern(this->link_schema);
        LinkTemplate::fetched_links_cache().set(link_schema_handle, handles);
    }
    LOG_DEBUG("Positive importance flag: " + string(this->positive_importance_flag ? "true" : "false"));
    LOG_INFO("Fetched " + std::to_string(handles->size()) + " atoms in " + link_schema_handle);
    vector<pair<char*, float>> tagged_handles;
    if (handles->size() > 0) {
        auto iterator = handles->get_iterator();
        char* handle;
        while ((handle = iterator->next()) != nullptr) {
            tagged_handles.push_back(make_pair<char*, float>((char*) handle, 0));
        }
        compute_importance(tagged_handles);
    }
    unsigned int pending = tagged_handles.size();
    unsigned int cursor = 0;
    Assignment assignment;
    unsigned int count_matched = 0;
    while ((pending > 0) && !monitor->stopped()) {
        pair<char*, float> tagged_handle = tagged_handles[cursor++];
        if (this->positive_importance_flag && tagged_handle.second <= 0) {
            pending = 0;
        } else {
            if ((tagged_handle.second > 0 || !this->positive_importance_flag) &&
                this->link_schema.match(string(tagged_handle.first), assignment, *db.get())) {
                this->source_element->add_handle(tagged_handle.first, tagged_handle.second, assignment);
                assignment.clear();
                count_matched++;
            }
            pending--;
        }
    }
    LOG_INFO("Matched " + std::to_string(count_matched) + " atoms in " + link_schema_handle);
    Utils::sleep();
    this->source_element->query_answers_finished();
    LOG_DEBUG("LinkTemplate " + link_schema_handle + " finished processing. It's going to sleep.");
    while (!monitor->stopped()) {
        // Keep stack variables (e.g. the handles which are passed as QueryAnswers
        // to source_element)
        Utils::sleep();
    }
    LOG_DEBUG("LinkTemplate " + link_schema_handle +
              " woke up and is returning from processing thread.");
}

void LinkTemplate::start_thread() {
    if (this->inner_flag) {
        Utils::error("Can't start thread in inner LinkTemplates");
    } else {
        this->processor = make_shared<StoppableThread>("processor_thread(" + this->id + ")");
        this->processor->attach(new thread(&LinkTemplate::processor_method, this, this->processor));
    }
}

shared_ptr<Source> LinkTemplate::get_source_element() {
    if (this->inner_flag) {
        Utils::error("Inner LinkTemplates doesn't have source_element");
        return nullptr;
    } else {
        return source_element;
    }
}

string LinkTemplate::get_handle() {
    if (this->inner_flag) {
        Utils::error("Inner LinkTemplates doesn't have handle");
        return "";
    } else {
        return this->link_schema.handle();
    }
}

const string& LinkTemplate::get_type() { return this->link_schema.type; }

string LinkTemplate::to_string() {
    if (this->inner_flag) {
        return "<Inner LinkTemplate>";
    } else {
        return this->link_schema.to_string();
    }
}
