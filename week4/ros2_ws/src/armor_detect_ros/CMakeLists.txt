cmake_minimum_required(VERSION 3.8)
project(armor_detect_ros)

# 默认编译选项
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 查找依赖
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(std_msgs REQUIRED)
find_package(OpenCV REQUIRED)

# 包含头文件目录
include_directories(include)

# 将 W3 的 armor_detector.cpp 编译为库
add_library(armor_detector_lib src/armor_detector.cpp)
target_link_libraries(armor_detector_lib ${OpenCV_LIBRARIES})
target_include_directories(armor_detector_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# 检测节点可执行文件
add_executable(detect_node src/detect_node.cpp)
ament_target_dependencies(detect_node rclcpp sensor_msgs cv_bridge std_msgs)
target_link_libraries(detect_node armor_detector_lib ${OpenCV_LIBRARIES})
target_include_directories(detect_node PRIVATE ${OpenCV_INCLUDE_DIRS})

# 安装规则
install(TARGETS detect_node
  DESTINATION lib/${PROJECT_NAME}
)
install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY config launch
  DESTINATION share/${PROJECT_NAME}
)

# 海康相机节点
add_executable(hik_camera_node src/hik_camera_node.cpp)
ament_target_dependencies(hik_camera_node rclcpp sensor_msgs cv_bridge std_msgs)
target_include_directories(hik_camera_node PRIVATE
    /opt/MVS/include
    ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(hik_camera_node
    ${OpenCV_LIBRARIES}
    /opt/MVS/lib/64/libMvCameraControl.so
)
install(TARGETS hik_camera_node
    DESTINATION lib/${PROJECT_NAME}
)
# 取色节点
add_executable(color_picker src/color_picker.cpp)
ament_target_dependencies(color_picker rclcpp sensor_msgs cv_bridge std_msgs)
target_link_libraries(color_picker ${OpenCV_LIBRARIES})
target_include_directories(color_picker PRIVATE ${OpenCV_INCLUDE_DIRS})
install(TARGETS color_picker DESTINATION lib/${PROJECT_NAME})

# 测试支持（可选）
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
