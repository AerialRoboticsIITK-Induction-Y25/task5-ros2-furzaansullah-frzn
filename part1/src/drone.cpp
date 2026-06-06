#include "drone_exceptions.hpp"
#include "drone.hpp"
#include <iostream>

using namespace std;

void Drone::take_off(float target_altitude) {
    if (target_altitude > max_altitude)
        throw AltitudeError();
    else
        altitude = target_altitude;
}

void Drone::land() {
    set_status("idle");
    altitude = 0;
}

void Drone::emergency_stop() {
    float current_battery = get_battery_level();
    if (current_battery == 0)
        throw BatteryDepletedError(current_battery);
    else if (current_battery < 30)
        reduce_battery_level(get_battery_level());
    else 
        reduce_battery_level(30);

    throw InvalidStateError();
}

void Drone::get_info() {
    cout << "Name: " << name << endl;
    cout << "Altitude: " << altitude << endl;
    cout << "Battery Level: " << get_battery_level() << endl;
}