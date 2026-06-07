#pragma once
#include "mission_drone.hpp"
#include <vector>
#include <string>
#include <tuple>

class AutonomousDrone : public MissionDrone {
    private:
        std::vector<std::string> obstacle_log;
    public:
        std::string ai_mode;
        std::tuple<float, float, float> home_position;

        void set_ai_mode(const std::string& mode);

        void detect_obstacle(std::tuple<float, float, float> position, const std::string severity);

        std::vector<std::tuple<float, float, float>> auto_replan(const std::vector<std::tuple<float, float, float>>& obstacles);

        void get_info() override;
};