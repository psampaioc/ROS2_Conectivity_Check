# Copyright 2026 Pedro Sampaio
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, GroupAction
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node, PushRosNamespace
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    # Use get_package_share_directory equivalent for config file path
    config_file = PathJoinSubstitution([
        FindPackageShare('conectivity_check'),
        'config',
        'connectivity.yaml'
    ])

    namespace_arg = DeclareLaunchArgument(
        'namespace', default_value='', description='Namespace'
    )

    return LaunchDescription([
        namespace_arg,

        GroupAction([
            PushRosNamespace(LaunchConfiguration('namespace')),

            # Ping Checker
            Node(
                package='conectivity_check',
                executable='ping_checker',
                name='ping_checker',
                output='screen',
                parameters=[{'config_file': config_file}],
            ),

            # RSS Monitor (CORE)
            Node(
                package='conectivity_check',
                executable='rss_monitor',
                name='rss_monitor',
                output='screen',
                parameters=[{'config_file': config_file}],
            ),

            # Connectivity Monitor (Aggregator - optional)
            Node(
                package='conectivity_check',
                executable='connectivity_monitor',
                name='connectivity_monitor',
                output='screen',
                parameters=[{'config_file': config_file}],
            ),

            # Speedtest Server
            Node(
                package='conectivity_check',
                executable='speedtest_server',
                name='speedtest_server',
                output='screen',
                parameters=[{'config_file': config_file}],
            ),
        ]),
    ])
