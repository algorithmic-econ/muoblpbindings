#include <pybind11/pybind11.h>

#include "mes_utils.hpp"

namespace py = pybind11;

PYBIND11_MODULE(muoblpbindings, m) {
    m.def("add", &add);
    m.def("subtract", &subtract);
}