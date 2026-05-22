cmake_minimum_required(VERSION 3.8)
project(armor_detect)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(std_msgs REQUIRED)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

find_package(OpenCV REQUIRED)

# 包含头文件目录
include_directories(include)

# ========== 新增：装甲板检测库 ==========
add_library(armor_detector_lib src/armor_detector.cpp)
target_link_libraries(armor_detector_lib ${OpenCV_LIBRARIES})
target_include_directories(armor_detector_lib PUBLIC include)

# ========== 新增：视频批处理器 ==========
add_executable(video_processor src/video_processor.cpp)
target_link_libraries(video_processor armor_detector_lib ${OpenCV_LIBRARIES})
install(TARGETS video_processor DESTINATION lib/${PROJECT_NAME})

# ========== 以下是你原有的可执行文件（保留不动） ==========

add_executable(color_split src/color_split.cpp)
ament_target_dependencies(color_split rclcpp sensor_msgs cv_bridge std_msgs)
target_link_libraries(color_split ${OpenCV_LIBRARIES})
target_include_directories(color_split PRIVATE ${OpenCV_INCLUDE_DIRS})
install(TARGETS color_split DESTINATION lib/${PROJECT_NAME})

add_executable(step4_lightbars src/step4_lightbars.cpp)
target_link_libraries(step4_lightbars ${OpenCV_LIBRARIES})
target_include_directories(step4_lightbars PRIVATE ${OpenCV_INCLUDE_DIRS})
install(TARGETS step4_lightbars DESTINATION lib/${PROJECT_NAME})

add_executable(hsv_check src/hsv_check.cpp)
target_link_libraries(hsv_check ${OpenCV_LIBRARIES})
target_include_directories(hsv_check PRIVATE ${OpenCV_INCLUDE_DIRS})
install(TARGETS hsv_check DESTINATION lib/${PROJECT_NAME})

add_executable(hsv_scan src/hsv_scan.cpp)
target_link_libraries(hsv_scan ${OpenCV_LIBRARIES})
target_include_directories(hsv_scan PRIVATE ${OpenCV_INCLUDE_DIRS})
install(TARGETS hsv_scan DESTINATION lib/${PROJECT_NAME})

# add_executable(step5_armor src/step5_armor.cpp)
# target_link_libraries(step5_armor ${OpenCV_LIBRARIES})
# target_include_directories(step5_armor PRIVATE ${OpenCV_INCLUDE_DIRS})
# install(TARGETS step5_armor DESTINATION lib/${PROJECT_NAME})

add_executable(video_player src/video_player.cpp)
target_link_libraries(video_player ${OpenCV_LIBRARIES})
target_include_directories(video_player PRIVATE ${OpenCV_INCLUDE_DIRS})
install(TARGETS video_player DESTINATION lib/${PROJECT_NAME})

add_executable(diagnose src/diagnose.cpp)
target_link_libraries(diagnose armor_detector_lib ${OpenCV_LIBRARIES})
install(TARGETS diagnose DESTINATION lib/${PROJECT_NAME})

add_executable(extract_frames src/extract_frames.cpp)
target_link_libraries(extract_frames ${OpenCV_LIBRARIES})
install(TARGETS extract_frames DESTINATION lib/${PROJECT_NAME})

ament_package()
