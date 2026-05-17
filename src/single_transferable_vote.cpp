#include "single_transferable_vote.hpp"

#include <pybind11/pytypes.h>
#include <pybind11/stl.h>

#include <algorithm>
#include <format>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <unordered_map>

namespace {

using VoterId = uint32_t;
using CandidateId = uint32_t;
using Utility = double;
using Cost = double;

std::vector<std::vector<CandidateId>> get_rankings(
    const std::vector<std::vector<Utility>>& utility, const size_t m) {
  if (utility.empty()) {
    return {};
  }
  std::vector<std::vector<CandidateId>> res(utility.size(),
                                            std::vector<CandidateId>(m));
  for (size_t i = 0; i < utility.size(); i++) {
    std::vector<CandidateId>& ranking = res[i];
    std::iota(ranking.begin(), ranking.end(), 0);
    std::ranges::sort(ranking, [&](CandidateId a, CandidateId b) {
      return utility[i][a] > utility[i][b];
    });
    for (CandidateId a = 0; a + 1 < ranking.size(); a++) {
      if (utility[i][ranking[a]] <= utility[i][ranking[a + 1]]) {
        throw std::invalid_argument(
            std::format("Objective {} does not create a strict ranking", i));
      }
    }
  }
  return res;
}

struct Instance {
  std::vector<std::string> candidate_names;
  std::unordered_map<std::string, CandidateId> candidate_ids;
  Cost budget;
  std::vector<Cost> candidate_costs;
  std::vector<std::vector<Utility>> voters;
  std::vector<double> voter_weights;

