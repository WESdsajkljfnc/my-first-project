#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>
#include "MvCameraControl.h"

class HikCameraNode : public rclcpp::Node
{
public:
    HikCameraNode() : Node("hik_camera_node")
    {
        this->declare_parameter<int>("width", 1280);
        this->declare_parameter<int>("height", 1024);
        this->declare_parameter<int>("exposure_time", 5000); // 50ms
        this->declare_parameter<float>("gain", 5.0);
        this->declare_parameter<std::string>("frame_id", "camera_optical_frame");

        width_ = this->get_parameter("width").as_int();
        height_ = this->get_parameter("height").as_int();
        frame_id_ = this->get_parameter("frame_id").as_string();

        pub_ = this->create_publisher<sensor_msgs::msg::Image>("image_raw", 10);

        if (!initCamera()) {
            RCLCPP_ERROR(this->get_logger(), "相机初始化失败！");
            rclcpp::shutdown();
            return;
        }

        // 使用 10Hz (100ms) 防止图像队列堆积
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(100),
            std::bind(&HikCameraNode::timerCallback, this));
    }

    ~HikCameraNode()
    {
        if (handle_) {
            MV_CC_StopGrabbing(handle_);
            MV_CC_CloseDevice(handle_);
            MV_CC_DestroyHandle(handle_);
        }
    }

private:
    bool initCamera()
    {
        MV_CC_DEVICE_INFO_LIST stDeviceList;
        memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
        int nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &stDeviceList);
        if (nRet != MV_OK || stDeviceList.nDeviceNum == 0) {
            RCLCPP_ERROR(this->get_logger(), "未发现相机！");
            return false;
        }
        RCLCPP_INFO(this->get_logger(), "发现 %d 台相机", stDeviceList.nDeviceNum);

        nRet = MV_CC_CreateHandle(&handle_, stDeviceList.pDeviceInfo[0]);
        if (nRet != MV_OK) { RCLCPP_ERROR(this->get_logger(), "创建句柄失败"); return false; }

        nRet = MV_CC_OpenDevice(handle_);
        if (nRet != MV_OK) { RCLCPP_ERROR(this->get_logger(), "打开设备失败"); return false; }

        MV_CC_SetEnumValue(handle_, "TriggerMode", MV_TRIGGER_MODE_OFF);
        MV_CC_SetFloatValue(handle_, "ExposureTime", 500.0);
        MV_CC_SetFloatValue(handle_, "Gain", 0.0);

        nRet = MV_CC_StartGrabbing(handle_);
        if (nRet != MV_OK) { RCLCPP_ERROR(this->get_logger(), "开始取流失败"); return false; }

        RCLCPP_INFO(this->get_logger(), "相机初始化成功");
        return true;
    }

    void timerCallback()
    {
        MV_FRAME_OUT stImageInfo;
        memset(&stImageInfo, 0, sizeof(MV_FRAME_OUT));
        int nRet = MV_CC_GetImageBuffer(handle_, &stImageInfo, 2000);
        if (nRet != MV_OK) {
            RCLCPP_WARN(this->get_logger(), "获取图像超时");
            return;
        }

        // ---------- 使用 SDK 统一转换为 BGR8 ----------
        MV_CC_PIXEL_CONVERT_PARAM stConvertParam;
        memset(&stConvertParam, 0, sizeof(MV_CC_PIXEL_CONVERT_PARAM));
        stConvertParam.nWidth      = stImageInfo.stFrameInfo.nWidth;
        stConvertParam.nHeight     = stImageInfo.stFrameInfo.nHeight;
        stConvertParam.enSrcPixelType = stImageInfo.stFrameInfo.enPixelType;
        stConvertParam.pSrcData    = stImageInfo.pBufAddr;
        stConvertParam.nSrcDataLen = stImageInfo.stFrameInfo.nFrameLen;
        stConvertParam.enDstPixelType = PixelType_Gvsp_BGR8_Packed;
        stConvertParam.pDstBuffer  = NULL;
        stConvertParam.nDstBufferSize = stConvertParam.nWidth * stConvertParam.nHeight * 3;

        unsigned char* pDstData = new unsigned char[stConvertParam.nDstBufferSize];
        stConvertParam.pDstBuffer = pDstData;

        nRet = MV_CC_ConvertPixelType(handle_, &stConvertParam);
        if (nRet != MV_OK) {
            RCLCPP_WARN(this->get_logger(), "像素格式转换失败，错误码: %d", nRet);
            delete[] pDstData;
            MV_CC_FreeImageBuffer(handle_, &stImageInfo);  // 释放原图缓冲
            return;
        }

        cv::Mat frame(stConvertParam.nHeight, stConvertParam.nWidth, CV_8UC3, pDstData);

        auto msg = cv_bridge::CvImage(
            std_msgs::msg::Header(),
            sensor_msgs::image_encodings::BGR8,
            frame).toImageMsg();
        msg->header.frame_id = frame_id_;
        msg->header.stamp = this->now();
        pub_->publish(*msg);

        delete[] pDstData;
        MV_CC_FreeImageBuffer(handle_, &stImageInfo);
    }

    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr pub_;
    rclcpp::TimerBase::SharedPtr timer_;
    void* handle_ = nullptr;
    int width_, height_;
    std::string frame_id_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<HikCameraNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
