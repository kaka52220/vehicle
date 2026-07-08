#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import rospy
import sys
import json
import time
import math
from PyQt5.QtWidgets import *
from PyQt5.QtCore import *
from PyQt5.QtGui import *
from nav_msgs.msg import Odometry
from geometry_msgs.msg import PoseStamped
import paho.mqtt.client as mqtt

# ---------- 常量定义 ----------
LAT_ORIGIN = 30.39655027
LON_ORIGIN = 114.53435053
DEG_TO_RAD = math.pi / 180.0
# 纬度每度米数（近似）
M_PER_DEG_LAT = 111320.0
# 经度每度米数随纬度变化
M_PER_DEG_LON = M_PER_DEG_LAT * math.cos(LAT_ORIGIN * DEG_TO_RAD)

# ---------- ROS 订阅回调 ----------
current_lat = LAT_ORIGIN
current_lon = LON_ORIGIN

def odom_callback(msg):
    global current_lat, current_lon
    # 获取位置 (x, y) 单位：米
    x = msg.pose.pose.position.x
    y = msg.pose.pose.position.y
    # 转换为经纬度偏移
    delta_lat = y / M_PER_DEG_LAT
    delta_lon = x / M_PER_DEG_LON
    current_lat = LAT_ORIGIN + delta_lat
    current_lon = LON_ORIGIN + delta_lon
    # 更新界面（通过信号）
    try:
        main_window.update_latlon(current_lat, current_lon)
    except:
        pass

