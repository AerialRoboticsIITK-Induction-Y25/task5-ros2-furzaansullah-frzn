#pragma once
#include <exception>
#include <string>

class BaseDroneException : public std::exception {
    public:
        virtual const char* what() const noexcept override {
            return "Base Drone Exception";
        }
};

class BatteryDepletedError: public BaseDroneException {
    private:
        std::string message;
    public:
        BatteryDepletedError(float battery) : message("Low Battery percentage: " + std::to_string(battery)) {}
        virtual const char* what() const noexcept override {
            return message.c_str();
        }
};

class InvalidStateError : public BaseDroneException {
    public:
        const char* what() const noexcept override {
            return "Invalid Drone State";
        }
};

class AltitudeError : public BaseDroneException {
    public:
        const char* what() const noexcept override {
            return "Target Altitude Exceeds safety limits";
        }
};