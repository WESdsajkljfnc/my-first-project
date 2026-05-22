// ============================================================
// armor_detector.cpp — 装甲板检测类实现
// 功能：颜色分离、形态学处理、灯条提取、灯条配对、装甲板画框
// 支持 IMAGE（图片）和 VIDEO（视频）两种模式
// ============================================================

#include "armor_detect/armor_detector.h"
#include <algorithm>
#include <cmath>

using namespace cv;
using namespace std;

// ============================================================
// 构造函数：初始化形态学内核
// 根据模式（IMAGE/VIDEO）设置不同的形态学核大小
// ============================================================
ArmorDetector::ArmorDetector(Mode mode) : mode_(mode) {
    // 红色灯条用较大的核（灯条大，噪声多）
    kernel_red_open_  = getStructuringElement(MORPH_ELLIPSE, Size(1, 1));   // 开运算：5x5 椭圆核去噪
    kernel_red_close_ = getStructuringElement(MORPH_RECT, Size(30, 30));     // 闭运算：9x9 矩形核填空//Red_3000中有掩膜中间断开，改大闭运算核

    // 蓝色灯条用较小的核（灯条小，避免被腐蚀掉）
    kernel_blue_open_  = getStructuringElement(MORPH_ELLIPSE, Size(2, 2));  // 开运算：2x2 椭圆核轻去噪
    kernel_blue_close_ = getStructuringElement(MORPH_RECT, Size(5, 5));    // 闭运算：5x5 矩形核填空
}

// ============================================================
// 切换检测模式（IMAGE 或 VIDEO）
// ============================================================
void ArmorDetector::setMode(Mode mode) { mode_ = mode; }

// ============================================================
// 颜色分离：将 HSV 图像按红/蓝阈值分割为二值掩膜
// IMAGE 模式：针对 armor.jpg 调优的参数
// VIDEO 模式：针对视频素材取色后调优的参数
// ============================================================
void ArmorDetector::colorSplit(const Mat& hsv) {
    Mat mask_red1, mask_red2;

    if (mode_ == IMAGE) {
        // ====== 图片模式 ======
        // 红色分为两段：H [0,20] 和 [150,180]
        inRange(hsv, Scalar(0, 50, 50), Scalar(20, 255, 255), mask_red1);       // 红色第一段
        inRange(hsv, Scalar(150, 210, 50), Scalar(180, 255, 255), mask_red2);   // 红色第二段
        bitwise_or(mask_red1, mask_red2, red_mask_);                             // 合并两段

        // 蓝色：H [90,110], S [45,205], V [170,255]
        inRange(hsv, Scalar(90, 45, 170), Scalar(110, 205, 255), blue_mask_);

    } else {
        // ====== 视频模式 ======
        // 红色：H [0,20] 和 [150,180]，基于实测取色放宽
        inRange(hsv, Scalar(0, 50, 50), Scalar(20, 255, 255), mask_red1);       // H: 0~20
        inRange(hsv, Scalar(150, 210, 150), Scalar(180, 255, 255), mask_red2); // H: 150~180, V≥150
        bitwise_or(mask_red1, mask_red2, red_mask_);

        // 蓝色：H [90,120]，降低 S 下限到 20，覆盖更多蓝色区域
        inRange(hsv, Scalar(90, 20, 170), Scalar(120, 255, 255), blue_mask_);
    }
}

// ============================================================
// 形态学处理：对掩膜进行开闭运算，去噪 + 填空洞
// ============================================================
void ArmorDetector::morphologyProcess() {
    // 开运算（先腐蚀后膨胀）：去除小白点噪声
    // 闭运算（先膨胀后腐蚀）：填补内部小黑洞
    morphologyEx(red_mask_,  red_mask_,  MORPH_OPEN, kernel_red_open_);
    morphologyEx(red_mask_,  red_mask_,  MORPH_CLOSE, kernel_red_close_);
    morphologyEx(blue_mask_, blue_mask_, MORPH_OPEN, kernel_blue_open_);
    morphologyEx(blue_mask_, blue_mask_, MORPH_CLOSE, kernel_blue_close_);
}

