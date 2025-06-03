#pragma once

#include <vector> // Required for std::vector
#include <string> // Required for std::string
#include <unordered_map> // Required for std::unordered_map
#include <utility> // Required for std::pair

std::vector<std::string> equal_shares_add1(
    const std::vector<std::string>& voters,
    const std::vector<std::string>& projects,
    const std::unordered_map<std::string, double>& cost,
    std::unordered_map<std::string, std::vector<std::pair<std::string, double>>> approvers_utilities,
    double total_budget);
