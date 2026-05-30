import os
from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    pkg_dir = get_package_share_directory('armor_detect_ros')
    params_path = os.path.join(pkg_dir, 'config', 'params.yaml')

    return LaunchDescription([
        # 1. 海康相机驱动节点
        Node(
            package='armor_detect_ros',
            executable='hik_camera_node',
            name='hik_camera_node',
            output='screen'
        ),
        # 2. 装甲板检测节点
        Node(
            package='armor_detect_ros',
            executable='detect_node',
            name='detect_node',
            parameters=[params_path],
            output='screen'
        ),
        # 3. 显示节点（实时显示检测结果）
        Node(
            package='armor_detect_ros',
            executable='display_node',
            name='display_node',
            output='screen'
        )
    ])