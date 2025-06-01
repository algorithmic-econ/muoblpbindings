#include <iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <functional>
#include <utility>

using namespace std;

unordered_map<string, int> get_project_utilities(
    const vector<string>& projects,
    const unordered_map<string, vector<pair<string, int>>>& approvers_utilities) {

    unordered_map<string, int> project_utilities;

    for (const auto& project : projects) {
        int total_utility = 0;
        for (const auto& [voter, utility] : approvers_utilities.at(project)) {
            total_utility += utility;
        }
        project_utilities[project] = total_utility;
    }

    return project_utilities;
}

vector<string> break_ties(const vector<string>& voters,
                          const vector<string>& projects,
                          const unordered_map<string, double>& cost,
                          const unordered_map<string, vector<pair<string, int>>>& approvers_utilities,
                          const vector<string>& choices) {
    vector<string> remaining = choices;

    // Find the minimum cost among the remaining projects
    double best_cost = numeric_limits<double>::max();
    for (const auto& c : remaining) {
        best_cost = min(best_cost, cost.at(c));
    }

    // Filter remaining projects with the minimum cost
    remaining.erase(remove_if(remaining.begin(), remaining.end(),
                              [&cost, best_cost](const string& c) {
                                  return cost.at(c) != best_cost;
                              }), remaining.end());

    // Find the maximum number of approvers among the remaining projects
    size_t best_count = 0;
    for (const auto& c : remaining) {
        best_count = max(best_count, approvers_utilities.at(c).size());
    }

    // Filter remaining projects with the maximum approvers
    remaining.erase(remove_if(remaining.begin(), remaining.end(),
                              [&approvers_utilities, best_count](const string& c) {
                                  return approvers_utilities.at(c).size() != best_count;
                              }), remaining.end());

    return remaining;
}

// Function to compute Equal Shares Method
vector<string> equal_shares(const vector<string>& voters,
                            const vector<string>& projects,
                            const unordered_map<string, double>& cost,
                            unordered_map<string, vector<pair<string, int>>>& approvers_utilities,
                            double total_budget) {
    // Step 1: Initialize budget for each voter
    unordered_map<string, double> budget;
    double individual_budget = total_budget / voters.size();
    for (const auto& voter : voters) {
        budget[voter] = individual_budget;
    }

    unordered_map<string, double> remaining;  // remaining candidate -> previous effective vote count
    for (const auto& c : projects) {
        if (cost.at(c) > 0 && !approvers_utilities.at(c).empty()) {
            remaining[c] = approvers_utilities.at(c).size();
        }
    }

    vector<string> winners;
    while (true) {
        vector<string> best;
        double best_eff_vote_count = 0.0;

        // Step 2: Sort remaining candidates by effective vote count (descending order)
        vector<string> remaining_sorted;
        for (const auto& entry : remaining) {
            remaining_sorted.push_back(entry.first);
        }
        sort(remaining_sorted.begin(), remaining_sorted.end(),
             [&remaining](const string& c1, const string& c2) {
                 return remaining.at(c1) > remaining.at(c2);
             });

        // Step 3: Process each candidate and calculate the effective vote count
        for (const auto& c : remaining_sorted) {
            double previous_eff_vote_count = remaining.at(c);

            if (previous_eff_vote_count < best_eff_vote_count) {
                break;  // No need to continue since we already found a better candidate
            }

            double money_behind_now = 0.0;
            for (const auto& [voter, _] : approvers_utilities.at(c)) {
                money_behind_now += budget.at(voter);
            }

            if (money_behind_now < cost.at(c)) {
                remaining.erase(c);  // Candidate cannot be afforded, remove from remaining
                continue;
            }

            // Sort the approvers by their available budget (ascending order)
            sort(approvers_utilities.at(c).begin(), approvers_utilities.at(c).end(),
                 [&budget](const pair<string, int>& v1, const pair<string, int>& v2) {
                     return budget.at(v1.first) < budget.at(v2.first);
                 });

            double paid_so_far = 0.0;
            size_t denominator = approvers_utilities.at(c).size();
            for (const auto& [i, _] : approvers_utilities.at(c)) {
                double max_payment = (cost.at(c) - paid_so_far) / denominator;
                double eff_vote_count = cost.at(c) / max_payment;

                if (max_payment > budget.at(i)) {
                    paid_so_far += budget.at(i);
                    denominator -= 1;
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

        if (best.empty()) {
            break;  // No remaining candidates can be afforded
        }

        // Step 4: Break ties if necessary
        best = break_ties(voters, projects, cost, approvers_utilities, best);
        if (best.size() > 1) {
            throw runtime_error("Tie-breaking failed: tie between projects could not be resolved.");
        }

        // Step 5: Select the best project
        string selected_project = best[0];
        winners.push_back(selected_project);
        remaining.erase(selected_project);

        // Charge the approvers of the selected project
        double best_max_payment = cost.at(selected_project) / best_eff_vote_count;
        for (const auto& [i, _] : approvers_utilities.at(selected_project)) {
            if (budget.at(i) > best_max_payment) {
                budget.at(i) -= best_max_payment;
            } else {
                budget.at(i) = 0.0;
            }
        }
    }

    return winners;
}
