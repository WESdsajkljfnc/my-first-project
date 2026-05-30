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
    ArmorDetector();   // 无参构造函数

    std::vector<ArmorResult> detect(const cv::Mat& frame);

    // 调试图像获取
    cv::Mat getRedMask()   const { return red_mask_; }
    cv::Mat getBlueMask()  const { return blue_mask_; }
    cv::Mat getRedBars()   const { return red_bars_; }
    cv::Mat getBlueBars()  const { return blue_bars_; }
    cv::Mat getDebugFrame() const { return debug_frame_; }

    // 参数设置
    void setRedThreshold1(int hl, int sl, int vl, int hh, int sh, int vh);
    void setRedThreshold2(int hl, int sl, int vl, int hh, int sh, int vh);
    void setBlueThreshold(int hl, int sl, int vl, int hh, int sh, int vh);
    void setMorphKernels(int r_open, int r_close, int b_open, int b_close);
    void setFilterParams(float min_area, float min_length, float min_ratio, float max_ratio, float angle_tol);
    void setPairParams(float max_angle_diff, float max_len_ratio, float min_dist_factor, float max_dist_factor);
    void setExtendRatio(float ratio);

private:
    void colorSplit(const cv::Mat& hsv);
    void morphologyProcess();
    std::vector<LightBar> extractBars(const cv::Mat& mask, const std::string& color);
    bool canPair(const LightBar& a, const LightBar& b);
    void sortCorners(std::vector<cv::Point2f>& corners);

    cv::Mat red_mask_, blue_mask_, red_bars_, blue_bars_, debug_frame_;
    cv::Mat kernel_red_open_, kernel_red_close_, kernel_blue_open_, kernel_blue_close_;

    cv::Scalar red_low1_, red_high1_, red_low2_, red_high2_, blue_low_, blue_high_;
    float min_area_, min_length_, min_ratio_, max_ratio_, angle_tol_;
    float max_angle_diff_, max_len_ratio_, min_dist_factor_, max_dist_factor_;
    float extend_ratio_;
};

#endif
