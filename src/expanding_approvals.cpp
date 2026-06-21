#include <format>
#include <numeric>

#include "common.hpp"
#include "methods.hpp"

namespace {

using Event = std::tuple<Utility, VoterId, CandidateId>;

std::vector<Event> get_events(const Instance& instance, py::object prob) {
  std::vector<Event> res;

  py::list objectives = prob.attr("objectives");
  for (size_t i = 0; i < instance.voter_weights.size(); i++) {
    py::handle objective = objectives[i];
    for (const auto& item : objective.cast<py::dict>()) {
      py::object var = py::reinterpret_borrow<py::object>(item.first);
      std::string name = var.attr("name").cast<std::string>();
      const CandidateId candidate_id = instance.candidate_ids.at(name);
      const Utility coeff = item.second.cast<Utility>();
      if (coeff > 0) {
        res.emplace_back(coeff, i, candidate_id);
      } else if (coeff < 0) {
        throw std::invalid_argument(
            std::format("Objective {} has a negative coefficient", i));
      }
    }
  }

  std::ranges::sort(res, std::greater());
  return res;
}

std::vector<CandidateId> solve(std::vector<Event> events,
                               std::vector<double> weight, const double quota,
                               const size_t m) {
  assert(quota > 0);
  const size_t n = weight.size();

  std::vector<double> backing_money(m, 0);
  std::vector<std::vector<VoterId>> supporters_of(m);
  std::vector<std::vector<CandidateId>> supported_by(n);
  std::vector<bool> elected(m, false);
  std::vector<CandidateId> committee;

  for (const auto& [u, i, c] : events) {
    backing_money[c] += weight[i];
    supporters_of[c].emplace_back(i);
    supported_by[i].emplace_back(c);

    if (!elected[c] && backing_money[c] >= quota) {
      elected[c] = true;
      committee.emplace_back(c);

      double to_pay = quota;
      for (const VoterId& j : supporters_of[c]) {
        const double paid = std::min(to_pay, weight[j]);
        weight[j] -= paid;
        to_pay -= paid;
        for (const CandidateId& c2 : supported_by[j]) {
          backing_money[c2] -= paid;
          assert(backing_money[c2] >= 0);
        }
      }
      assert(to_pay == 0);
    }
  }

  return committee;
}

}  // namespace

std::vector<std::string> expanding_approvals(py::object prob) {
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
      solve(get_events(instance, prob), instance.voter_weights, quota,
            instance.candidate_names.size());

  return instance.map_names(committee);
}
