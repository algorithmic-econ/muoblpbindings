#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "mes_utils.hpp"
#include "mes.hpp"

namespace py = pybind11;

PYBIND11_MODULE(muoblpbindings, m) {
    m.def("add", &add);
    m.def("subtract", &subtract);
    m.def("equal_shares", &equal_shares);
}
