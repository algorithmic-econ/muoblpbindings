#include <format>

#include <boost/dynamic_bitset.hpp>

#include "common.hpp"
#include "methods.hpp"

namespace {

using bitset = boost::dynamic_bitset<>;
using SolidCoalition = std::pair<double, bitset>;

bool on_top(const std::vector<CandidateId>& ranking, const bitset& vars) {
  const size_t r = vars.count();
  if (ranking.size() < r) {
    return false;
  }
  for (size_t i = 0; i < r; i++) {
    if (!vars.at(ranking[i])) {
      return false;
    }
  }
  return true;
}

std::vector<SolidCoalition> get_all_coalitions(
    const std::vector<std::vector<CandidateId>>& rankings,
    const std::vector<double>& weights, const size_t m) {
  assert(rankings.size() == weights.size());
  std::vector<SolidCoalition> res;
  for (VoterId i = 0; i < rankings.size(); i++) {
    bitset candidates(m);
    for (size_t r = 0; r < rankings[i].size(); r++) {
      candidates.set(rankings[i][r]);
      double weight = 0;
      for (VoterId j = 0; j < rankings.size(); j++) {
        if (on_top(rankings[j], candidates)) {
          if (j < i) {
            // This coalition has already been considered
            break;
          } else {
            weight += weights[j];
          }
        }
      }
      if (weight > 0) {
        res.emplace_back(weight, candidates);
      }
    }
  }
  return res;
}

bitset solve(const std::vector<std::vector<CandidateId>>& rankings,
             const std::vector<double>& weights, const size_t committee_size,
             const size_t m) {
  const size_t n = rankings.size();
  assert(rankings.size() == weights.size());

  std::vector<SolidCoalition> coalitions =
      get_all_coalitions(rankings, weights, m);
  bitset committee(m);

  bitset all_candidates(m);
  all_candidates.set();

  for (size_t k = 0; k < committee_size; k++) {
    bitset const* curr = &all_candidates;
    while ((*curr & (~committee)).count() > 1) {
      double best_value = 0;
      bitset const* best = nullptr;
      for (const auto& [weight, candidates] : coalitions) {
        if (!candidates.is_proper_subset_of(*curr) ||
            candidates.is_subset_of(committee)) {
          continue;
        }
        double value = weight / ((candidates & committee).count() + 1);
        if (value > best_value) {
          best_value = value;
          best = &candidates;
        }
      }
      assert(best_value > 0);
      curr = best;
    }

    committee |= *curr;
  }

  return committee;
}

}  // namespace

std::vector<std::string> solid_coalition_refinement(py::object prob) {
  Instance instance = Instance::from_MuoblpProblem(prob);

  for (size_t i = 0; i < instance.candidate_costs.size(); i++) {
    if (instance.candidate_costs[i] != 1) {
      throw std::invalid_argument(std::format(
          "All candidates are expected to have weight 1, but candidate {} has "
          "{}",
          instance.candidate_names[i], instance.candidate_costs[i]));
    }
  }

  const size_t m = instance.candidate_names.size();
  bitset committee = solve(get_rankings(instance, prob), instance.voter_weights,
                           instance.budget, m);

  return instance.map_names(committee);
}
