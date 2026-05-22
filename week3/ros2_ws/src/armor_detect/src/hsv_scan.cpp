#include <opencv2/opencv.hpp>
#include <iostream>
#include <cstdlib>
#include <map>

using namespace cv;
using namespace std;

int main() {
    string home = getenv("HOME");
    string assets = home + "/桌面/course/week3/Assets/";
    string image_path = assets + "armor.jpg";

    Mat src = imread(image_path);
    if (src.empty()) {
        cerr << "无法读取图片: " << image_path << endl;
        return -1;
    }

    Mat hsv;
    cvtColor(src, hsv, COLOR_BGR2HSV);

    // 统计高亮像素（V > 200）的 H 和 S 分布
    map<int, int> h_hist;  // H 值分布
    int count = 0;

    for (int y = 0; y < hsv.rows; y++) {
        for (int x = 0; x < hsv.cols; x++) {
            Vec3b pixel = hsv.at<Vec3b>(y, x);
            int h = pixel[0];
            int s = pixel[1];
            int v = pixel[2];

            // 只统计高亮像素（V > 200）且非灰色（S > 20）
            if (v > 200 && s > 20) {
                h_hist[h]++;
                count++;
            }
        }
    }

    cout << "图片中 V>200 且 S>20 的像素总数: " << count << endl;
    cout << "H 值分布（只显示像素数 > 100 的 H 值）:" << endl;
    for (auto& kv : h_hist) {
        if (kv.second > 100) {
            cout << "H=" << kv.first << "  像素数=" << kv.second << endl;
        }
    }

    // 同时保存一张可视化图：把符合上述条件的像素标成绿色
    Mat visual = src.clone();
    for (int y = 0; y < hsv.rows; y++) {
        for (int x = 0; x < hsv.cols; x++) {
            Vec3b pixel = hsv.at<Vec3b>(y, x);
            if (pixel[2] > 200 && pixel[1] > 20) {
                visual.at<Vec3b>(y, x) = Vec3b(0, 255, 0);  // 绿色标记
            }
        }
    }
    imwrite(assets + "hsv_scan_marked.jpg", visual);
    cout << "已保存标记图: " << assets + "hsv_scan_marked.jpg" << endl;

    imshow("高亮且饱和的区域（绿色标记）", visual);
    waitKey(0);
    return 0;
}
