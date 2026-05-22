#ifndef ARMOR_DETECTOR_H
#define ARMOR_DETECTOR_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

struct LightBar {
    cv::RotatedRect rect;
    float length, width, angle, area;
    cv::Point2f center;
};

struct ArmorResult {
    std::string color;
    std::vector<cv::Point2f> corners;
};

class ArmorDetector {
public:
    enum Mode { IMAGE, VIDEO };

    ArmorDetector(Mode mode = VIDEO);  // 默认视频模式

    void setMode(Mode mode);

    std::vector<ArmorResult> detect(const cv::Mat& frame);

    cv::Mat getRedMask()   const { return red_mask_; }
    cv::Mat getBlueMask()  const { return blue_mask_; }
    cv::Mat getRedBars()   const { return red_bars_; }
    cv::Mat getBlueBars()  const { return blue_bars_; }
    cv::Mat getDebugFrame() const { return debug_frame_; }

private:
    Mode mode_;
    void colorSplit(const cv::Mat& hsv);
    void morphologyProcess();
    std::vector<LightBar> extractBars(const cv::Mat& mask, const std::string& color);
    bool canPair(const LightBar& a, const LightBar& b);
    void sortCorners(std::vector<cv::Point2f>& corners);

    cv::Mat red_mask_, blue_mask_;
    cv::Mat red_bars_, blue_bars_, debug_frame_;
    cv::Mat kernel_red_open_, kernel_red_close_;
    cv::Mat kernel_blue_open_, kernel_blue_close_;
};

#endif
