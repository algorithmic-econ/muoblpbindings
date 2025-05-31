#include <pybind11/pybind11.h>

#include "mes_utils.hpp"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

int add(int i, int j) {
    return i + j;
}

namespace py = pybind11;

PYBIND11_MODULE(muoblpbindings, m) {
    m.def("add", &add);
    m.def("subtract", &subtract);
}