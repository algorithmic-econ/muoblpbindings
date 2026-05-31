#pragma once

#include <pybind11/pybind11.h>

#include <string>
#include <vector>

namespace py = pybind11;

std::vector<std::string> single_transferable_vote(py::object lp_problem);
