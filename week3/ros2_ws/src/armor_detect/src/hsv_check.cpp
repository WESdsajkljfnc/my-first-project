#include <opencv2/opencv.hpp>
#include <iostream>
#include <cstdlib>

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

    cout << "请点击图片中的灯条区域，按任意键退出" << endl;
    namedWindow("取色", WINDOW_NORMAL);
    imshow("取色", src);

    // 鼠标回调：点击时打印 BGR 和 HSV 值
    struct Param { Mat* src; Mat* hsv; };
    Param param{&src, &hsv};

    auto onMouse = [](int event, int x, int y, int, void* userdata) {
        if (event == EVENT_LBUTTONDOWN) {
            Param* p = (Param*)userdata;
            Vec3b bgr = p->src->at<Vec3b>(y, x);
            Mat bgr_pixel(1, 1, CV_8UC3, bgr);
            Mat hsv_pixel;
            cvtColor(bgr_pixel, hsv_pixel, COLOR_BGR2HSV);
            Vec3b hsv_val = hsv_pixel.at<Vec3b>(0, 0);
            cout << "坐标(" << x << "," << y << ")  BGR:("
                 << (int)bgr[0] << "," << (int)bgr[1] << "," << (int)bgr[2]
                 << ")  HSV:(" << (int)hsv_val[0] << "," << (int)hsv_val[1] << "," << (int)hsv_val[2] << ")" << endl;
        }
    };

    setMouseCallback("取色", onMouse, &param);
    waitKey(0);
    destroyAllWindows();
    return 0;
}
