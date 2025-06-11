#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <utility>

std::vector<std::string> equal_shares_add1(
    const std::vector<std::string>& voters,
    const std::vector<std::string>& projects,
    const std::unordered_map<std::string, double>& cost,
    const std::unordered_map<std::string, std::vector<std::pair<std::string, int>>>& approvers_utilities,
    const std::unordered_map<std::string, int>& total_utility,
    double total_budget);
