#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/trampoline.h>

#include <type_traits>

#include "query_engine/QueryAnswer.h"

namespace nb = nanobind;
using namespace nb::literals;

using namespace std;
using namespace query_engine;

NB_MODULE(hyperon_das_query_engine_ext, m) {

    // QueryAnswer.h
    nb::class_<QueryAnswer>(m, "QueryAnswer")
        .def(nb::init<>())
        .def(nb::init<double>(), "importance"_a)
        .def(
            "__init__",
            [](QueryAnswer& self, const string& handle, double importance) {
                new (&self) QueryAnswer(handle.c_str(), importance);
            },
            "handle"_a,
            "importance"_a)
        .def_prop_ro("handles",
                     [](const QueryAnswer& self) -> const vector<string> {
                         vector<string> handles;
                         for (size_t i = 0; i < self.handles_size; i++) {
                             handles.emplace_back(self.handles[i]);
                         }
                         return handles;
                     })
        .def_ro("handles_size", &QueryAnswer::handles_size)
        .def_ro("importance", &QueryAnswer::importance)
        .def(
            "add_handle",
            [](QueryAnswer& self, const string& handle) { self.add_handle(handle.c_str()); },
            "handle"_a)
        .def(
            "merge",
            [](QueryAnswer& self, const QueryAnswer& other, bool merge_handles) -> bool {
                return self.merge((QueryAnswer*) &other, merge_handles);
            },
            "other"_a,
            "merge_handles"_a)
        .def_static(
            "copy",
            [](const QueryAnswer& other) -> shared_ptr<QueryAnswer> {
                return shared_ptr<QueryAnswer>(QueryAnswer::copy((QueryAnswer*) &other));
            },
            "other"_a);
}
