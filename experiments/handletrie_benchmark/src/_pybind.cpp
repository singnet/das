#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "HandleTrie.h"
#include <iostream>

using namespace attention_broker_server;
using namespace std;

namespace py = pybind11;

class Value : public HandleTrie::TrieValue
{
public:
    py::object value_;
    Value(py::object value)
    {
        this->value_ = value;
    }
    void merge(TrieValue *other)
    {
    }

    py::object get_value()
    {
        return this->value_;
    }

    std::string to_string()
    {
    }
};

PYBIND11_MODULE(handletrie_pybind, m)
{

    py::class_<HandleTrie>(m, "HandleTrie")
        .def(py::init<int>())
        .def("insert", [](HandleTrie &self, std::string key, py::object obj)
             {
            Value *v = new Value(obj);
            self.insert(key, v); })
        .def("lookup", [](HandleTrie &self, std::string key) -> py::object
             {
            Value *v = (Value*) self.lookup(key);
            if (v == NULL){
                return py::none();
            }
            return v->value_; });
}