#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

// ------------------------------------------------------------
// MqttClient 实现
// ------------------------------------------------------------
MqttClient::MqttClient(const char *id, QObject *parent)
    : QObject(parent), mosqpp::mosquittopp(id), m_connected(false)
{
   // username_pw_set("admin", "public");
    // 启动库自带后台循环线程，不阻塞UI
    int loopRc = loop_start();
    if (loopRc != MOSQ_ERR_SUCCESS) {
        emit logMsg("⚠️ 无法启动 MQTT 网络线程，请检查库版本");
    }
}
MqttClient::~MqttClient()
{
    if (isConnected()) {
        mosquittopp::disconnect(); // 带超时，消除deprecated警告
    }
    loop_stop(true); // 等待线程完全退出
}

bool MqttClient::connectToBroker(const QString &host, int port, int keepalive)
{
    if (isConnected()) {
        emit logMsg("⚠️ 已连接，先断开旧连接");
        mosquittopp::disconnect();
    }

    int rc = mosquittopp::connect(host.toStdString().c_str(), port, keepalive);
    if (rc != MOSQ_ERR_SUCCESS) {
        emit logMsg(QString("❌ 连接请求失败，错误码: %1").arg(rc));
        return false;
    }
    emit logMsg("⏳ 正在连接 MQTT 服务器...");
    return true;
}

bool MqttClient::publishMessage(const QString &topic, const QString &payload, int qos, bool retain)
{
    int rc = mosquittopp::publish(nullptr, topic.toStdString().c_str(),
                                  payload.size(), payload.toStdString().c_str(),
                                  qos, retain);
    if (rc != MOSQ_ERR_SUCCESS) {
        emit logMsg(QString("❌ 发布失败，错误码: %1").arg(rc));
        return false;
    }
    return true;
}
void MqttClient::on_connect(int rc)
{
    if (rc == 0) {
        m_connected = true;
        emit logMsg("✅ MQTT 连接成功！");
        emit connected();
    } else {
        m_connected = false;
        emit logMsg(QString("❌ MQTT 连接失败，错误码: %1").arg(rc));
    }
}

void MqttClient::on_disconnect(int rc)
{
    m_connected = false;
    emit logMsg(QString("⚠️ MQTT 已断开，原因码: %1").arg(rc));
    emit disconnected();
}

void MqttClient::on_publish(int mid)
{
    emit logMsg(QString("📤 消息发布成功，mid: %1").arg(mid));
    emit messagePublished(mid);
}
// ------------------------------------------------------------
// MainWindow 实现
// ------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    mosqpp::lib_init();

    // 创建客户端，不再传入ui->textEdit_log
    mqttClient = new MqttClient("qt_client", this);

    // 核心绑定：MQTT子线程的日志信号 → 主线程文本框append（线程安全）
    connect(mqttClient, &MqttClient::logMsg, ui->textEdit_log, &QTextEdit::append);

    // 原有connected信号绑定不变
    connect(mqttClient, &MqttClient::connected, this, [this]() {
        QString data = ui->textEdit_log->toPlainText();
        if (data.isEmpty()) {
            ui->textEdit_log->append("⚠️ 没有数据可发送");
            return;
        }
        mqttClient->publishMessage("log/data", data);
    });

    timeTimer = new QTimer(this);
    connect(timeTimer, &QTimer::timeout, this, &MainWindow::updateTime);
    timeTimer->start(1000);

    pushdata_timer = new QTimer(this);
    connect(pushdata_timer, &QTimer::timeout, this, &MainWindow::update1Time);
    pushdata_timer->stop();
}

MainWindow::~MainWindow()
{
    delete mqttClient;
    mosqpp::lib_cleanup();
    delete ui;
}

// -------------------- 页面切换 -----------------------
void MainWindow::on_btn_page1_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}
void MainWindow::on_btn_page2_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}
void MainWindow::on_btn_page3_clicked()
{
    ui->stackedWidget->setCurrentIndex(2);
}

// -------------------- 时间更新 -----------------------
void MainWindow::updateTime()
{
    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString showText = QString("TIME：%1").arg(currentTime);
    ui->label_time1->setText(showText);
    ui->label_time2->setText(showText);
    ui->label_time3->setText(showText);
}

