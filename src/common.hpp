#pragma once

#include <pybind11/pybind11.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/dynamic_bitset.hpp>

namespace py = pybind11;

using VoterId = uint32_t;
using CandidateId = uint32_t;
using Utility = double;
using Cost = double;

struct Instance {
  std::vector<std::string> candidate_names;
  std::unordered_map<std::string, CandidateId> candidate_ids;
  Cost budget;
  std::vector<Cost> candidate_costs;
  std::vector<double> voter_weights;

  inline std::vector<std::string> map_names(
      const std::vector<CandidateId>& ids) {
    std::vector<std::string> res(ids.size());
    for (size_t i = 0; i < ids.size(); i++) {
      res[i] = this->candidate_names[ids[i]];
    }
    return res;
  }

  inline std::vector<std::string> map_names(const boost::dynamic_bitset<>& bitset) {
    std::vector<std::string> res;
    for (CandidateId i = 0; i < candidate_names.size(); i++) {
      if (bitset.at(i)) {
        res.emplace_back(candidate_names[i]);
      }
    }
    return res;
  }

  static Instance from_MuoblpProblem(py::object prob);
};

std::vector<std::vector<CandidateId>> get_rankings(const Instance& instance,
                                                   py::object prob);
