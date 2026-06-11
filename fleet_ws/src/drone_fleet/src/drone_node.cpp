#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

#include "mission_drone.hpp"
#include "drone_exceptions.hpp"

#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <tuple>
#include <sstream>
#include <iomanip>

using namespace std::chrono_literals;

class DroneNode : public rclcpp::Node {
    private:
        std::unique_ptr<MissionDrone> drone_;

        std::string drone_name_;
        std::string mission_name_;
        double initial_battery_;
        int publish_count_ = 0;
        double current_speed_ = 3.2;

        rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_pub_;
        rclcpp::Publisher<std_msgs::msg::String>::SharedPtr alert_pub_;
        rclcpp::Publisher<std_msgs::msg::String>::SharedPtr mission_complete_pub_;
        rclcpp::Publisher<std_msgs::msg::String>::SharedPtr telemetry_pub_;

        rclcpp::TimerBase::SharedPtr status_timer_;
        rclcpp::TimerBase::SharedPtr telemetry_timer_;
        
        std::string to_string_with_precision(float val, int precision = 1) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(precision) << val;
            return ss.str();
        }
    public:
        DroneNode() : Node("drone_node") {
            this->declare_parameter<std::string>("drone_name", "Alpha");
            this->declare_parameter<double>("initial_battery", 100.0);
            this->declare_parameter<std::string>("mission_name", "Default_Mission");

            this->get_parameter("drone_name", drone_name_);
            this->get_parameter("initial_battery", initial_battery_);
            this->get_parameter("mission_name", mission_name_);

            RCLCPP_INFO(this->get_logger(), "Initializing Drone Node for: %s", drone_name_.c_str());

            drone_ = std::make_unique<MissionDrone>();
            drone_->name = drone_name_;
            drone_->mission_name = mission_name_;

            drone_->waypoints = {
                {10.0, 10.0, 5.0},
                {15.0, 15.0, 20.0},
                {20.0, 25.0, 25.0},
                {20.0, 30.0, 30.0},
                {35.0, 30.0, 35.0}
            };

            try {
                /* If we want the initial battery of the drone to become initial_battery_
                   then we have to charge the battery for (initial_battery_) seconds as rate
                   of power supply is taken to be 1.0 units/second */
                drone_->charge_battery(1.0, static_cast<int>(initial_battery_));
                drone_->take_off(10.0);
            }
            catch (const BaseDroneException& e) {
                RCLCPP_ERROR(this->get_logger(), "Error setting up initial state: %s", e.what());
            }
            
            status_pub_ = this->create_publisher<std_msgs::msg::String>("/drone/" + drone_name_ + "/status", 10);
            alert_pub_ = this->create_publisher<std_msgs::msg::String>("/drone/" + drone_name_ + "/alert", 10);
            mission_complete_pub_ = this->create_publisher<std_msgs::msg::String>("/drone/" + drone_name_ + "/mission_complete", 10);
            telemetry_pub_ = this->create_publisher<std_msgs::msg::String>("/drone/" + drone_name_ + "/telemetry", 10);

            status_timer_ = this->create_wall_timer(1000ms, std::bind(&DroneNode::handle_status_publish, this));
            telemetry_timer_ = this->create_wall_timer(2000ms, std::bind(&DroneNode::handle_telemetry_publish, this));
        }

    private:
        void handle_status_publish() {
            try {
                drone_->drain_battery(0.5);
                publish_count_++;

                if (drone_->is_critical()) {
                    std_msgs::msg::String alert_msg;
                    alert_msg.data = "CRITICAl ALERT: Drone " + drone_name_ + " battery is dangerously low at " + to_string_with_precision(drone_->get_battery_level()) + "%";
                    alert_pub_->publish(alert_msg);

                    RCLCPP_WARN(this->get_logger(), "Battery critical! Initializing landing sequence.");
                    drone_->land();
                }

                if (publish_count_ % 3 == 0 && drone_->get_status() == "flying" && !drone_->mission_complete()) {
                    std::tuple<float, float, float> target = drone_->next_waypoint();
                    RCLCPP_INFO(this->get_logger(), "Advance to waypoint index %u: (%f, %f, %f)", drone_->current_waypoint_index, std::get<0>(target), std::get<1>(target), std::get<2>(target));
                }

                if (drone_->mission_complete()) {
                    std_msgs::msg::String complete_msg;
                    complete_msg.data = "Mission " + mission_name_ + " for drone " + drone_name_ + " has SUCCESSFULLY COMPLETED.";
                    mission_complete_pub_->publish(complete_msg);

                    RCLCPP_INFO(this->get_logger(), "Mission Complete!");
                    drone_->current_waypoint_index = 0;
                }

                std::stringstream ss;
                ss << "name:" << drone_name_ << "|"
                   << "battery:" << to_string_with_precision(drone_->get_battery_level()) << "|" // Fixed: get_battery_level() 
                   << "altitude:" << to_string_with_precision(10.0) << "|"
                   << "status:" << drone_->get_status() << "|"
                   << "waypoint:" << drone_->current_waypoint_index << "/" << drone_->waypoints.size() << "|"
                   << "speed:" << to_string_with_precision(current_speed_);
                
                std_msgs::msg::String status_msg;
                status_msg.data = ss.str();
                status_pub_->publish(status_msg);
            }
            catch (const BatteryDepletedError& e) {
                RCLCPP_ERROR(this->get_logger(), "Failure! Battery completed depleted: %s", e.what());
                drone_->land();
            }
            catch (const BaseDroneException& e) {
                RCLCPP_ERROR(this->get_logger(), "Drone exception encountered in runtime loop: %s", e.what());
            }
        }

        void handle_telemetry_publish() {
            std::stringstream json_ss;
            json_ss << "{"
                    << "\"drone_name\": \"" << drone_name_ << "\","
                    << "\"mission_name\": \"" << mission_name_ << "\","
                    << "\"battery_level\": " << to_string_with_precision(drone_->get_battery_level()) << ","
                    << "\"status\": \"" << drone_->get_status() << "\","
                    << "\"current_waypoint_index\": " << drone_->current_waypoint_index << ","
                    << "\"total_waypoints\": " << drone_->waypoints.size()
                    << "}";

            std_msgs::msg::String telemetry_msg;
            telemetry_msg.data = json_ss.str();
            telemetry_pub_->publish(telemetry_msg);
        }
};

int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<DroneNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}