void MainWindow::update1Time()
{

    QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString id = ui->lineEdit->text();

    int type = 0;
    if (ui->radioButton_vehicle->isChecked())
        type = 1;
    else if (ui->radioButton_UAV->isChecked())
        type = 2;

    QString latitude = ui->LinEdit_latitude->text();   // 注意控件名，请确认是否正确
    QString longitude = ui->lineEdit_longitude->text(); // 改名为 longitude 更规范
    QString euler = ui->lineEdit_euler->text();

//    QString laserstatus = ui->comboBox_laser->currentText();
//    QString Miwastatus = ui->comboBox_MiWa->currentText();
//    QString camerastatus = ui->comboBox_camera->currentText();
//    QString BeiDouststus = ui->comboBox_BeiDou->currentText();
    int laserstatus = (ui->comboBox_laser->currentText() == "online") ? 1 : 0;
    int miwaStatus = (ui->comboBox_MiWa->currentText() == "online") ? 1 : 0;
    int camerastatus = (ui->comboBox_camera->currentText() == "online") ? 1 : 0;
    int BeiDouststus = (ui->comboBox_BeiDou->currentText() == "online") ? 1 : 0;


    int speed = ui->horizontalSlider->value();
    int power = ui->progressBar_power->value();

    int Alarmstatus = 0;
    if (ui->checkBox_overspeed->isChecked()) Alarmstatus |= 1;   // bit0
    if (ui->checkBox_diverge->isChecked())   Alarmstatus |= 2;   // bit1
    if (ui->checkBox_lowpower->isChecked())  Alarmstatus |= 4;   // bit2

    // 构造 JSON，注意占位符 %1~%13 与后面 .arg() 一一对应
    QString payload = QString(
        "{\"reportTime\":\"%1\","
        "\"seriesNumber\":\"%2\","
        "\"serviceType\":\"%3\","
        "\"serviceData\": {"
        "\"lidarStatus\":\"%4\","
        "\"cameraStatus\":\"%5\","
        "\"rtkStatus\":\"%6\","
        "\"latitude\":\"%7\","
        "\"longitude\":\"%8\","
        "\"power\":%9,"
        "\"alarmStatus\":%10}}"
    )
    .arg(time)                // %1
    .arg(id)                  // %2
    .arg(type)                // %3
    .arg(laserstatus)         // %4
    .arg(camerastatus)
    .arg(BeiDouststus)
    .arg(latitude)
    .arg(longitude)
    .arg(power)
    .arg(Alarmstatus);
      // 发布到话题 think/car/002
      if (mqttClient->isConnected()) {
          bool ok = mqttClient->publishMessage("/thing/car/20230303002", payload, 0, false);
          if (ok) {
              ui->textEdit_log->append(QString("[%1] 已发布数据到 /thing/car/20230303002").arg(time));
          } else {
              ui->textEdit_log->append(QString("[%1] 发布失败").arg(time));
          }
      } else {
          ui->textEdit_log->append(QString("[%1] MQTT未连接，无法发布").arg(time));
      }

    ui->textEdit_log->append(QString("[%1]suscced upload").arg(time));

    QString ID = ui->lineEdit->text();
    ui->textEdit_log->append(QString("[%1]ID：%2").arg(time).arg(ID));

    QString Lat = ui->LinEdit_latitude->text();
    int in_lAt = Lat.toInt();
    if(in_lAt != 140) {
        ui->textEdit_log->append(QString("[%1]Latitude：%2").arg(time).arg(Lat));
    } else {
        ui->textEdit_log->append(QString("[%1]Latitude：Alarm").arg(time));
    }

    QString aut = ui->lineEdit_longitude->text();
    ui->textEdit_log->append(QString("[%1]Longitude：%2").arg(time).arg(aut));
}

// -------------------- 按钮槽函数 -----------------------
void MainWindow::on_btn_connect_server_clicked()
{
//   QString host = "10.134.2.113";
//    int port = 1884;r
   QString host = "broker.emqx.io";
   int port = 1883;
    bool ok = mqttClient->connectToBroker(host, port);
    if (ok) {
        ui->textEdit_log->append(QString("[%1] 发起连接 MQTT %2:%3")
                                 .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                                 .arg(host).arg(port));
    } else {
        ui->textEdit_log->append(QString("[%1] 连接发起失败")
                                 .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
    }
}

void MainWindow::on_btn_push_data_clicked()
{
    if (!m_refreshRunning) {
        pushdata_timer->start(2000);
        m_refreshRunning = true;
        ui->textEdit_log->append("open 1s");
    } else {
        pushdata_timer->stop();
        m_refreshRunning = false;
        ui->textEdit_log->append("close 1s");
    }
}

void MainWindow::on_btn_clear_log_clicked()
{
    ui->textEdit_log->clear();
}

void MainWindow::on_checkBox_lowpower_stateChanged(int arg1)
{
    QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
    if (arg1 == Qt::Checked) {
        ui->textEdit_log->append(QString("[%1] open lowpoer alarm").arg(time));
    } else if (arg1 == Qt::Unchecked) {
        ui->textEdit_log->append(QString("[%1]  close lowpoer alarm").arg(time));
    }
}

void MainWindow::on_horizontalSlider_actionTriggered(int action)
{
    QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
    int sliderVal = ui->horizontalSlider->value();
    ui->textEdit_log->append(QString("[%1]speed %2").arg(time).arg(sliderVal));
}

void MainWindow::on_comboBox_BeiDou_activated(const QString &arg1)
{

    QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
    int idx = ui->comboBox_BeiDou->currentIndex();
    if(idx == 0) {
      ui->textEdit_log->append(QString("[%1]BeiDouposition : online").arg(time));
     } else if(idx == 1) {
       ui->textEdit_log->append(QString("[%1]BeiDouposition : Outline").arg(time));
   }
}

void MainWindow::updateLatLon(double lat, double lon)
{
    // 假设您的 UI 中有两个 QLineEdit 名为 lat_edit 和 lon_edit
    ui->LinEdit_latitude->setText(QString::number(lat, 'f', 8));
    ui->lineEdit_longitude->setText(QString::number(lon, 'f', 8));
}
