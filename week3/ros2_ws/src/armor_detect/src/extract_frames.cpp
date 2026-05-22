#include <opencv2/opencv.hpp>
#include <iostream>
#include <cstdlib>

using namespace cv;
using namespace std;

int main() {
    string home = getenv("HOME");
    string video_path = home + "/桌面/course/week3/videos/Red_5000.mp4";
    string output_dir = home + "/桌面/course/week3/Assets/frames_Red5000/";

    // 创建输出目录
    system(("mkdir -p " + output_dir).c_str());

    VideoCapture cap(video_path);
    if (!cap.isOpened()) {
        cerr << "无法打开: " << video_path << endl;
        return -1;
    }

    int total = cap.get(CAP_PROP_FRAME_COUNT);
    int interval = max(1, total / 10);  // 抽 10 帧
    int frame_idx = 0, saved = 0;

    Mat frame;
    while (cap.read(frame)) {
        if (frame_idx % interval == 0) {
            string filename = output_dir + "frame_" + to_string(saved) + ".jpg";
            imwrite(filename, frame);
            cout << "保存: " << filename << endl;
            saved++;
        }
        frame_idx++;
    }

    cout << "共保存 " << saved << " 帧到 " << output_dir << endl;
    return 0;
}
