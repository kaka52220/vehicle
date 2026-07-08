#include <ros/ros.h>
#include <nav_msgs/Odometry.h>
#include <QApplication>
#include <QThread>
#include "mainwindow.h"   // 您的窗口头文件

// 全局指针，便于回调中访问
MainWindow* g_mainWindow = nullptr;

// 经纬度转换常量（同您之前 Python 版本）
const double LAT_ORIGIN = 30.39655027;
const double LON_ORIGIN = 114.53435053;
const double DEG_TO_RAD = M_PI / 180.0;
const double M_PER_DEG_LAT = 111320.0;
const double M_PER_DEG_LON = M_PER_DEG_LAT * std::cos(LAT_ORIGIN * DEG_TO_RAD);

void odomCallback(const nav_msgs::Odometry::ConstPtr& msg)
{
    double x = msg->pose.pose.position.x;
    double y = msg->pose.pose.position.y;
    double delta_lat = y / M_PER_DEG_LAT;
    double delta_lon = x / M_PER_DEG_LON;
    double lat = LAT_ORIGIN + delta_lat;
    double lon = LON_ORIGIN + delta_lon;

    // 通过信号槽更新界面（线程安全）
    if (g_mainWindow) {
        // 使用 Qt::QueuedConnection 确保在主线程执行
        QMetaObject::invokeMethod(g_mainWindow, "updateLatLon",
                                  Qt::QueuedConnection,
                                  Q_ARG(double, lat),
                                  Q_ARG(double, lon));
    }
}

int main(int argc, char** argv)
{
    // 1. 初始化 Qt 应用程序（必须先于 ROS，因为 QApplication 需要 argc/argv）
    QApplication app(argc, argv);

    // 2. 初始化 ROS 节点
    ros::init(argc, argv, "car_status_node");
    ros::NodeHandle nh;

    // 3. 创建您的主窗口
    MainWindow mainWindow;
    g_mainWindow = &mainWindow;
    mainWindow.show();

    // 4. 创建 ROS 订阅者
    ros::Subscriber sub = nh.subscribe("/odom", 10, odomCallback);

    // 5. 启动一个单独的线程来执行 ros::spin()，避免阻塞 Qt 事件循环
    std::thread spin_thread([](){
        ros::spin();
    });

    // 6. 进入 Qt 事件循环（主线程）
    int result = app.exec();

    // 7. 退出清理
    ros::shutdown();
    spin_thread.join();
    return result;
}
