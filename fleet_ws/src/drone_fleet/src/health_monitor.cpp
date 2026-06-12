#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

#include <string>
#include <vector>
#include <deque>
#include <map>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <iostream>

using namespace std::chrono_literals;

struct BatteryHistory {
    std::deque<float> readings;
    float average_drain_rate = 0.0;
};

class HealthMonitor : public rclcpp::Node {
    private:
        std::map<std::string, BatteryHistory> health_map_;

        std::vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> subscriptions_;
        rclcpp::Publisher<std_msgs::msg::String>::SharedPtr warning_pub_;
        rclcpp::Publisher<std_msgs::msg::String>::SharedPtr summary_pub_;
        rclcpp::TimerBase::SharedPtr reporting_timer_;

        float parse_json_float(const std::string& json, const std::string& key) {
            size_t key_pos = json.find("\"" + key + "\":");
            if (key_pos == std::string::npos) return 0.0f;

            size_t start_pos = key_pos + key.length() + 3;
            
            size_t end_pos = json.find_first_of(",}", start_pos);
            if (end_pos == std::string::npos) return 0.0f;

            std::string value_str = json.substr(start_pos, end_pos - start_pos);
            try {
                return std::stof(value_str);
            } catch (...) {
                return 0.0f;
            }
        }

    public:
        HealthMonitor() : Node("health_monitor") {
            RCLCPP_INFO(this->get_logger(), "Launching Advanced Telemetry Health Monitor Node");

            health_map_["Alpha"] = BatteryHistory();
            health_map_["Beta"]  = BatteryHistory();
            health_map_["Gamma"] = BatteryHistory();

            warning_pub_ = this->create_publisher<std_msgs::msg::String>("/fleet/health_warning", 10);
            summary_pub_ = this->create_publisher<std_msgs::msg::String>("/fleet/health_summary", 10);

            std::vector<std::string> targets = {"Alpha", "Beta", "Gamma"};
            for (const auto& name : targets) {
                subscriptions_.push_back(this->create_subscription<std_msgs::msg::String>(
                    "/drone/" + name + "/telemetry", 10,
                    [this, name](const std_msgs::msg::String::SharedPtr msg) {
                        this->process_telemetry(msg, name);
                    }
                ));
            }

            reporting_timer_ = this->create_wall_timer(10000ms, std::bind(&HealthMonitor::render_and_publish_diagnostics, this));
        }

    private:
        void process_telemetry(const std_msgs::msg::String::SharedPtr msg, const std::string& drone_name) {
            float battery_level = parse_json_float(msg->data, "battery_level");
            auto& tracker = health_map_[drone_name];
            tracker.readings.push_back(battery_level);

            if (tracker.readings.size() > 10) {
                tracker.readings.pop_front();
            }

            if (tracker.readings.size() >= 2) {
                size_t n = tracker.readings.size();
                float instantaneous_rate = (tracker.readings[n - 2] - tracker.readings[n - 1]) / 2.0f;

                if (instantaneous_rate > 1.5f) {
                    std_msgs::msg::String warn_msg;
                    warn_msg.data = "WARNING: Drone " + drone_name + " battery drain rate is CRITICAL at " +
                                    std::to_string(instantaneous_rate) + "/s!";
                    warning_pub_->publish(warn_msg);

                    RCLCPP_WARN(this->get_logger(), "%s", warn_msg.data.c_str());
                }
            }
        }

        void render_and_publish_diagnostics() {
            std::cout << "\033[2J\033[1;1H";

            std::cout << "=========================================================================================\n";
            std::cout << "                                AUTOMATED HEALTH REPORT STATION                       \n";
            std::cout << "=========================================================================================\n";
            std::cout << std::left
                      << std::setw(12) << "DRONE"
                      << std::setw(15) << "DRAIN RATE"
                      << std::setw(25) << "TIME TO CRITICAL (<20%)"
                      << "TIME TO DEPLETION (0%)\n";
            std::cout << "-----------------------------------------------------------------------------------------\n";

            std::stringstream json_summary_ss;
            json_summary_ss << "{";

            bool first_entry = true;

            for (auto& [name, tracker] : health_map_) {
                float calculated_drain_rate = 0.0f;

                if (tracker.readings.size() >= 2) { 
                    float total_drop = tracker.readings.front() - tracker.readings.back();
                    float total_time = (tracker.readings.size() - 1) * 2.0f; 
                    calculated_drain_rate = (total_time > 0) ? (total_drop / total_time) : 0.0f;
                }
                
                tracker.average_drain_rate = calculated_drain_rate;

                float current_battery = tracker.readings.empty() ? 0.0f : tracker.readings.back();

                std::string str_time_to_crit = "N/A";
                std::string str_time_to_deplete = "N/A";

                if (calculated_drain_rate > 0.001f) {
                    if (current_battery > 20.0f) {
                        float sec_to_crit = (current_battery - 20.0f) / calculated_drain_rate;
                        str_time_to_crit = std::to_string(static_cast<int>(sec_to_crit)) + "s";
                    } else {
                        str_time_to_crit = "ALREADY CRITICAL";
                    }

                    float sec_to_deplete = current_battery / calculated_drain_rate;
                    str_time_to_deplete = std::to_string(static_cast<int>(sec_to_deplete)) + "s";
                } else if (!tracker.readings.empty()) {
                    str_time_to_deplete = "STABLE / IDLE";
                }

                std::cout << std::left
                          << std::setw(12) << name
                          << std::fixed << std::setprecision(2) << std::setw(1) << calculated_drain_rate << std::setw(14) << "%/s"
                          << std::setw(25) << str_time_to_crit
                          << str_time_to_deplete << "\n";

                if (!first_entry) json_summary_ss << ",";
                json_summary_ss << "\"" << name << "\": {"
                                << "\"drain_rate\": " << calculated_drain_rate << ","
                                << "\"time_to_critical\": \"" << str_time_to_crit << "\","
                                << "\"time_to_depletion\": \"" << str_time_to_deplete << "\""
                                << "}";
                first_entry = false;
            }

            json_summary_ss << "}";
            std::cout << "=========================================================================================\n";
            std::flush(std::cout);

            std_msgs::msg::String summary_msg;
            summary_msg.data = json_summary_ss.str();
            summary_pub_->publish(summary_msg);
        }
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<HealthMonitor>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}