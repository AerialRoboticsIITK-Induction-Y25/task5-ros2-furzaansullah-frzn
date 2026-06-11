#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

#include <string>
#include <vector>
#include <sstream>

class HealthMonitor : public rclcpp::Node {
    private:
        std::vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> subscriptions_;

        // Helper to split string by delimiter
        std::vector<std::string> split_string(const std::string& str, char delimiter) {
            std::vector<std::string> tokens;
            std::string token;
            std::stringstream ss(str);
            while (std::getline(ss, token, delimiter)) {
                tokens.push_back(token);
            }
            return tokens;
        }

        // Helper to get value from key-value tokens (e.g., "battery:15.5" -> "15.5")
        std::string extract_value(const std::string& token) {
            size_t pos = token.find(':');
            if (pos != std::string::npos) {
                return token.substr(pos + 1);
            }
            return "";
        }

    public:
        HealthMonitor() : Node("health_monitor") {
            RCLCPP_INFO(this->get_logger(), "=== Emergency Health Monitor Node Initialized ===");

            std::vector<std::string> drones = {"Alpha", "Beta", "Gamma"};

            for (const auto& name : drones) {
                // Subscribe to Status to monitor battery safety limits manually
                subscriptions_.push_back(this->create_subscription<std_msgs::msg::String>(
                    "/drone/" + name + "/status", 10,
                    [this, name](const std_msgs::msg::String::SharedPtr msg) {
                        this->check_battery_health(msg, name);
                    }
                ));

                // Subscribe to explicit drone Alerts
                subscriptions_.push_back(this->create_subscription<std_msgs::msg::String>(
                    "/drone/" + name + "/alert", 10,
                    [this, name](const std_msgs::msg::String::SharedPtr msg) {
                        this->handle_emergency_alert(msg, name);
                    }
                ));
            }
        }

    private:
        void check_battery_health(const std_msgs::msg::String::SharedPtr msg, const std::string& drone_name) {
            std::vector<std::string> parts = split_string(msg->data, '|');
            float battery_level = 100.0;

            for (const auto& part : parts) {
                std::string key = part.substr(0, part.find(':'));
                if (key == "battery") {
                    battery_level = std::stof(extract_value(part));
                    break;
                }
            }

            // Safety rule validation
            if (battery_level < 20.0) {
                RCLCPP_ERROR(this->get_logger(), 
                    "\033[1;31m[CRITICAL BATTERY BREACH] Drone %s is at %.1f%%! Safety thresholds violated!\033[0m", 
                    drone_name.c_str(), battery_level);
            }
        }

        void handle_emergency_alert(const std_msgs::msg::String::SharedPtr msg, const std::string& drone_name) {
            RCLCPP_ERROR(this->get_logger(), 
                "\033[1;33m[INTERCEPTED ALERT - %s]: %s\033[0m", 
                drone_name.c_str(), msg->data.c_str());
        }
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<HealthMonitor>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}