#include "autonomous_drone.hpp"
#include <iostream>
#include <chrono>
#include <sstream>
#include <cmath>
#include <iomanip>

using namespace std;

void AutonomousDrone::set_ai_mode(const string& mode) {
    ai_mode = mode;

    if (mode == "return_home") {
        waypoints.insert(waypoints.begin() + current_waypoint_index, home_position);
    }
}

void AutonomousDrone::detect_obstacle(tuple<float, float, float> position, const string severity) {
    float x = get<0>(position);
    float y = get<1>(position);
    float z = get<2>(position);

    auto now = chrono::system_clock::now();
    auto in_time_t = chrono::system_clock::to_time_t(now);
    stringstream ss;
    ss << put_time(localtime(&in_time_t), "%Y-%m-%d %X") << " - OBSTACLE DETECTED at (" << x << ", " << y << ", " << z << ") [" << severity << "]";
    obstacle_log.push_back(ss.str());

    if (severity == "high")
        emergency_stop();
}

vector<tuple<float, float, float>> AutonomousDrone::auto_replan(const vector<tuple<float, float, float>>& obstacles) {
    vector<tuple<float, float, float>> safe_waypoints;

    for (const auto& wp: waypoints) {
        float wp_x = get<0>(wp);
        float wp_y = get<1>(wp);
        float wp_z = get<2>(wp);

        bool safe = true;

        for (const auto& obs : obstacles) {
            float obs_x = get<0>(obs);
            float obs_y = get<1>(obs);
            float obs_z = get<2>(obs);

            float distance = sqrt(pow(obs_x - wp_x, 2) + pow(obs_y - wp_y, 2) + pow(obs_z - wp_z, 2));

            if (distance < 5.0) {
                safe = false;
                break;
            }
        }
        if (safe)
            safe_waypoints.push_back(wp);
    }

    waypoints = safe_waypoints;
    current_waypoint_index = 0;

    return safe_waypoints;
}

void AutonomousDrone::get_info() {
    MissionDrone::get_info();
    cout << "AI Mode: " << ai_mode << endl;
}