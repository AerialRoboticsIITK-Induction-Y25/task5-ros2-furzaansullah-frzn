#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <tuple>
#include <map>
#include <sstream>
#include <iomanip>
#include <iostream>

using namespace std::chrono_literals;

struct DroneData {
    std::string name = "Unknown";
    float battery = 0.0;
    float altitude = 0.0;
    std::string status = "idle";
    std::string waypoint_progress = "0/0";
    float speed = 0.0;
    std::string last_alert = "NONE";
    bool mission_done = false;
};

class FleetManager : public rclcpp::Node {
    private:
        // Dictionary mapping to store data fields for all drones
        std::map<std::string, DroneData> fleet_map_;
        // Unique Subscription handles per drone per channel to prevent overwriting
        std::vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> subscriptions_;
        // Timer
        rclcpp::TimerBase::SharedPtr interface_timer_;
        // Helper function to split the string by a constant delimiter
        std::vector<std::string> split_string(const std::string& str, char delimiter) {
            std::vector<std::string> tokens;
            std::string token;
            std::stringstream ss(str);
            while (std::getline(ss, token, delimiter)) {
                tokens.push_back(token);
            }
            return tokens;
        }
        // Helper function to extract value string from key-value pairs
        std::string extract_value(const std::string& token, char separator = ':') {
            size_t pos = token.find(separator);
            if (pos != std::string::npos) {
                return token.substr(pos + 1);
            }
            return "";
        }

    public:
        FleetManager() : Node("fleet_manager") {
            RCLCPP_INFO(this->get_logger(), "Starting Central Fleet Manager Console Node...");

            // Initialize known tracking elements into our dictionary map
            fleet_map_["Alpha"] = DroneData{"Alpha", 0.0, 0.0, "idle", "0/0", 0.0, "NONE", false};
            fleet_map_["Beta"]  = DroneData{"Beta", 0.0, 0.0, "idle", "0/0", 0.0, "NONE", false};
            fleet_map_["Gamma"] = DroneData{"Gamma", 0.0, 0.0, "idle", "0/0", 0.0, "NONE", false};
            std::vector<std::string> targets = {"Alpha", "Beta", "Gamma"};

            for (const auto& drone : targets) {
                // Subscribe to Status
                subscriptions_.push_back(this->create_subscription<std_msgs::msg::String>("/drone/" + drone + "/status", 10, [this](const std_msgs::msg::String::SharedPtr msg) {this->callback_status(msg); }));
                // Subscribe to Alert
                subscriptions_.push_back(this->create_subscription<std_msgs::msg::String>("/drone/" + drone + "/alert", 10, [this](const std_msgs::msg::String::SharedPtr msg) {this->callback_alert(msg); }));
                // Subscribe to Mission Completion updates
                subscriptions_.push_back(this->create_subscription<std_msgs::msg::String>("/drone/" + drone + "/mission_complete", 10, [this](const std_msgs::msg::String::SharedPtr msg) {this->callback_mission_complete(msg); }));
            }
            // Setup asynchronous dashboard UI updates every 5 seconds
            interface_timer_ = this->create_wall_timer(5000ms, std::bind(&FleetManager::render_dashboard, this));
        }
    private:
        // Callback to parse pipe delimited data strings: "name:Alpha|battery:87.3|altitude:10.0|status:flying..."
        void callback_status(const std_msgs::msg::String::SharedPtr msg) {
            std::vector<std::string> attributes = split_string(msg->data, '|');
            std::string d_name = "";
            DroneData temp_data;

            for (const auto& attr : attributes) {
                std::string key = attr.substr(0, attr.find(':'));
                std::string val = extract_value(attr);

                if (key == "name") d_name = val;
                else if (key == "battery") temp_data.battery = std::stof(val);
                else if (key == "altitude") temp_data.altitude = std::stof(val);
                else if (key == "status") temp_data.status = val;
                else if (key == "waypoint") temp_data.waypoint_progress = val;
                else if (key == "speed") temp_data.speed = std::stof(val);
            }

            if (!d_name.empty() && fleet_map_.find(d_name) != fleet_map_.end()) {
                temp_data.name = d_name;
                // Preserve state configurations not contained within basic status updates
                temp_data.last_alert = fleet_map_[d_name].last_alert;
                temp_data.mission_done = fleet_map_[d_name].mission_done;
                fleet_map_[d_name] = temp_data;
            }
        }

        void callback_alert(const std_msgs::msg::String::SharedPtr msg) {
            // Find which drone broadcasted the alert
            for (auto& [name, data] : fleet_map_) {
                if (msg->data.find(name) != std::string::npos) {
                    data.last_alert = msg->data;
                    RCLCPP_WARN(this->get_logger(), "ALERT RECEPTION:  %s", msg->data.c_str());
                }
            }
        }

        void callback_mission_complete(const std_msgs::msg::String::SharedPtr msg) {
            for (auto& [name, data] : fleet_map_) {
                if (msg->data.find(name) != std::string::npos) {
                    data.mission_done = true;
                    data.status = "landed";
                }
            }
        }

        void render_dashboard() {
            std::cout << "========================================================================";
            std::cout << "                   VIRTUAL FLEET MONITOR SYSTEM";
            std::cout << "========================================================================";
            std::cout << std::left
                      << std::setw(12) << "DRONE"
                      << std::setw(12) << "BATTERY"
                      << std::setw(14) << "ALTITUDE"
                      << std::setw(12) << "SPEED"
                      << std::setw(14) << "WAYPOINT"
                      << std::setw(12) << "STATUS"
                      << "MISSION\n";
            std::cout << "------------------------------------------------------------------------\n";

            for (const auto& [name, data] : fleet_map_) {
                std::string completion_status = data.mission_done ? "SUCCESS" : "IN PROGRESS";
                
                std::cout << std::left 
                          << std::setw(12) << data.name
                          << std::setw(1)  << std::fixed << std::setprecision(1) << data.battery << std::setw(11) << "%"
                          << std::setw(1)  << data.altitude << std::setw(13) << "m"
                          << std::setw(1)  << data.speed << std::setw(11) << "m/s"
                          << std::setw(14) << data.waypoint_progress
                          << std::setw(12) << data.status
                          << completion_status << "\n";
            }
            std::cout << "=========================================================================\n";
            
            // Print out critical active alert summary lists if present
            std::cout << "\n[RECENT CRITICAL INCIDENT LOGS]:\n";
            bool alerts_found = false;
            for (const auto& [name, data] : fleet_map_) {
                if (data.last_alert != "NONE") {
                    std::cout << " -> " << data.last_alert << "\n";
                    alerts_found = true;
                }
            }
            if (!alerts_found) std::cout << " -> All vehicle diagnostics report healthy operating thresholds.\n";
            std::cout << "=========================================================================\n";
            std::flush(std::cout);
            
        }
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    auto fleet_manager = std::make_shared<FleetManager>();
    rclcpp::spin(fleet_manager);
    rclcpp::shutdown();
    return 0;
}