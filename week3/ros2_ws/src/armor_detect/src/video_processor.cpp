#include "armor_detect/armor_detector.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <cstdlib>

using namespace cv;
using namespace std;

int main() {
    string home = getenv("HOME");
    string video_dir = home + "/桌面/course/week3/videos/";

    vector<string> videos = {
        "Blue_1000.mp4", "Blue_3000.mp4", "Blue_5000.mp4",
        "Red_1000.mp4",  "Red_3000.mp4",  "Red_5000.mp4"
    };

    ArmorDetector detector;

    for (const auto& name : videos) {
        string input_path  = video_dir + name;
        string output_path = video_dir + "output_" + name;

        VideoCapture cap(input_path);
        if (!cap.isOpened()) { cerr << "无法打开: " << input_path << endl; continue; }

        int w = cap.get(CAP_PROP_FRAME_WIDTH), h = cap.get(CAP_PROP_FRAME_HEIGHT);
        double fps = cap.get(CAP_PROP_FPS);
        VideoWriter writer(output_path, VideoWriter::fourcc('m','p','4','v'), fps, Size(w,h));

        cout << "处理中: " << name << " -> output_" << name << endl;
        Mat frame;
        int frame_count = 0;
        while (cap.read(frame)) {
            frame_count++;
            auto results = detector.detect(frame);
            Mat output = detector.getDebugFrame();
            writer << output;
        }
        cout << "完成: " << name << " (" << frame_count << "帧)" << endl;
    }

    cout << "全部视频处理完成！" << endl;
    return 0;
}
