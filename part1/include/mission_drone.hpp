#pragma once
#include "drone.hpp"
#include <vector>
#include <tuple>
#include <string>
#include <utility>

class MissionDrone : public Drone {
    private:
        std::vector<std::pair<std::tuple<float,float,float>, std::string>> visited_waypoints;
    public:
        std::string mission_name;
        std::vector<std::tuple<float, float, float>> waypoints;
        unsigned int current_waypoint_index = 0;

        void skip_waypoint(const std::string& reason);

        std::tuple<float, float, float> next_waypoint();

        bool mission_complete();

        std::string mission_summary();

        void get_info() override;
};