# ---------- Qt 主窗口 ----------
class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("车辆状态监控与上传")
        self.setGeometry(100, 100, 500, 400)

        # 创建中心部件和布局
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        layout = QVBoxLayout(central_widget)

        # ----- 第一行：经纬度（只读）-----
        lat_lon_layout = QHBoxLayout()
        lat_lon_layout.addWidget(QLabel("纬度:"))
        self.lat_edit = QLineEdit(str(LAT_ORIGIN))
        self.lat_edit.setReadOnly(True)
        lat_lon_layout.addWidget(self.lat_edit)
        lat_lon_layout.addWidget(QLabel("经度:"))
        self.lon_edit = QLineEdit(str(LON_ORIGIN))
        self.lon_edit.setReadOnly(True)
        lat_lon_layout.addWidget(self.lon_edit)
        layout.addLayout(lat_lon_layout)

        # ----- 第二行：终端ID -----
        id_layout = QHBoxLayout()
        id_layout.addWidget(QLabel("终端ID:"))
        self.id_edit = QLineEdit("XS09290320")
        id_layout.addWidget(self.id_edit)
        layout.addLayout(id_layout)

        # ----- 第三行：状态选择（下拉框）-----
        grid = QGridLayout()
        row = 0
        grid.addWidget(QLabel("激光雷达:"), row, 0)
        self.lidar_combo = QComboBox()
        self.lidar_combo.addItems(["在线", "离线"])
        self.lidar_combo.setCurrentIndex(0)
        grid.addWidget(self.lidar_combo, row, 1)

        grid.addWidget(QLabel("相机:"), row, 2)
        self.camera_combo = QComboBox()
        self.camera_combo.addItems(["在线", "离线"])
        self.camera_combo.setCurrentIndex(0)
        grid.addWidget(self.camera_combo, row, 3)

        row += 1
        grid.addWidget(QLabel("北斗定位:"), row, 0)
        self.rtk_combo = QComboBox()
        self.rtk_combo.addItems(["正常", "失效"])
        self.rtk_combo.setCurrentIndex(0)
        grid.addWidget(self.rtk_combo, row, 1)

        grid.addWidget(QLabel("电量:"), row, 2)
        self.power_slider = QSlider(Qt.Horizontal)
        self.power_slider.setRange(0, 100)
        self.power_slider.setValue(80)
        self.power_slider.setTickInterval(10)
        self.power_slider.setTickPosition(QSlider.TicksBelow)
        self.power_label = QLabel("80%")
        self.power_slider.valueChanged.connect(lambda v: self.power_label.setText(f"{v}%"))
        grid.addWidget(self.power_slider, row, 3)
        grid.addWidget(self.power_label, row, 4)
        layout.addLayout(grid)

        # ----- 第四行：报警状态（复选框）-----
        alarm_group = QGroupBox("报警状态（多选）")
        alarm_layout = QHBoxLayout()
        self.alarm_speed = QCheckBox("超速")
        self.alarm_route = QCheckBox("偏离路线")
        self.alarm_power = QCheckBox("电量过低")
        alarm_layout.addWidget(self.alarm_speed)
        alarm_layout.addWidget(self.alarm_route)
        alarm_layout.addWidget(self.alarm_power)
        alarm_group.setLayout(alarm_layout)
        layout.addWidget(alarm_group)

        # ----- 第五行：按钮 -----
        btn_layout = QHBoxLayout()
        self.connect_btn = QPushButton("连接服务器")
        self.connect_btn.clicked.connect(self.connect_server)
        btn_layout.addWidget(self.connect_btn)

        self.upload_btn = QPushButton("上传数据")
        self.upload_btn.clicked.connect(self.upload_data)
        btn_layout.addWidget(self.upload_btn)

        layout.addLayout(btn_layout)

        # ----- 状态栏 -----
        self.status_bar = QStatusBar()
        self.setStatusBar(self.status_bar)

        # ----- MQTT 客户端 -----
        self.mqtt_client = None
        self.mqtt_connected = False

        # 全局引用，供回调更新
        global main_window
        main_window = self

    def update_latlon(self, lat, lon):
        """由ROS回调调用的更新经纬度"""
        self.lat_edit.setText(f"{lat:.8f}")
        self.lon_edit.setText(f"{lon:.8f}")

    def connect_server(self):
        """连接 EMQX 服务器"""
        if self.mqtt_connected:
            self.status_bar.showMessage("已连接，无需重复连接", 3000)
            return
        try:
            # 可修改为您的 EMQX 地址
            broker = "localhost"
            port = 1883
            self.mqtt_client = mqtt.Client()
            self.mqtt_client.connect(broker, port, 60)
            self.mqtt_client.loop_start()
            self.mqtt_connected = True
            self.status_bar.showMessage(f"已连接到 {broker}:{port}", 5000)
        except Exception as e:
            self.status_bar.showMessage(f"连接失败: {str(e)}", 5000)

    def upload_data(self):
        """打包JSON并发布"""
        if not self.mqtt_connected:
            self.status_bar.showMessage("请先连接服务器", 3000)
            return

        # 读取界面参数
        series = self.id_edit.text().strip()
        if not series:
            series = "XS09290320"

        # 状态值（转为0/1）
        lidar = 0 if self.lidar_combo.currentIndex() == 1 else 1
        camera = 0 if self.camera_combo.currentIndex() == 1 else 1
        rtk = 0 if self.rtk_combo.currentIndex() == 1 else 1
        power = self.power_slider.value()

        # 报警状态组合（位0:超速, 位1:偏离路线, 位2:电量过低）
        alarm = 0
        if self.alarm_speed.isChecked():
            alarm |= 1
        if self.alarm_route.isChecked():
            alarm |= 2
        if self.alarm_power.isChecked():
            alarm |= 4

        # 当前经纬度（从界面读取，确保是最新）
        try:
            lat = float(self.lat_edit.text())
            lon = float(self.lon_edit.text())
        except:
            lat = LAT_ORIGIN
            lon = LON_ORIGIN

        # 时间戳
        report_time = int(time.time())

        # 构建 JSON
        data = {
            "reportTime": str(report_time),
            "seriesNumber": series,
            "serviceType": "1",          # 无人车
            "serviceData": {
                "lidarStatus": lidar,
                "cameraStatus": camera,
                "rtkStatus": rtk,
                "latitude": lat,
                "longitude": lon,
                "power": power,
                "alarmStatus": alarm
            },
            "serviceType": "updateCarStatus"   # 最外层 serviceType（注意原示例有两个）
        }

        json_str = json.dumps(data, ensure_ascii=False)
        # 发布 Topic: /thing/car/学号  请将“学号”替换为您的实际学号
        topic = "/thing/car/你的学号"
        self.mqtt_client.publish(topic, json_str)
        self.status_bar.showMessage(f"数据已上传至 {topic}", 5000)

# ---------- ROS 节点主函数 ----------
def ros_node():
    rospy.init_node('car_status_node', anonymous=True)
    # 订阅 /odom（请根据您实际发布的 odom 话题名调整，也可能为 /gazebo/model_states）
    rospy.Subscriber("/odom", Odometry, odom_callback, queue_size=10)
    # 如果使用 /gazebo/model_states，可另行处理，这里以 /odom 为例

    # 启动 Qt 应用程序
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()

    # 单独开一个线程处理 ROS 回调，避免阻塞 Qt
    import threading
    def spin_ros():
        rospy.spin()
    ros_thread = threading.Thread(target=spin_ros)
    ros_thread.daemon = True
    ros_thread.start()

    # 进入 Qt 事件循环
    sys.exit(app.exec_())

if __name__ == "__main__":
    # 若未设置 ROS_MASTER_URI，可在此设置
    try:
        ros_node()
    except rospy.ROSInterruptException:
        pass
