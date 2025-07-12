#include "rclcpp/rclcpp.hpp"
#include "lifecycle_msgs/msg/transition_event.hpp"
#include "std_msgs/msg/bool.hpp"
#include <string>
#include <sstream>

class ControllersStateMonitor : public rclcpp::Node {
public:
  ControllersStateMonitor(const std::vector<std::string> & target_nodes)
  : Node("controller_state_monitor"), launched_(false)
  {
    this->declare_parameter<std::string>("robot_pkg_path", "");
    
    robot_pkg_path_ = this->get_parameter("robot_pkg_path").as_string();
  
    for (const auto & name : target_nodes) {
      states_[name] = "unknown";
      auto sub = create_subscription<lifecycle_msgs::msg::TransitionEvent>(
        "/" + name + "/transition_event", 10,
        [this, name](lifecycle_msgs::msg::TransitionEvent::SharedPtr msg) {
          if (msg->goal_state.label == "active") {
            RCLCPP_INFO(get_logger(), "%s is now ACTIVE", name.c_str());
            states_[name] = "active";
            check_all_active();
          } else {
            RCLCPP_INFO(get_logger(), "%s transitioned to %s",
              name.c_str(), msg->goal_state.label.c_str());
          }
        });
      subs_.push_back(sub);
    }
  }

private:
  void check_all_active() {
    if (launched_) return;
    for (const auto & pair : states_) {
      if (pair.second != "active") return;
    }

    RCLCPP_INFO(get_logger(), "All nodes are ACTIVE. Publishing launch_trigger...");
    
    std::stringstream nav_cmd;
    nav_cmd << "ros2 launch " << robot_pkg_path_
            << "/launch/parts/bringup_dummy_lidar.launch.py " << " &";
    std::system(nav_cmd.str().c_str());
    
    launched_ = true;
  }

  std::vector<rclcpp::Subscription<lifecycle_msgs::msg::TransitionEvent>::SharedPtr> subs_;
  std::map<std::string, std::string> states_;
  bool launched_;
  std::string robot_pkg_path_;
};

int main(int argc, char ** argv) {
  rclcpp::init(argc, argv);
  std::vector<std::string> names = {"mechanum_controller", "joint_state_broadcaster"};
  auto node = std::make_shared<ControllersStateMonitor>(names);
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}

