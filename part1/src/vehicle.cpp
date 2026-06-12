#include "vehicle.hpp"
#include "drone_exceptions.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>

using namespace std;

void Vehicle::reduce_battery_level(float amount) {
    battery_level -= amount;
}

void Vehicle::set_status(const string& new_status) {
    if(new_status != "idle" && new_status != "flying" && new_status != "charging") {
        throw InvalidStateError();
    }
    status = new_status;

    auto now = chrono::system_clock::now();
    auto in_time_t = chrono::system_clock::to_time_t(now);
    stringstream ss;
    ss << put_time(localtime(&in_time_t), "%Y-%m-%d %X") << " - Status changed to: " << status;
    flight_log.push_back(ss.str());
}

void Vehicle::drain_battery(float amount) {
    if (battery_level == 0)
        throw BatteryDepletedError(battery_level);
    else if (battery_level - amount < 0)
        battery_level = 0;
    else
        battery_level -= amount;
}

void Vehicle::charge_battery(float amount, int duration_seconds) {
    if (status == "flying")
        throw InvalidStateError();
    string old_status = status;
    status = "charging";
    battery_level += (amount*duration_seconds);
    if (battery_level > 100.0) battery_level = 100.0;
    status = old_status;
}

bool Vehicle:: is_critical() {
    return (battery_level < 20.0);
}

string Vehicle::get_flight_log() {
    string log_summary = "";
    for (const auto& log_entry : flight_log) {
        log_summary += log_entry + "\n";
}
    return log_summary;
}

float Vehicle::get_battery_level() {
    return battery_level;
}

string Vehicle::get_status() {
    return status;
}