// ============================================================
// 灯条提取：对掩膜找轮廓，按几何特征筛选灯条候选
// IMAGE 模式：红蓝大小不同，分开设置参数
// VIDEO 模式：红蓝差不多大，统一参数
// ============================================================
vector<LightBar> ArmorDetector::extractBars(const Mat& mask, const string& color) {
    float min_area, min_length, min_ratio, max_ratio, min_width, angle_tol;

    if (mode_ == IMAGE) {
        // ====== 图片模式：红蓝灯条大小不同 ======
        if (color == "Red") {
            min_area=50;   min_length=10;  // 红色灯条：大面积、长边 ≥10
            min_ratio=2;   max_ratio=8;    // 长宽比 2~8
            min_width=0;   angle_tol=10;   // 不限制宽度，角度偏离竖直线 ≤10°
        } else {
            min_area=10;   min_length=5;   // 蓝色灯条：小面积、短边 ≥5
            min_ratio=2;   max_ratio=6;    // 长宽比 2~6
            min_width=4;   angle_tol=10;   // 最小宽度 4，过滤窄噪声
        }
    } else {
        // ====== 视频模式：红蓝差不多大，统一参数 ======
        min_area=50;   min_length=10;     // 最小面积 50，最短边 10
        min_ratio=2;   max_ratio=8;       // 长宽比 2~8
        min_width=0;   angle_tol=10;      // 不限制宽度，角度偏离竖直线 ≤10°
    }

    // 找轮廓（RETR_EXTERNAL 只取最外层轮廓）
    vector<vector<Point>> contours;
    findContours(mask.clone(), contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    vector<LightBar> bars;

    for (const auto& c : contours) {
        float area = contourArea(c);                        // 轮廓面积
        if (area < min_area) continue;                      // 面积过滤

        RotatedRect rect = minAreaRect(c);                  // 最小外接旋转矩形
        float w = rect.size.width, h = rect.size.height;
        float longSide = max(w,h), shortSide = min(w,h);    // 长边、短边
        if (longSide < min_length) continue;                // 长度过滤

        float ratio = longSide/shortSide;                   // 长宽比
        if (ratio < min_ratio || ratio > max_ratio) continue; // 长宽比过滤

        if (shortSide < min_width) continue;                // 宽度过滤

        // 角度计算：调整为长边与水平轴的夹角，范围 [-90, 90)
        float angle = rect.angle;
        if (w < h) angle += 90;                             // OpenCV 默认返回宽边角度，校正为长边
        if (angle < -90) angle += 180;
        if (angle >= 90) angle -= 180;

        // 角度过滤：只保留接近竖直（±10°）的灯条
        if (abs(abs(angle)-90) > angle_tol) continue;

        // 保存灯条
        bars.push_back({rect, longSide, shortSide, angle, area, rect.center});
    }
    return bars;
}

// ============================================================
// 灯条配对判断：检查两个灯条是否属于同一个装甲板
// 条件：角度差小 + 长度接近 + 中心距离适中
// ============================================================
bool ArmorDetector::canPair(const LightBar& a, const LightBar& b) {
    // 1. 角度差 ≤15°（考虑角度循环 0°=180°）
    float angle_diff = abs(a.angle - b.angle);
    if (angle_diff > 180) angle_diff = 360 - angle_diff;
    if (angle_diff > 15) return false;

    // 2. 长度比 ≤1.8
    float len_ratio = max(a.length, b.length) / min(a.length, b.length);
    if (len_ratio > 1.8) return false;

    // 3. 中心距离：0.3~5 倍平均灯条长度
    float dist = norm(a.center - b.center);
    float avg_len = (a.length + b.length) / 2;
    if (dist < avg_len * 0.3 || dist > avg_len * 5) return false;

    return true;
}

// ============================================================
// 角点排序：将四个角点排序为 左上、右上、右下、左下
// ============================================================
void ArmorDetector::sortCorners(vector<Point2f>& corners) {
    // 按 x 坐标排序，前两个是左侧点，后两个是右侧点
    sort(corners.begin(), corners.end(),
         [](const Point2f& a, const Point2f& b) { return a.x < b.x; });

    // 左侧两点按 y 排序：左上(y小)、左下(y大)
    if (corners[0].y > corners[1].y) swap(corners[0], corners[1]);

    // 右侧两点按 y 排序：右上(y小)、右下(y大)
    if (corners[2].y > corners[3].y) swap(corners[2], corners[3]);

    // 最终顺序：左上[0]、右上[2]、右下[3]、左下[1]
    corners = {corners[0], corners[2], corners[3], corners[1]};
}

// ============================================================
// 主检测接口：输入 BGR 图像，返回装甲板检测结果列表
// 流程：颜色分离 → 形态学 → 灯条提取 → 配对 → 画框
// ============================================================
vector<ArmorResult> ArmorDetector::detect(const Mat& frame) {
    vector<ArmorResult> results;                        // 检测结果列表
    Mat hsv;
    cvtColor(frame, hsv, COLOR_BGR2HSV);               // BGR → HSV

    colorSplit(hsv);                                    // 步骤1：颜色分离
    morphologyProcess();                                // 步骤2：形态学处理

    // 步骤3：提取灯条
    vector<LightBar> redBars  = extractBars(red_mask_, "Red");
    vector<LightBar> blueBars = extractBars(blue_mask_, "Blue");

    // ---- 调试：绘制灯条候选 ----
    red_bars_ = frame.clone();
    for (auto& bar : redBars) {
        Point2f pts[4]; bar.rect.points(pts);
        for (int i=0;i<4;i++) line(red_bars_, pts[i], pts[(i+1)%4], Scalar(0,255,0), 2);
    }
    blue_bars_ = frame.clone();
    for (auto& bar : blueBars) {
        Point2f pts[4]; bar.rect.points(pts);
        for (int i=0;i<4;i++) line(blue_bars_, pts[i], pts[(i+1)%4], Scalar(255,0,0), 2);
    }

    // ---- 步骤4+5：配对并绘制装甲板框 ----
    debug_frame_ = frame.clone();

    // 通用配对+绘制函数（红蓝共用）
    auto processColor = [&](vector<LightBar>& bars, const string& color, Scalar clr) {
        for (size_t i=0; i<bars.size(); i++) {
            for (size_t j=i+1; j<bars.size(); j++) {
                if (!canPair(bars[i], bars[j])) continue;   // 配对判断

                // 区分左右灯条（按中心 x 坐标）
                LightBar* left  = (bars[i].center.x < bars[j].center.x) ? &bars[i] : &bars[j];
                LightBar* right = (bars[i].center.x < bars[j].center.x) ? &bars[j] : &bars[i];

                // 取灯条的四顶点
                Point2f lpts[4], rpts[4];
                left->rect.points(lpts);
                right->rect.points(rpts);

                // 左灯条取 x 最小的两个点 → 左上、左下
                sort(lpts, lpts+4, [](Point2f& a, Point2f& b){return a.x<b.x;});
                Point2f tl=lpts[0], bl=lpts[1];
                if(tl.y>bl.y) swap(tl,bl);      // y 小的为左上

                // 右灯条取 x 最大的两个点 → 右上、右下
                sort(rpts, rpts+4, [](Point2f& a, Point2f& b){return a.x>b.x;});
                Point2f tr=rpts[0], br=rpts[1];
                if(tr.y>br.y) swap(tr,br);      // y 小的为右上

                // 四个角点 + 排序
                vector<Point2f> corners = {tl, tr, br, bl};
                sortCorners(corners);               // 左上、右上、右下、左下

                // 上下方向扩展 30%，让框比灯条稍大
                Point2f center = (corners[0]+corners[1]+corners[2]+corners[3])*0.25;
                for(auto& p : corners) p.y += (p.y-center.y)*0.3;

                // 绘制装甲板框
                for(int k=0;k<4;k++)
                    line(debug_frame_, corners[k], corners[(k+1)%4], clr, 2);
                // 标注颜色标签
                putText(debug_frame_, color, corners[0]+Point2f(10,-10),
                        FONT_HERSHEY_SIMPLEX, 0.7, clr, 2);

                // 保存结果
                results.push_back({color, corners});
            }
        }
    };

    processColor(redBars,  "Red",  Scalar(0,0,255));      // 红色框
    processColor(blueBars, "Blue", Scalar(255,0,0));       // 蓝色框

    return results;
}
