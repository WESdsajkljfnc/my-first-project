#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <algorithm>

class ColorPicker : public rclcpp::Node
{
public:
    ColorPicker() : Node("color_picker"), show_mask_(false)
    {
        this->declare_parameter<std::string>("image_topic", "/image_raw");
        std::string topic = this->get_parameter("image_topic").as_string();
        sub_ = this->create_subscription<sensor_msgs::msg::Image>(
            topic, 10, std::bind(&ColorPicker::imageCallback, this, std::placeholders::_1));
        RCLCPP_INFO(this->get_logger(), "取色节点已启动，订阅 %s", topic.c_str());
        RCLCPP_INFO(this->get_logger(), "点击图像记录样本，按 'm' 显示/隐藏掩膜，按 'r' 重置红色，按 'b' 重置蓝色，ESC 退出");
    }

private:
    void imageCallback(const sensor_msgs::msg::Image::SharedPtr msg)
    {
        cv::Mat frame;
        try {
            frame = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8)->image;
        } catch (const cv_bridge::Exception& e) {
            RCLCPP_ERROR(this->get_logger(), "转换失败: %s", e.what());
            return;
        }

        // 转换为 HSV
        cv::Mat hsv;
        cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

        // 根据是否显示掩膜决定展示内容
        cv::Mat display;
        if (show_mask_) {
            display = generateMaskOverlay(frame, hsv);
        } else {
            display = frame.clone();
        }

        // 显示图像
        cv::imshow("Color Picker", display);

        // 设置鼠标回调（记录当前帧的数据）
        userdata_ = {&hsv, &red_samples_, &blue_samples_};
        cv::setMouseCallback("Color Picker", onMouse, &userdata_);

        // 按键处理
        int key = cv::waitKey(1);
        if (key == 27) {  // ESC 退出
            rclcpp::shutdown();
        } else if (key == 'm' || key == 'M') {
            show_mask_ = !show_mask_;
            RCLCPP_INFO(this->get_logger(), "切换掩膜显示: %s", show_mask_ ? "ON" : "OFF");
        } else if (key == 'r' || key == 'R') {
            red_samples_.clear();
            RCLCPP_INFO(this->get_logger(), "红色样本已清空");
        } else if (key == 'b' || key == 'B') {
            blue_samples_.clear();
            RCLCPP_INFO(this->get_logger(), "蓝色样本已清空");
        }
    }

    // 静态鼠标回调
    static void onMouse(int event, int x, int y, int, void* userdata)
    {
        if (event != cv::EVENT_LBUTTONDOWN) return;
        auto* data = (UserData*)userdata;
        if (!data || !data->hsv || data->hsv->empty()) return;
        if (x < 0 || x >= data->hsv->cols || y < 0 || y >= data->hsv->rows) return;

        cv::Vec3b hsv_val = data->hsv->at<cv::Vec3b>(y, x);
        printf("坐标(%d,%d)  BGR:(-,-,-)  HSV:(%d,%d,%d)\n",
               x, y, hsv_val[0], hsv_val[1], hsv_val[2]);

        // 根据色相粗略区分红/蓝，存入对应列表
        int h = hsv_val[0];
        if ((h >= 0 && h <= 20) || (h >= 150 && h <= 180)) {
            data->red_samples->push_back(hsv_val);
            printf("  -> 已加入红色样本 (共 %zu 个)\n", data->red_samples->size());
        } else if (h >= 90 && h <= 140) {
            data->blue_samples->push_back(hsv_val);
            printf("  -> 已加入蓝色样本 (共 %zu 个)\n", data->blue_samples->size());
        } else {
            printf("  -> 色调不明确，未记录\n");
        }
    }

    // 根据样本生成掩膜叠加图
    cv::Mat generateMaskOverlay(const cv::Mat& bgr, const cv::Mat& hsv) const
{
    cv::Mat mask_red, mask_blue;
    if (!red_samples_.empty()) {
        mask_red = createMaskFromSamples(hsv, red_samples_, true);
    } else {
        mask_red = cv::Mat::zeros(hsv.size(), CV_8UC1);
    }
    if (!blue_samples_.empty()) {
        mask_blue = createMaskFromSamples(hsv, blue_samples_, false);
    } else {
        mask_blue = cv::Mat::zeros(hsv.size(), CV_8UC1);
    }

    // 显示原始掩膜（黑白）
    cv::imshow("Red Mask", mask_red);
    cv::imshow("Blue Mask", mask_blue);

    // 叠加显示
    cv::Mat overlay = bgr.clone();
    overlay.setTo(cv::Scalar(0, 0, 255), mask_red);
    overlay.setTo(cv::Scalar(255, 0, 0), mask_blue);
    cv::addWeighted(bgr, 0.6, overlay, 0.4, 0, overlay);
    return overlay;
}

    // 根据样本列表创建掩膜（扩展一定余量）
    cv::Mat createMaskFromSamples(const cv::Mat& hsv, const std::vector<cv::Vec3b>& samples, bool is_red) const
    {
        if (samples.empty()) return cv::Mat::zeros(hsv.size(), CV_8UC1);

        int h_min = 255, h_max = 0, s_min = 255, s_max = 0, v_min = 255, v_max = 0;
        for (const auto& s : samples) {
            h_min = std::min(h_min, (int)s[0]); h_max = std::max(h_max, (int)s[0]);
            s_min = std::min(s_min, (int)s[1]); s_max = std::max(s_max, (int)s[1]);
            v_min = std::min(v_min, (int)s[2]); v_max = std::max(v_max, (int)s[2]);
        }

        // 扩展余量
        int h_margin = is_red ? 5 : 10; // 红色范围窄，蓝色稍宽
        int s_margin = 20;
        int v_margin = 20;

        int h_low = std::max(0, h_min - h_margin);
        int h_high = std::min(180, h_max + h_margin);
        int s_low = std::max(0, s_min - s_margin);
        int s_high = std::min(255, s_max + s_margin);
        int v_low = std::max(0, v_min - v_margin);
        int v_high = std::min(255, v_max + v_margin);

        cv::Mat mask;
        if (is_red) {
            // 红色可能跨两段，需要特殊处理
            cv::Mat mask1, mask2;
            if (h_low <= h_high) {
                cv::inRange(hsv, cv::Scalar(h_low, s_low, v_low), cv::Scalar(h_high, s_high, v_high), mask1);
            }
            // 如果样本包含 >150，则添加第二段
            if (h_max >= 150 || h_low >= 150) {
                int h_low2 = std::max(150, h_low);
                int h_high2 = std::min(180, h_high);
                cv::inRange(hsv, cv::Scalar(h_low2, s_low, v_low), cv::Scalar(h_high2, s_high, v_high), mask2);
                cv::bitwise_or(mask1, mask2, mask);
            } else {
                mask = mask1;
            }
        } else {
            cv::inRange(hsv, cv::Scalar(h_low, s_low, v_low), cv::Scalar(h_high, s_high, v_high), mask);
        }
        return mask;
    }

    // 传递给鼠标回调的数据结构
    struct UserData {
        cv::Mat* hsv = nullptr;
        std::vector<cv::Vec3b>* red_samples = nullptr;
        std::vector<cv::Vec3b>* blue_samples = nullptr;
    };

    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr sub_;
    bool show_mask_;
    std::vector<cv::Vec3b> red_samples_;
    std::vector<cv::Vec3b> blue_samples_;
    UserData userdata_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<ColorPicker>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}