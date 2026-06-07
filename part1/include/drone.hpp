#pragma once
#include <string>
#include <vector>
#include "vehicle.hpp"

class Drone : public Vehicle {
    protected:
        float altitude = 0.0;
        float max_altitude = 25.0;
    private:
        float speed = 0.0;
    public :
        void take_off(float target_altitude);

        void land();

        void emergency_stop();

        void get_info() override;
};