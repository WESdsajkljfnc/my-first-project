from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    # talker 节点，传入参数 period
    talker_node = Node(
        package='training_pkg',
        executable='talker',
        name='my_talker',          # 自定义节点名（可选）
        parameters=[{
            'period': 1.0          # 设置为 1 秒发布一次
        }]
    )

    # listener 节点
    listener_node = Node(
        package='training_pkg',
        executable='listener',
        name='my_listener'
    )

    # 返回 LaunchDescription，包含两个节点
    return LaunchDescription([talker_node, listener_node])
