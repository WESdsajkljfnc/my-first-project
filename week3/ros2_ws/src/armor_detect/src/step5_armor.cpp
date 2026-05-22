#include "armor_detect/armor_detector.h"
#include <algorithm>
#include <cmath>

using namespace cv;
using namespace std;

ArmorDetector::ArmorDetector(Mode mode) : mode_(mode) {
    kernel_red_open_  = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
    kernel_red_close_ = getStructuringElement(MORPH_RECT, Size(9, 9));
    kernel_blue_open_  = getStructuringElement(MORPH_ELLIPSE, Size(2, 2));
    kernel_blue_close_ = getStructuringElement(MORPH_RECT, Size(5, 5));
}

void ArmorDetector::setMode(Mode mode) {
    mode_ = mode;
}

void ArmorDetector::colorSplit(const Mat& hsv) {
    Mat mask_red1, mask_red2;

    if (mode_ == IMAGE) {
        // === 图片模式（你验证过的参数，不动） ===
        inRange(hsv, Scalar(0, 50, 50), Scalar(20, 255, 255), mask_red1);
        inRange(hsv, Scalar(150, 210, 50), Scalar(180, 255, 255), mask_red2);
        bitwise_or(mask_red1, mask_red2, red_mask_);
        inRange(hsv, Scalar(90, 45, 170), Scalar(110, 205, 255), blue_mask_);
    } else {
        // === 视频模式（基于 Blue_1000 取色） ===
        // 红色：先用图片参数跑，等你取了红色 HSV 后再微调
        inRange(hsv, Scalar(0, 50, 50), Scalar(20, 255, 255), mask_red1);
        inRange(hsv, Scalar(150, 210, 50), Scalar(180, 255, 255), mask_red2);
        bitwise_or(mask_red1, mask_red2, red_mask_);
        // 蓝色：基于取色数据 (H:104~113, S:214~255, V:173~255)
        inRange(hsv, Scalar(100, 200, 100), Scalar(115, 255, 255), blue_mask_);
    }
}
void ArmorDetector::morphologyProcess() {
    morphologyEx(red_mask_, red_mask_, MORPH_OPEN, kernel_red_open_);
    morphologyEx(red_mask_, red_mask_, MORPH_CLOSE, kernel_red_close_);
    morphologyEx(blue_mask_, blue_mask_, MORPH_OPEN, kernel_blue_open_);
    morphologyEx(blue_mask_, blue_mask_, MORPH_CLOSE, kernel_blue_close_);
}

