#include "armor_detect_ros/armor_detector.h"
#include <algorithm>
#include <cmath>

using namespace cv;
using namespace std;

ArmorDetector::ArmorDetector() {
    // 颜色阈值默认值
    red_low1_  = Scalar(0, 50, 50);
    red_high1_ = Scalar(20, 255, 255);
    red_low2_  = Scalar(150, 210, 50);
    red_high2_ = Scalar(180, 255, 255);
    blue_low_  = Scalar(90, 45, 170);
    blue_high_ = Scalar(110, 205, 255);

    // 形态学核默认大小
    kernel_red_open_  = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
    kernel_red_close_ = getStructuringElement(MORPH_RECT, Size(30, 30));
    kernel_blue_open_  = getStructuringElement(MORPH_ELLIPSE, Size(2, 2));
    kernel_blue_close_ = getStructuringElement(MORPH_RECT, Size(5, 5));

    // 筛选默认参数
    min_area_    = 50.0f;
    min_length_  = 10.0f;
    min_ratio_   = 2.0f;
    max_ratio_   = 8.0f;
    angle_tol_   = 10.0f;

    // 配对默认参数
    max_angle_diff_  = 15.0f;
    max_len_ratio_   = 1.8f;
    min_dist_factor_ = 0.3f;
    max_dist_factor_ = 5.0f;

    // 扩展比例
    extend_ratio_ = 0.3f;
}

// Setter 实现
void ArmorDetector::setRedThreshold1(int hl, int sl, int vl, int hh, int sh, int vh) {
    red_low1_  = Scalar(hl, sl, vl);
    red_high1_ = Scalar(hh, sh, vh);
}
void ArmorDetector::setRedThreshold2(int hl, int sl, int vl, int hh, int sh, int vh) {
    red_low2_  = Scalar(hl, sl, vl);
    red_high2_ = Scalar(hh, sh, vh);
}
void ArmorDetector::setBlueThreshold(int hl, int sl, int vl, int hh, int sh, int vh) {
    blue_low_  = Scalar(hl, sl, vl);
    blue_high_ = Scalar(hh, sh, vh);
}

void ArmorDetector::setMorphKernels(int r_open, int r_close, int b_open, int b_close) {
    kernel_red_open_  = getStructuringElement(MORPH_ELLIPSE, Size(r_open, r_open));
    kernel_red_close_ = getStructuringElement(MORPH_RECT, Size(r_close, r_close));
    kernel_blue_open_  = getStructuringElement(MORPH_ELLIPSE, Size(b_open, b_open));
    kernel_blue_close_ = getStructuringElement(MORPH_RECT, Size(b_close, b_close));
}

void ArmorDetector::setFilterParams(float min_area, float min_length, float min_ratio, float max_ratio, float angle_tol) {
    min_area_   = min_area;
    min_length_ = min_length;
    min_ratio_  = min_ratio;
    max_ratio_  = max_ratio;
    angle_tol_  = angle_tol;
}

void ArmorDetector::setPairParams(float max_angle_diff, float max_len_ratio, float min_dist_factor, float max_dist_factor) {
    max_angle_diff_  = max_angle_diff;
    max_len_ratio_   = max_len_ratio;
    min_dist_factor_ = min_dist_factor;
    max_dist_factor_ = max_dist_factor;
}

void ArmorDetector::setExtendRatio(float ratio) {
    extend_ratio_ = ratio;
}

// 颜色分离
void ArmorDetector::colorSplit(const Mat& hsv) {
    Mat mask_red1, mask_red2;
    inRange(hsv, red_low1_, red_high1_, mask_red1);
    inRange(hsv, red_low2_, red_high2_, mask_red2);
    bitwise_or(mask_red1, mask_red2, red_mask_);
    inRange(hsv, blue_low_, blue_high_, blue_mask_);
}

// 形态学
void ArmorDetector::morphologyProcess() {
    morphologyEx(red_mask_, red_mask_, MORPH_OPEN, kernel_red_open_);
    morphologyEx(red_mask_, red_mask_, MORPH_CLOSE, kernel_red_close_);
    morphologyEx(blue_mask_, blue_mask_, MORPH_OPEN, kernel_blue_open_);
    morphologyEx(blue_mask_, blue_mask_, MORPH_CLOSE, kernel_blue_close_);
}

