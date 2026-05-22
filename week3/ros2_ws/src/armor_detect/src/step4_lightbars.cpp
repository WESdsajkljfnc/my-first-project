#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <cstdlib>

using namespace cv;
using namespace std;

// 灯条结构体（为后续配对做准备）
struct LightBar {
    RotatedRect rect;   // 最小外接旋转矩形
    float length;       // 长边
    float width;        // 短边
    float angle;        // 长边与水平轴的角度（度）
    float area;         // 轮廓面积
    Point2f center;     // 中心点
};

int main() {
    string home = getenv("HOME");
    string assets = home + "/桌面/course/week3/Assets/";
    string image_path = assets + "armor.jpg";

    Mat src = imread(image_path);
    if (src.empty()) {
        cerr << "无法读取图片: " << image_path << endl;
        return -1;
    }

    // ========== 颜色分离（与之前完全相同） ==========
    Mat hsv;
    cvtColor(src, hsv, COLOR_BGR2HSV);

    Mat mask_red1, mask_red2, mask_red;
    inRange(hsv, Scalar(0, 50, 50), Scalar(20, 255, 255), mask_red1);
    inRange(hsv, Scalar(150, 210,50), Scalar(180, 255, 255), mask_red2);
    bitwise_or(mask_red1, mask_red2, mask_red);

    Mat mask_blue;
    inRange(hsv, Scalar(90,200,170), Scalar(120, 255, 255), mask_blue);

    // ========== 形态学处理 ==========
    // 红色灯条（较大，用稍大核）
    Mat kernel_red_open  = getStructuringElement(MORPH_ELLIPSE, Size(1, 1));
    Mat kernel_red_close = getStructuringElement(MORPH_RECT, Size(30, 30));
    morphologyEx(mask_red, mask_red, MORPH_OPEN, kernel_red_open);
    morphologyEx(mask_red, mask_red, MORPH_CLOSE, kernel_red_close);

    // 蓝色灯条（更小，用更小核避免把灯条腐蚀没了）
    Mat kernel_blue_open  = getStructuringElement(MORPH_ELLIPSE, Size(2, 2));
    Mat kernel_blue_close = getStructuringElement(MORPH_RECT, Size(5, 5));
    morphologyEx(mask_blue, mask_blue, MORPH_OPEN, kernel_blue_open);
    morphologyEx(mask_blue, mask_blue, MORPH_CLOSE, kernel_blue_close);

    // 保存最终的掩模（可选，作为中间结果）
    imwrite(assets + "mask_red.jpg", mask_red);
    imwrite(assets + "mask_blue.jpg", mask_blue);

    // ========== 第4步核心：找轮廓并筛选灯条 ==========
    // 处理红色和蓝色灯条的函数（lambda）
    auto processColor = [&](const Mat& mask, const string& colorName, Mat& drawImg) {
    // 根据颜色设置不同的筛选参数
    float min_area, min_length, min_ratio, max_ratio, angle_tol,min_width;//加入min_width
    if (colorName == "Red") {
        min_area   = 20.0f;
        min_length = 6.0f;
        min_ratio  = 1.5f;
        max_ratio  = 10.0f;
        min_width  = 0.0f;     //红色不限制宽度
        angle_tol  = 15.0f;
    } else {  // Blue
        min_area   = 50.0f;    // 蓝色灯条更小，降低面积要求
        min_length = 10.0f;     // 蓝色灯条更短，降低长度要求
        min_ratio  = 2.0f;     // 长宽比下限保持
        max_ratio  = 8.0f;     // 长宽比上限
        min_width  = 0.0f;     // ← 新增：宽度过滤
        angle_tol  = 10.0f;    // 角度容忍度
    }

    // 找轮廓
    vector<vector<Point>> contours;
    findContours(mask.clone(), contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    vector<LightBar> bars;

    for (const auto& contour : contours) {
        float area = contourArea(contour);
        if (area < min_area) continue;

        RotatedRect rect = minAreaRect(contour);
        float w = rect.size.width;
        float h = rect.size.height;
        float longSide = max(w, h);
        float shortSide = min(w, h);
        float ratio = longSide / shortSide;
        if (longSide < min_length) continue;
        if (ratio < min_ratio || ratio > max_ratio) continue;
        
        if (shortSide < min_width) continue;   // ← 新增：宽度过滤

        // 角度处理
        float angle = rect.angle;
        if (w < h) angle += 90;
        if (angle < -90) angle += 180;
        if (angle >= 90) angle -= 180;

        // 角度过滤：保留接近竖直的灯条
        float angle_abs = abs(angle);
        if (abs(angle_abs - 90.0f) > angle_tol) continue;

        LightBar bar;
        bar.rect = rect;
        bar.length = longSide;
        bar.width = shortSide;
        bar.angle = angle;
        bar.area = area;
        bar.center = rect.center;
        bars.push_back(bar);
    }

   

        // 绘制灯条候选
        for (const auto& bar : bars) {
            Point2f vertices[4];
            bar.rect.points(vertices);
            for (int i = 0; i < 4; i++)
                line(drawImg, vertices[i], vertices[(i+1)%4], Scalar(255, 0, 0), 2);
            circle(drawImg, bar.center, 3, Scalar(0, 255, 255), -1);
        }

        // 显示数量
        putText(drawImg, colorName + " LightBars: " + to_string(bars.size()),
                Point(30, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 0, 0), 2);

        return bars;
    };

    // 创建绘图图像
    Mat draw_red = src.clone();
    Mat draw_blue = src.clone();

    vector<LightBar> redBars = processColor(mask_red, "Red", draw_red);
    vector<LightBar> blueBars = processColor(mask_blue, "Blue", draw_blue);

    // 保存中间结果
    imwrite(assets + "step4_lightbars_red.jpg", draw_red);
    imwrite(assets + "step4_lightbars_blue.jpg", draw_blue);

    cout << "红色灯条候选数: " << redBars.size() << endl;
    cout << "蓝色灯条候选数: " << blueBars.size() << endl;
    cout << "灯条筛选结果已保存" << endl;

    // 显示
    imshow("Red LightBars", draw_red);
    imshow("Blue LightBars", draw_blue);
    waitKey(0);

    return 0;
}