  static Instance from_MuoblpProblem(py::object prob) {
    Instance instance;

    // Create names->ids mapping
    py::list vars = prob.attr("variables")();
    const size_t m = vars.size();
    if (m > std::numeric_limits<CandidateId>::max()) {
      throw std::invalid_argument(
          std::format("Too many candidates. Got {} while the maximum is {}", m,
                      std::numeric_limits<CandidateId>::max()));
    }
    for (py::handle var : vars) {
      std::string name = var.attr("name").cast<std::string>();
      instance.candidate_names.emplace_back(name);
      auto cat = var.attr("cat").cast<std::string>();
      if (cat != "Integer") {
        py::print(
            std::format("Warning: variable {} is defined as {}, but "
                        "treated as integer instead",
                        name, cat));
      }
    }

    // Construct the reverse ids->names mapping
    for (size_t i = 0; i < m; i++) {
      instance.candidate_ids[instance.candidate_names[i]] = i;
    }

    // Extract budget
    py::dict constraints = prob.attr("constraints");
    if (constraints.size() != 1) {
      throw std::invalid_argument(std::format(
          "Provided {} constraints, expected 1", constraints.size()));
    }
    py::object constraint =
        py::reinterpret_borrow<py::object>(constraints.begin()->second);
    instance.budget = -constraint.attr("constant").cast<Cost>();

    // Extract candidate consts
    instance.candidate_costs.resize(m, 0);
    for (const auto& item : constraint.cast<py::dict>()) {
      py::object var = py::reinterpret_borrow<py::object>(item.first);
      std::string name = var.attr("name").cast<std::string>();
      Cost cost = item.second.cast<Cost>();
      instance.candidate_costs[instance.candidate_ids[name]] = cost;
    }

    // Extract voters
    py::list objectives = prob.attr("objectives");
    py::dict objectives_weights = prob.attr("objectives_weights");
    const size_t n = objectives.size();
    if (n > std::numeric_limits<VoterId>::max()) {
      throw std::invalid_argument(
          std::format("Too many voters. Got {} while the maximum is {}", n,
                      std::numeric_limits<VoterId>::max()));
    }
    instance.voters.resize(n, std::vector<Utility>(m, 0));
    instance.voter_weights.resize(n, 1);
    for (size_t i = 0; i < n; i++) {
      py::handle objective = objectives[i];
      py::object voter_name = objective.attr("name");
      py::object weight = objectives_weights[voter_name];
      instance.voter_weights[i] = weight.cast<double>();
      for (const auto& item : objective.cast<py::dict>()) {
        py::object var = py::reinterpret_borrow<py::object>(item.first);
        std::string name = var.attr("name").cast<std::string>();
        const CandidateId candidate_id = instance.candidate_ids.at(name);
        instance.voters[i][candidate_id] = item.second.cast<Utility>();
      }
    }

    return instance;
  };
};

std::vector<CandidateId> solve(
    const std::vector<std::vector<CandidateId>>& rankings,
    const std::vector<double>& weights, const double quota, const size_t m) {
  const size_t n = rankings.size();
  assert(quota > 0);
  assert(rankings.size() == weights.size());

  std::vector<double> money_left(n, 0);
  std::vector<std::vector<CandidateId>> reversed_rankings(
      n, std::vector<CandidateId>{});
  std::vector<double> backing_money(m, 0);
  std::vector<std::vector<VoterId>> supporters(m, std::vector<VoterId>{});
  std::vector<bool> alive(m, true);
  uint32_t alive_count = m;
  std::vector<CandidateId> passing_quota;
  std::vector<CandidateId> committee;

  const auto approve_next = [&](VoterId i) {
    while (!reversed_rankings[i].empty() &&
           !alive.at(reversed_rankings[i].back())) {
      reversed_rankings[i].pop_back();
    }
    if (reversed_rankings[i].empty()) {
      return;
    }
    CandidateId c = reversed_rankings[i].back();
    if (backing_money[c] < quota && backing_money[c] + money_left[i] >= quota) {
      passing_quota.emplace_back(c);
    }
    backing_money[c] += money_left[i];
    supporters[c].emplace_back(i);
  };

  for (VoterId i = 0; i < n; i++) {
    money_left[i] = weights[i];
    reversed_rankings[i] =
        std::vector<CandidateId>(rankings[i].rbegin(), rankings[i].rend());
    approve_next(i);
  }

  while (alive_count > 0) {
    if (passing_quota.empty()) {
      CandidateId c =
          std::ranges::min_element(backing_money) - backing_money.begin();
      assert(alive.at(c));
      alive[c] = false;
      alive_count--;
      backing_money[c] = std::numeric_limits<double>::infinity();
      for (const VoterId& i : supporters[c]) {
        assert(!reversed_rankings[i].empty());
        approve_next(i);
      }
    } else {
      const CandidateId c = passing_quota.back();
      passing_quota.pop_back();
      assert(alive.at(c));

      committee.emplace_back(c);

      alive[c] = false;
      alive_count--;
      backing_money[c] = std::numeric_limits<double>::infinity();

      // Pay for the candidate
      double to_pay = quota;
      while (to_pay > 0) {
        assert(!supporters[c].empty());
        const VoterId i = supporters[c].back();
        if (to_pay < money_left[i]) {
          money_left[i] -= to_pay;
          to_pay = 0;
        } else {
          to_pay -= money_left[i];
          money_left[i] = 0;
          supporters[c].pop_back();
        }
      }

      // Pass the rest of supporters
      for (const VoterId& i : supporters[c]) {
        assert(!reversed_rankings[i].empty());
        approve_next(i);
      }

      supporters[c].clear();
    }
  }

  return committee;
}

}  // namespace

std::vector<std::string> single_transferable_vote(py::object prob) {
  Instance instance = Instance::from_MuoblpProblem(prob);

  for (size_t i = 0; i < instance.candidate_costs.size(); i++) {
    if (instance.candidate_costs[i] != 1) {
      throw std::invalid_argument(std::format(
          "All candidates are expected to have weight 1, but candidate {} has "
          "{}",
          instance.candidate_names[i], instance.candidate_costs[i]));
    }
  }

  double quota = std::accumulate(instance.voter_weights.begin(),
                                 instance.voter_weights.end(), 0.) /
                 instance.budget;

  std::vector<CandidateId> committee =
      solve(get_rankings(instance.voters, instance.candidate_names.size()),
            instance.voter_weights, quota, instance.candidate_names.size());

  std::vector<std::string> res(committee.size());
  for (size_t i = 0; i < committee.size(); i++) {
    res[i] = instance.candidate_names[committee[i]];
  }

  return res;
}
