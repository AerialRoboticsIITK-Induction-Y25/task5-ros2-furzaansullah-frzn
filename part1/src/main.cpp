#include "vehicle.hpp"
#include "drone.hpp"
#include "mission_drone.hpp"
#include "autonomous_drone.hpp"
#include "drone_exceptions.hpp"
#include <vector>
#include <iostream>

using namespace std;

int main() {
    vector<Vehicle*> fleet;

    Drone* alpha = new Drone();
    alpha->name = "Alpha";
    alpha->charge_battery(2, 30);
    alpha->take_off(10.0);
    
    MissionDrone* beta = new MissionDrone();
    beta->name = "Beta";
    beta->charge_battery(2, 25);
    beta->take_off(9.0);
    beta->mission_name = "TestRun";

    AutonomousDrone* gamma = new AutonomousDrone();
    gamma->name = "Gamma";
    gamma->charge_battery(2, 27); // battery_level = 54;
    gamma->take_off(9.5);
    gamma->mission_name = "TestRun2";
    gamma->ai_mode = "return_home";

    fleet.push_back(alpha);
    fleet.push_back(beta);
    fleet.push_back(gamma);

    // POLYMORPHISM PROVED
    fleet[0]->get_info();
    cout << endl;
    fleet[1]->get_info();
    cout << endl;
    fleet[2]->get_info();

    // PRIVATE MEMBERS ARE NOT ACCESSIBLE DIRECTLY
    // cout << alpha->speed;
    /* Above line doesn't work because speed was declared under the private tag and
    it can be accessed directly only in the class in which it was declared*/

    gamma->drain_battery(15); // battery_level = 39
    // takeoff already used
    gamma->detect_obstacle({4, 2, 6}, "low");

    // Invoking Exceptions
    // 1) Altitude Error 
    try {
        gamma->take_off(26.0);
    }
    catch (const BaseDroneException& e) {
        cout << "Caught Expected Error: " << e.what() << endl;
    }
    // 2) Invalid State Error
    try {
        gamma->land();
        gamma->next_waypoint();
    }
    catch (const BaseDroneException& e) {
        cout << "Caught Expected Error: " << e.what() << endl;
    }
    // 3) Battery Depleted Error
    try {
        cout << "Draining battery via flight simulation..." << endl;
        while (true) {
            gamma->next_waypoint();
        }
    }
    catch (const BaseDroneException& e) {
        cout << "Caught Expected Error: " << e.what() << endl;
    }

    // Running a full Mission
    AutonomousDrone* epsilon = new AutonomousDrone();
    
    try {
        epsilon->home_position = {0.0, 0.0, 0.0};
        epsilon->mission_name = "Dawn Of Drones";
        epsilon->set_ai_mode("autonomous");

        epsilon->waypoints = {
            {5.0, 5.0, 5.0},
            {10.0, 5.0, 5.0},
            {20.0, 25.0, 15.0},
            {35.0, 40.0, 15.0},
        };

        cout << "Initiating launch sequence ..." << endl;
        epsilon->take_off(10.0);

        cout << "Waypoint traversing..." << endl;
        while (!epsilon->mission_complete()) {
            tuple<float, float, float> target = epsilon->next_waypoint();
            cout << " -> Navigating to waypoint index " << epsilon->current_waypoint_index << ": (" << get<0>(target) << ", " << get<1>(target) << ", " << get<2>(target) << ")" << endl;

            // Simulating an Obstacle encounter during flight
            if (epsilon->current_waypoint_index == 2) {
                cout << "Obstacle Detected Ahead..." << endl;

                // Low severity obstacle
                epsilon->detect_obstacle({12.0, 10.0, 5.0}, "low");
                // High severity obstacle
                epsilon->detect_obstacle({21.0, 26.0, 15.0}, "high");
                cout << "Recalculating path around the obstacle..." << endl;
                vector<tuple<float,float,float>> obstacles = {{21.0, 26.0, 15.0}};
                epsilon->auto_replan(obstacles);
                break; // Quit the loop after Replanning
            }
        }
    }

    catch (const InvalidStateError& e) {
        cout << "\n[MISSION HALTED] Emergency State triggered: " << e.what() << endl;
    }
    catch (const BatteryDepletedError& e) {
        cout << "\n[MISSION HALTED] Battery critically low: " << e.what() << endl;
    }
    catch (const AltitudeError& e) {
        cout << "\n[MISSION HALTED] Altitude limit breached: " << e.what() << endl;
    }
    catch (const BaseDroneException& e) {
        cout << "\n MISSION HALTED... Safety failure in main:" << e.what() << endl;
    }
    catch (const exception& e){
        cout << "\n SYSTEM ERROR... Standard exception encountered; " << e.what() << endl;
    }

    cout << "Final Mission Status Summary: " << epsilon->mission_summary() << endl;
    
    for (Vehicle* v : fleet) {
        delete v;
    }
    delete epsilon;
    
    return 0;
}