cmake_minimum_required(VERSION 3.0.2)
project(my_robot_description)

find_package(catkin REQUIRED COMPONENTS roscpp nav_msgs geometry_msgs)
find_package(Qt5 COMPONENTS Widgets REQUIRED)

# 启用自动 MOC 和 UIC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

include_directories(
  ${catkin_INCLUDE_DIRS}
  ${Qt5Widgets_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(${PROJECT_NAME}_node
  src/ros_qt_node.cpp
  src/mainwindow.cpp
  src/mainwindow.ui   # 明确列出 .ui 文件（可选，但有助于 AUTOUIC）
)

target_link_libraries(${PROJECT_NAME}_node
  ${catkin_LIBRARIES}
  Qt5::Widgets
  mosquittopp   # 根据您的 MQTT 库
)
# 安装编译生成的C++可执行文件
install(TARGETS my_robot_description_node
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)
