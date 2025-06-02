#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "mes_utils.hpp"
#include "mes.hpp"

namespace py = pybind11;

PYBIND11_MODULE(muoblpbindings, m) {
    m.def("equal_shares", &equal_shares);
    m.def("equal_shares_utils", &equal_shares_utils);
}
