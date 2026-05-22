#include "armor_detect/armor_detector.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <cstdlib>

using namespace cv;
using namespace std;

int main() {
    string home = getenv("HOME");
    string video_path = home + "/桌面/course/week3/videos/Red_1000.mp4";  // 先诊断这个

    VideoCapture cap(video_path);
    if (!cap.isOpened()) { cerr << "无法打开" << endl; return -1; }

    Mat frame;
    cap >> frame;

    ArmorDetector detector(ArmorDetector::VIDEO);
    auto results = detector.detect(frame);

    cout << "检测到装甲板: " << results.size() << " 个" << endl;
    for (auto& r : results) {
        cout << "颜色: " << r.color << " 角点: ";
        for (auto& p : r.corners) cout << "(" << p.x << "," << p.y << ") ";
        cout << endl;
    }

    // 显示掩膜和灯条
    imshow("红色掩膜", detector.getRedMask());
    imshow("蓝色掩膜", detector.getBlueMask());
    imshow("红色灯条", detector.getRedBars());
    imshow("蓝色灯条", detector.getBlueBars());
    imshow("检测结果", detector.getDebugFrame());
    waitKey(0);

    return 0;
}
