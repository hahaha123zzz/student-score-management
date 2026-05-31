#pragma once
#include "../lib/json.hpp"
using json = nlohmann::json;

namespace enhance {
    // Learning goals
    json setGoal(const json& data);
    json getGoals(const std::string& studentId);
    
    // Knowledge mastery (知识点掌握度)
    json getKnowledgeMap(const std::string& studentId);
    
    // Growth portfolio
    json getPortfolio(const std::string& studentId);
    
    // XP/Points system
    json getPoints(const std::string& studentId);
    json addPoints(const std::string& studentId, int points, const std::string& reason);
    
    // Review reminders (based on error notebook)
    json getReviewPlan(const std::string& studentId);
    
    // Mental wellness check-in
    json checkin(const json& data);
    json getCheckins(const std::string& studentId);
}
