#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include "HandleTrie.h"
#include <iostream>

using namespace attention_broker_server;
using namespace std;

namespace nb = nanobind;

class Value : public HandleTrie::TrieValue
{
public:
    nb::object value_obj;
    Value(nb::object value)
    {
        this->value_obj = value;
    }
    void merge(TrieValue *other)
    {
    }

    std::string to_string()
    {
    }
};

NB_MODULE(handletrie_nanobind, m)
{

    nb::class_<HandleTrie>(m, "HandleTrie")
        .def(nb::init<int>())
        .def("insert", [](HandleTrie &self, std::string key, nb::object obj)
             {
            Value *v = new Value(obj);
            self.insert(key, v); })
        .def("lookup", [](HandleTrie &self, std::string key) -> nb::object
             {
            Value *v = (Value*) self.lookup(key);
            if (v == NULL){
                return nb::none();
            }
            return v->value_obj; });
}