using VoterId = std::string;
using CandidateId = std::string;

using Utility = double;
using CostMap = std::unordered_map<CandidateId, double>;
using BudgetMap = std::unordered_map<VoterId, double>;
using ApproversMap = std::unordered_map<CandidateId, std::vector<std::pair<VoterId, Utility>>>;

std::vector<CandidateId> equal_shares_utils(
    const std::vector<VoterId>& voters,
    const std::vector<CandidateId>& projects,
    const CostMap& cost,
    ApproversMap approvers_utilities,
    double total_budget)
