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
        video_dir + "Blue_1000.mp4",
        video_dir + "Blue_3000.mp4",
        video_dir + "Blue_5000.mp4",
        video_dir + "Red_1000.mp4",
        video_dir + "Red_3000.mp4",
        video_dir + "Red_5000.mp4",
        video_dir + "output_Blue_1000.mp4",
        video_dir + "output_Blue_3000.mp4",
        video_dir + "output_Blue_5000.mp4",
        video_dir + "output_Red_1000.mp4",
        video_dir + "output_Red_3000.mp4",
    	video_dir + "output_Red_5000.mp4"
    };

    int current = 0;
    VideoCapture cap(videos[current]);
    if (!cap.isOpened()) {
        cerr << "无法打开视频: " << videos[current] << endl;
        return -1;
    }

    cout << "按键: N=下一个  ESC=退出  空格=暂停" << endl;
    cout << "当前: " << videos[current] << endl;

    Mat frame;
    namedWindow("Video Player", WINDOW_NORMAL);

    while (true) {
        cap >> frame;
        if (frame.empty()) {
            current = (current + 1) % videos.size();
            cap.open(videos[current]);
            cout << "自动切换: " << videos[current] << endl;
            continue;
        }

        imshow("Video Player", frame);
        int key = waitKey(30);
        if (key == 27) break;
        else if (key == 'n' || key == 'N') {
            current = (current + 1) % videos.size();
            cap.open(videos[current]);
            cout << "切换: " << videos[current] << endl;
        }
        else if (key == 32) {
            while (waitKey(0) != 32);
        }
    }

    cap.release();
    destroyAllWindows();
    return 0;
}
