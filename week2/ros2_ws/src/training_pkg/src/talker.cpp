#include <chrono>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;

class Talker : public rclcpp::Node
{
public:
  Talker()
  : Node("talker"), count_(0)
  {
    this->declare_parameter<double>("period", 0.5);
    double period = this->get_parameter("period").as_double();
    // 创建一个发布者，话题名为 "chatter"，消息类型为 String，队列长度为 10
    publisher_ = this->create_publisher<std_msgs::msg::String>("chatter", 10);

    // 创建一个定时器，每 500ms 调用一次回调函数 timer_callback
    timer_ = this->create_wall_timer(
    	std::chrono::duration<double>(period),
        std::bind(&Talker::timer_callback, this));
    
    
    RCLCPP_INFO(this->get_logger(), "启动 talker，发布周期 %.2f 秒", period);
  }

private:
  void timer_callback()
  {
    auto message = std_msgs::msg::String();
    message.data = "Hello, world! " + std::to_string(count_++);
    RCLCPP_INFO(this->get_logger(), "发布: '%s'", message.data.c_str());
    publisher_->publish(message);   // 向 chatter 话题发布消息
  }

  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
  size_t count_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);              // 初始化 ROS 2
  rclcpp::spin(std::make_shared<Talker>()); // 让节点持续运行
  rclcpp::shutdown();
  return 0;
}