vector<LightBar> ArmorDetector::extractBars(const Mat& mask, const string& color) {
    // 与 step4_lightbars 完全一致的筛选参数
    float min_area, min_length, min_ratio, max_ratio, min_width, angle_tol;
    if (color == "Red") {
        min_area=50; min_length=10; min_ratio=2; max_ratio=8; min_width=0; angle_tol=10;
    } else {
        min_area=10; min_length=5; min_ratio=2; max_ratio=6; min_width=4; angle_tol=10;
    }

    vector<vector<Point>> contours;
    findContours(mask.clone(), contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    vector<LightBar> bars;

    for (const auto& c : contours) {
        float area = contourArea(c);
        if (area < min_area) continue;
        RotatedRect rect = minAreaRect(c);
        float w = rect.size.width, h = rect.size.height;
        float longSide = max(w,h), shortSide = min(w,h);
        if (longSide < min_length) continue;
        float ratio = longSide/shortSide;
        if (ratio < min_ratio || ratio > max_ratio) continue;
        if (shortSide < min_width) continue;
        float angle = rect.angle;
        if (w < h) angle += 90;
        if (angle < -90) angle += 180;
        if (angle >= 90) angle -= 180;
        if (abs(abs(angle)-90) > angle_tol) continue;
        bars.push_back({rect, longSide, shortSide, angle, area, rect.center});
    }
    return bars;
}

bool ArmorDetector::canPair(const LightBar& a, const LightBar& b) {
    float angle_diff = abs(a.angle - b.angle);
    if (angle_diff > 180) angle_diff = 360 - angle_diff;
    if (angle_diff > 15) return false;
    float len_ratio = max(a.length, b.length) / min(a.length, b.length);
    if (len_ratio > 1.8) return false;
    float dist = norm(a.center - b.center);
    float avg_len = (a.length + b.length) / 2;
    if (dist < avg_len * 0.3 || dist > avg_len * 5) return false;
    return true;
}

void ArmorDetector::sortCorners(vector<Point2f>& corners) {
    sort(corners.begin(), corners.end(),
         [](const Point2f& a, const Point2f& b) { return a.x < b.x; });
    if (corners[0].y > corners[1].y) swap(corners[0], corners[1]);
    if (corners[2].y > corners[3].y) swap(corners[2], corners[3]);
    corners = {corners[0], corners[2], corners[3], corners[1]};
}

vector<ArmorResult> ArmorDetector::detect(const Mat& frame) {
    vector<ArmorResult> results;
    Mat hsv;
    cvtColor(frame, hsv, COLOR_BGR2HSV);

    colorSplit(hsv);
    morphologyProcess();

    vector<LightBar> redBars  = extractBars(red_mask_, "Red");
    vector<LightBar> blueBars = extractBars(blue_mask_, "Blue");

    // 调试：灯条绘制
    red_bars_ = frame.clone();
    for (auto& bar : redBars) {
        Point2f pts[4]; bar.rect.points(pts);
        for (int i=0;i<4;i++) line(red_bars_, pts[i], pts[(i+1)%4], Scalar(0,255,0), 2);
        circle(red_bars_, bar.center, 3, Scalar(0,255,255), -1);
    }
    blue_bars_ = frame.clone();
    for (auto& bar : blueBars) {
        Point2f pts[4]; bar.rect.points(pts);
        for (int i=0;i<4;i++) line(blue_bars_, pts[i], pts[(i+1)%4], Scalar(255,0,0), 2);
        circle(blue_bars_, bar.center, 3, Scalar(0,255,255), -1);
    }

    debug_frame_ = frame.clone();

    auto processColor = [&](vector<LightBar>& bars, const string& color, Scalar clr) {
        for (size_t i=0; i<bars.size(); i++) {
            for (size_t j=i+1; j<bars.size(); j++) {
                if (!canPair(bars[i], bars[j])) continue;
                LightBar* left  = (bars[i].center.x < bars[j].center.x) ? &bars[i] : &bars[j];
                LightBar* right = (bars[i].center.x < bars[j].center.x) ? &bars[j] : &bars[i];

                Point2f lpts[4], rpts[4];
                left->rect.points(lpts); right->rect.points(rpts);

                sort(lpts, lpts+4, [](Point2f& a, Point2f& b){return a.x<b.x;});
                Point2f tl=lpts[0], bl=lpts[1];
                if(tl.y>bl.y) swap(tl,bl);

                sort(rpts, rpts+4, [](Point2f& a, Point2f& b){return a.x>b.x;});
                Point2f tr=rpts[0], br=rpts[1];
                if(tr.y>br.y) swap(tr,br);

                vector<Point2f> corners = {tl, tr, br, bl};
                sortCorners(corners);

                // 上下扩展 30%
                Point2f center = (corners[0]+corners[1]+corners[2]+corners[3])*0.25;
                for(auto& p : corners) p.y += (p.y-center.y)*0.3;

                // 绘制
                for(int k=0;k<4;k++) line(debug_frame_, corners[k], corners[(k+1)%4], clr, 2);
                putText(debug_frame_, color, corners[0]+Point2f(10,-10), FONT_HERSHEY_SIMPLEX, 0.7, clr, 2);

                results.push_back({color, corners});
            }
        }
    };

    processColor(redBars, "Red", Scalar(0,0,255));
    processColor(blueBars, "Blue", Scalar(255,0,0));

    return results;
}
