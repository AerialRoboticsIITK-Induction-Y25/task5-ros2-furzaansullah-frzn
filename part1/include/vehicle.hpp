#pragma once
#include <string>
#include <vector>

using namespace std;

class Vehicle {
    private:
        float battery_level = 0.0;
        string status = "idle";
        vector<string> flight_log;

    protected:
        void reduce_battery_level(float amount);
        void set_status(const string& new_status);

    public:
        string name;
        
        virtual void get_info() = 0;
        void drain_battery(float amount);
        void charge_battery(float amount, int duration_seconds);
        bool is_critical();
        string get_flight_log();
        float get_battery_level();
        string get_status();

        virtual ~Vehicle() = default;
};