// 灯条提取（红蓝共用）
vector<LightBar> ArmorDetector::extractBars(const Mat& mask, const string& /*color*/) {
    vector<vector<Point>> contours;
    findContours(mask.clone(), contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    vector<LightBar> bars;
    for (const auto& c : contours) {
        float area = contourArea(c);
        if (area < min_area_) continue;
        RotatedRect rect = minAreaRect(c);
        float w = rect.size.width, h = rect.size.height;
        float longSide = max(w, h), shortSide = min(w, h);
        if (longSide < min_length_) continue;
        float ratio = longSide / shortSide;
        if (ratio < min_ratio_ || ratio > max_ratio_) continue;
        float angle = rect.angle;
        if (w < h) angle += 90;
        if (angle < -90) angle += 180;
        if (angle >= 90) angle -= 180;
        if (abs(abs(angle) - 90) > angle_tol_) continue;
        bars.push_back({rect, longSide, shortSide, angle, area, rect.center});
    }
    return bars;
}

// 配对判断
bool ArmorDetector::canPair(const LightBar& a, const LightBar& b) {
    float angle_diff = abs(a.angle - b.angle);
    if (angle_diff > 180) angle_diff = 360 - angle_diff;
    if (angle_diff > max_angle_diff_) return false;
    float len_ratio = max(a.length, b.length) / min(a.length, b.length);
    if (len_ratio > max_len_ratio_) return false;
    float dist = norm(a.center - b.center);
    float avg_len = (a.length + b.length) / 2;
    if (dist < avg_len * min_dist_factor_ || dist > avg_len * max_dist_factor_) return false;
    return true;
}

// 角点排序
void ArmorDetector::sortCorners(vector<Point2f>& corners) {
    sort(corners.begin(), corners.end(),
         [](const Point2f& a, const Point2f& b) { return a.x < b.x; });
    if (corners[0].y > corners[1].y) swap(corners[0], corners[1]);
    if (corners[2].y > corners[3].y) swap(corners[2], corners[3]);
    corners = {corners[0], corners[2], corners[3], corners[1]};
}

// 主检测接口
vector<ArmorResult> ArmorDetector::detect(const Mat& frame) {
    vector<ArmorResult> results;
    Mat hsv;
    cvtColor(frame, hsv, COLOR_BGR2HSV);

    colorSplit(hsv);
    morphologyProcess();

    vector<LightBar> redBars  = extractBars(red_mask_, "Red");
    vector<LightBar> blueBars = extractBars(blue_mask_, "Blue");

    // 调试：灯条候选
    red_bars_ = frame.clone();
    for (auto& bar : redBars) {
        Point2f pts[4]; bar.rect.points(pts);
        for (int i = 0; i < 4; i++) line(red_bars_, pts[i], pts[(i+1)%4], Scalar(0,255,0), 2);
    }
    blue_bars_ = frame.clone();
    for (auto& bar : blueBars) {
        Point2f pts[4]; bar.rect.points(pts);
        for (int i = 0; i < 4; i++) line(blue_bars_, pts[i], pts[(i+1)%4], Scalar(255,0,0), 2);
    }

    debug_frame_ = frame.clone();

    auto processColor = [&](vector<LightBar>& bars, const string& color, Scalar clr) {
        for (size_t i = 0; i < bars.size(); i++) {
            for (size_t j = i + 1; j < bars.size(); j++) {
                if (!canPair(bars[i], bars[j])) continue;

                LightBar* left  = (bars[i].center.x < bars[j].center.x) ? &bars[i] : &bars[j];
                LightBar* right = (bars[i].center.x < bars[j].center.x) ? &bars[j] : &bars[i];

                Point2f lpts[4], rpts[4];
                left->rect.points(lpts);
                right->rect.points(rpts);

                sort(lpts, lpts+4, [](Point2f& a, Point2f& b){return a.x < b.x;});
                Point2f tl = lpts[0], bl = lpts[1];
                if (tl.y > bl.y) swap(tl, bl);

                sort(rpts, rpts+4, [](Point2f& a, Point2f& b){return a.x > b.x;});
                Point2f tr = rpts[0], br = rpts[1];
                if (tr.y > br.y) swap(tr, br);

                vector<Point2f> corners = {tl, tr, br, bl};
                sortCorners(corners);

                Point2f center = (corners[0] + corners[1] + corners[2] + corners[3]) * 0.25;
                for (auto& p : corners) p.y += (p.y - center.y) * extend_ratio_;

                for (int k = 0; k < 4; k++)
                    line(debug_frame_, corners[k], corners[(k+1)%4], clr, 2);
                putText(debug_frame_, color, corners[0] + Point2f(10, -10),
                        FONT_HERSHEY_SIMPLEX, 0.7, clr, 2);

                results.push_back({color, corners});
            }
        }
    };

    processColor(redBars,  "Red",  Scalar(0,0,255));
    processColor(blueBars, "Blue", Scalar(255,0,0));
    return results;
}
