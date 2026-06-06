#include "mission_drone.hpp"
#include "drone_exceptions.hpp"
#include <iostream>

using namespace std;

tuple<float, float, float> MissionDrone::next_waypoint() {
    if (waypoints.empty() || current_waypoint_index >= waypoints.size()) {
        throw InvalidStateError();
    }
    drain_battery(1.5);
    return waypoints[current_waypoint_index++];
}

bool MissionDrone::mission_complete() {
    return (current_waypoint_index >= waypoints.size());
}

string MissionDrone::mission_summary() {
    if (mission_complete()) return "COMPLETED";
    return "PENDING";
}
        
void MissionDrone::get_info() {
    Drone::get_info();
    cout << "Mission Name: " << mission_name << endl;
}