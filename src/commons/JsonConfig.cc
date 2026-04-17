#include "JsonConfig.h"

#include <nlohmann/json.hpp>

#include "Utils.h"

using namespace std;
using nlohmann::json;

namespace commons {

namespace {

shared_ptr<Properties> json_to_properties(const json& j) {
    auto out = make_shared<Properties>();
    if (j.is_object()) {
        for (auto it = j.begin(); it != j.end(); ++it) {
            string key = it.key();
            const json& val = it.value();
            if (val.is_object()) {
                (*out)[key] = json_to_properties(val);
            } else if (val.is_array()) {
                auto arr_props = make_shared<Properties>();
                for (size_t i = 0; i < val.size(); ++i) {
                    if (val[i].is_object()) {
                        (*arr_props)[to_string(i)] = json_to_properties(val[i]);
                    } else {
                        // scalar array element: store under index key
                        if (val[i].is_string()) {
                            (*arr_props)[to_string(i)] = val[i].get<string>();
                        } else if (val[i].is_number_integer()) {
                            (*arr_props)[to_string(i)] = (long) val[i].get<long>();
                        } else if (val[i].is_number_unsigned()) {
                            (*arr_props)[to_string(i)] = (unsigned int) val[i].get<unsigned int>();
                        } else if (val[i].is_number_float()) {
                            (*arr_props)[to_string(i)] = val[i].get<double>();
                        } else if (val[i].is_boolean()) {
                            (*arr_props)[to_string(i)] = val[i].get<bool>();
                        } else if (val[i].is_null()) {
                            (*arr_props)[to_string(i)] = string("");
                        }
                    }
                }
                (*out)[key] = arr_props;
            } else if (val.is_string()) {
                (*out)[key] = val.get<string>();
            } else if (val.is_number_integer()) {
                (*out)[key] = (long) val.get<long>();
            } else if (val.is_number_unsigned()) {
                (*out)[key] = (unsigned int) val.get<unsigned int>();
            } else if (val.is_number_float()) {
                (*out)[key] = val.get<double>();
            } else if (val.is_boolean()) {
                (*out)[key] = val.get<bool>();
            } else if (val.is_null()) {
                (*out)[key] = string("");
            }
        }
        return out;
    }
    return out;
}

}  // namespace

JsonConfig::JsonConfig() : nlohmann::json() {}

JsonConfig::JsonConfig(nlohmann::json root) : nlohmann::json(std::move(root)) {}

string JsonConfig::get_schema_version() const { return value("schema_version", string("")); }

JsonPathValue JsonConfig::at_path(const string& dotted_path) const {
    vector<string> keys = Utils::split(dotted_path, '.');
    const nlohmann::json* j = this;
    for (const string& key : keys) {
        if (!j->is_object() || !j->contains(key)) {
            return JsonPathValue(nlohmann::json());
        }
        j = &(*j)[key];
    }
    return JsonPathValue(*j);
}

Properties JsonConfig::to_properties() const {
    shared_ptr<Properties> nested = json_to_properties(*this);
    return nested ? *nested : Properties();
}

}  // namespace commons
