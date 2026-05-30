#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <std_msgs/msg/string.hpp>      // 新增：用于发布 /armor_result
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>
#include <chrono>
#include "armor_detect_ros/armor_detector.h"

class DetectNode : public rclcpp::Node
{
public:
    DetectNode() : Node("detect_node")
    {
        // 声明所有参数（与 YAML 中的键名一致）
        this->declare_parameter<std::string>("image_topic", "/image_raw");
        this->declare_parameter<bool>("debug", true);

        // 红色阈值
        this->declare_parameter<int>("red_h_low1", 0);
        this->declare_parameter<int>("red_s_low1", 150);
        this->declare_parameter<int>("red_v_low1", 80);
        this->declare_parameter<int>("red_h_high1", 10);
        this->declare_parameter<int>("red_s_high1", 255);
        this->declare_parameter<int>("red_v_high1", 255);

        this->declare_parameter<int>("red_h_low2", 160);
        this->declare_parameter<int>("red_s_low2", 150);
        this->declare_parameter<int>("red_v_low2", 80);
        this->declare_parameter<int>("red_h_high2", 180);
        this->declare_parameter<int>("red_s_high2", 255);
        this->declare_parameter<int>("red_v_high2", 255);

        // 蓝色阈值
        this->declare_parameter<int>("blue_h_low", 100);
        this->declare_parameter<int>("blue_s_low", 100);
        this->declare_parameter<int>("blue_v_low", 100);
        this->declare_parameter<int>("blue_h_high", 130);
        this->declare_parameter<int>("blue_s_high", 255);
        this->declare_parameter<int>("blue_v_high", 255);

        // 形态学核
        this->declare_parameter<int>("red_open_kernel", 5);
        this->declare_parameter<int>("red_close_kernel", 30);
        this->declare_parameter<int>("blue_open_kernel", 2);
        this->declare_parameter<int>("blue_close_kernel", 5);

        // 灯条筛选
        this->declare_parameter<double>("min_area", 50.0);
        this->declare_parameter<double>("min_length", 10.0);
        this->declare_parameter<double>("min_ratio", 2.0);
        this->declare_parameter<double>("max_ratio", 8.0);
        this->declare_parameter<double>("angle_tol", 10.0);

        // 灯条配对
        this->declare_parameter<double>("max_angle_diff", 15.0);
        this->declare_parameter<double>("max_len_ratio", 1.8);
        this->declare_parameter<double>("min_dist_factor", 0.3);
        this->declare_parameter<double>("max_dist_factor", 5.0);

        // 框扩展比例
        this->declare_parameter<double>("extend_ratio", 0.3);

        // 先构造检测器
        detector_ = std::make_unique<ArmorDetector>();
        loadParameters();

        // 订阅图像
        std::string topic = this->get_parameter("image_topic").as_string();
        debug_ = this->get_parameter("debug").as_bool();
        sub_ = this->create_subscription<sensor_msgs::msg::Image>(
            topic, 10, std::bind(&DetectNode::imageCallback, this, std::placeholders::_1));

        // 发布调试图像
        debug_pub_ = this->create_publisher<sensor_msgs::msg::Image>("/armor_debug_image", 10);

        // 新增：发布装甲板结果字符串
        result_pub_ = this->create_publisher<std_msgs::msg::String>("/armor_result", 10);
    }

private:
    void loadParameters()
    {
        detector_->setRedThreshold1(
            this->get_parameter("red_h_low1").as_int(),
            this->get_parameter("red_s_low1").as_int(),
            this->get_parameter("red_v_low1").as_int(),
            this->get_parameter("red_h_high1").as_int(),
            this->get_parameter("red_s_high1").as_int(),
            this->get_parameter("red_v_high1").as_int()
        );
        detector_->setRedThreshold2(
            this->get_parameter("red_h_low2").as_int(),
            this->get_parameter("red_s_low2").as_int(),
            this->get_parameter("red_v_low2").as_int(),
            this->get_parameter("red_h_high2").as_int(),
            this->get_parameter("red_s_high2").as_int(),
            this->get_parameter("red_v_high2").as_int()
        );
        detector_->setBlueThreshold(
            this->get_parameter("blue_h_low").as_int(),
            this->get_parameter("blue_s_low").as_int(),
            this->get_parameter("blue_v_low").as_int(),
            this->get_parameter("blue_h_high").as_int(),
            this->get_parameter("blue_s_high").as_int(),
            this->get_parameter("blue_v_high").as_int()
        );
        detector_->setMorphKernels(
            this->get_parameter("red_open_kernel").as_int(),
            this->get_parameter("red_close_kernel").as_int(),
            this->get_parameter("blue_open_kernel").as_int(),
            this->get_parameter("blue_close_kernel").as_int()
        );
        detector_->setFilterParams(
            this->get_parameter("min_area").as_double(),
            this->get_parameter("min_length").as_double(),
            this->get_parameter("min_ratio").as_double(),
            this->get_parameter("max_ratio").as_double(),
            this->get_parameter("angle_tol").as_double()
        );
        detector_->setPairParams(
            this->get_parameter("max_angle_diff").as_double(),
            this->get_parameter("max_len_ratio").as_double(),
            this->get_parameter("min_dist_factor").as_double(),
            this->get_parameter("max_dist_factor").as_double()
        );
        detector_->setExtendRatio(
            this->get_parameter("extend_ratio").as_double()
        );
    }

    void imageCallback(const sensor_msgs::msg::Image::SharedPtr msg)
    {
        // 限制处理频率（每 100ms 一次）
        auto now = std::chrono::steady_clock::now();
        if (last_process_time_ != std::chrono::steady_clock::time_point() &&
            now - last_process_time_ < std::chrono::milliseconds(100)) {
            return;
        }
        last_process_time_ = now;

        cv::Mat frame;
        try {
            frame = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8)->image;
        } catch (const cv_bridge::Exception& e) {
            RCLCPP_ERROR(this->get_logger(), "cv_bridge 转换失败: %s", e.what());
            return;
        }

        auto results = detector_->detect(frame);
        RCLCPP_INFO(this->get_logger(), "检测到 %zu 个装甲板", results.size());

        // 发布装甲板结果字符串
        if (!results.empty()) {
            std::string result_str;
            for (const auto& armor : results) {
                result_str += armor.color + ": ";
                for (size_t i = 0; i < armor.corners.size(); i++) {
                    result_str += "(" + std::to_string(static_cast<int>(armor.corners[i].x)) + "," +
                                  std::to_string(static_cast<int>(armor.corners[i].y)) + ")";
                    if (i < armor.corners.size() - 1) result_str += " ";
                }
                result_str += " | ";
            }
            if (!result_str.empty()) {
                result_str.pop_back(); // 去掉最后的空格
                auto msg_out = std_msgs::msg::String();
                msg_out.data = result_str;
                result_pub_->publish(msg_out);
            }
        }

        // 发布调试图像
        if (debug_) {
            cv::Mat debug_img = detector_->getDebugFrame();
            auto debug_msg = cv_bridge::CvImage(
                msg->header, sensor_msgs::image_encodings::BGR8, debug_img).toImageMsg();
            debug_pub_->publish(*debug_msg);
        }
    }

    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr sub_;
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr debug_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr result_pub_;   // 新增
    std::unique_ptr<ArmorDetector> detector_;
    bool debug_;
    std::chrono::steady_clock::time_point last_process_time_;
};

int main(int argc, char* argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<DetectNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}