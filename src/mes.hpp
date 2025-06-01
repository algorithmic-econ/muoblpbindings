std::vector<std::string> equal_shares(const std::vector<std::string>& voters,
                            const std::vector<std::string>& projects,
                            const std::unordered_map<std::string, double>& cost,
                            std::unordered_map<std::string, std::vector<std::pair<std::string, int>>>& approvers_utilities,
                            double total_budget);
