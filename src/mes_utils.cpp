#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <string>
#include <optional>

using namespace std;

using VoterId = string;
using CandidateId = string;
using Utility = double;
using CostMap = unordered_map<CandidateId, double>;
using BudgetMap = unordered_map<VoterId, double>;
using ApproversMap = unordered_map<CandidateId, vector<pair<VoterId, Utility>>>;


vector<CandidateId> break_ties(
    const vector<VoterId>& voters,
    const vector<CandidateId>& projects,
    const CostMap& cost,
    const ApproversMap& approvers_utilities,
    const vector<CandidateId>& choices) {

    unordered_map<CandidateId, double> total_utility;
    for (const auto& c : projects) {
        total_utility[c] = accumulate(approvers_utilities.at(c).begin(), approvers_utilities.at(c).end(), 0.0,
                                           [](double sum, const auto& p) { return sum + p.second; });
    }

    vector<CandidateId> remaining = choices;

    auto min_it = min_element(remaining.begin(), remaining.end(),
        [&](const auto& a, const auto& b) {
            return cost.at(a) < cost.at(b);
        });
    double best_cost = cost.at(*min_it);

    remaining.erase(remove_if(remaining.begin(), remaining.end(),
                                   [&](const auto& c) { return cost.at(c) != best_cost; }),
                    remaining.end());

    auto max_it = max_element(remaining.begin(), remaining.end(),
        [&](const auto& a, const auto& b) {
            return total_utility[a] < total_utility[b];
        });
    double best_count = total_utility[*max_it];
    remaining.erase(remove_if(remaining.begin(), remaining.end(),
                                   [&](const auto& c) { return total_utility[c] != best_count; }),
                    remaining.end());

    return remaining;
}

vector<CandidateId> equal_shares_utils(
    const vector<VoterId>& voters,
    const vector<CandidateId>& projects,
    const CostMap& cost,
    ApproversMap approvers_utilities,
    double total_budget) {

    BudgetMap budget;
    for (const auto& v : voters) {
        budget[v] = total_budget / voters.size();
    }

    unordered_map<CandidateId, double> remaining;
    for (const auto& c : projects) {
        if (cost.at(c) > 0 && !approvers_utilities[c].empty()) {
            remaining[c] = accumulate(approvers_utilities[c].begin(), approvers_utilities[c].end(), 0.0,
                                           [](double sum, const auto& p) { return sum + p.second; });
        }
    }

    vector<CandidateId> winners;

    while (!remaining.empty()) {
        vector<CandidateId> best;
        double best_eff_vote_count = 0.0;

        vector<CandidateId> remaining_sorted(remaining.size());
        transform(remaining.begin(), remaining.end(), remaining_sorted.begin(),
                       [](const auto& p) { return p.first; });
        sort(remaining_sorted.begin(), remaining_sorted.end(),
                  [&](const auto& a, const auto& b) { return remaining[a] > remaining[b]; });

        for (const auto& c : remaining_sorted) {
            double prev_eff_vote_count = remaining[c];
            if (prev_eff_vote_count < best_eff_vote_count) break;

            double money_behind_now = 0.0;
            for (const auto& [voter, _] : approvers_utilities[c]) {
                money_behind_now += budget[voter];
            }
            if (money_behind_now < cost.at(c)) {
                remaining.erase(c);
                continue;
            }

            sort(approvers_utilities[c].begin(), approvers_utilities[c].end(),
                      [&](const auto& a, const auto& b) {
                          return (budget[a.first] / a.second) < (budget[b.first] / b.second);
                      });

            double paid_so_far = 0.0;
            double denominator = remaining[c];
            for (const auto& [voter, utility] : approvers_utilities[c]) {
                double max_payment = (cost.at(c) - paid_so_far) / denominator;
                double eff_vote_count = cost.at(c) / max_payment;

                if (max_payment * utility > budget[voter]) {
                    paid_so_far += budget[voter];
                    denominator -= utility;
                } else {
                    remaining[c] = eff_vote_count;
                    if (eff_vote_count > best_eff_vote_count) {
                        best_eff_vote_count = eff_vote_count;
                        best = {c};
                    } else if (eff_vote_count == best_eff_vote_count) {
                        best.push_back(c);
                    }
                    break;
                }
            }
        }

        if (best.empty()) break;

        best = break_ties(voters, projects, cost, approvers_utilities, best);
        if (best.size() > 1) {
            throw runtime_error("Tie-breaking failed: unresolved tie between projects.");
        }

        const auto& selected = best.front();
        winners.push_back(selected);
        remaining.erase(selected);

        double best_max_payment = cost.at(selected) / best_eff_vote_count;
        for (const auto& [voter, utility] : approvers_utilities[selected]) {
            double payment = best_max_payment * utility;
            budget[voter] -= min(payment, budget[voter]);
        }
    }

    return winners;
}
