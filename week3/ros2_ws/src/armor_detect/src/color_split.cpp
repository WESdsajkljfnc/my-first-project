#include <opencv2/opencv.hpp>
#include <iostream>
#include <cstdlib>

int main() {
    // 构建 Assets 绝对路径
    std::string home = std::getenv("HOME");
    std::string assets_dir = home + "/桌面/course/week3/Assets/";
    std::string image_path = assets_dir + "armor.jpg";

    // 读取图片
    cv::Mat src = cv::imread(image_path);
    if (src.empty()) {
        std::cerr << "无法读取图片: " << image_path << std::endl;
        return -1;
    }

    // 转换到 HSV
    cv::Mat hsv;
    cv::cvtColor(src, hsv, cv::COLOR_BGR2HSV);

    // 红色阈值（两段合并）
    cv::Mat mask_red1, mask_red2, mask_red;
    cv::inRange(hsv, cv::Scalar(0, 100, 100), cv::Scalar(10, 255, 255), mask_red1);
    cv::inRange(hsv, cv::Scalar(160, 100, 100), cv::Scalar(180, 255, 255), mask_red2);
    cv::bitwise_or(mask_red1, mask_red2, mask_red);

    // 蓝色阈值
    cv::Mat mask_blue;
    cv::inRange(hsv, cv::Scalar(100, 100, 100), cv::Scalar(130, 255, 255), mask_blue);

    // 形态学运算（开运算 + 闭运算）
    cv::Mat kernel_open  = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::Mat kernel_close = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(7, 7));

    // 红色 mask 处理
    cv::morphologyEx(mask_red, mask_red, cv::MORPH_OPEN, kernel_open);
    cv::morphologyEx(mask_red, mask_red, cv::MORPH_CLOSE, kernel_close);

    // 蓝色 mask 处理
    cv::morphologyEx(mask_blue, mask_blue, cv::MORPH_OPEN, kernel_open);
    cv::morphologyEx(mask_blue, mask_blue, cv::MORPH_CLOSE, kernel_close);

    // 保存最终掩模
    cv::imwrite(assets_dir + "red_mask.jpg", mask_red);
    cv::imwrite(assets_dir + "blue_mask.jpg", mask_blue);

    std::cout << "颜色分离完成，掩模已保存至 " << assets_dir << std::endl;
   
    //生成彩色叠加图
    cv::Mat red_part, blue_part;
    cv::bitwise_and(src, src, red_part, mask_red);
    cv::bitwise_and(src, src, blue_part, mask_blue);

    // 显示（原图，掩膜，红色区域，蓝色区域）
    cv::imshow("原图", src);
    cv::imshow("红色掩膜", mask_red);
    cv::imshow("蓝色掩膜", mask_blue);
    cv::imshow("红色区域", red_part);
    cv::imshow("蓝色区域", blue_part);
    cv::waitKey(0);
    return 0;
}
