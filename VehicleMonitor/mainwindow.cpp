#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QDebug>
#include <QMetaObject>
#include <mosquittopp.h>
// ---------- MqttClient 实现 ----------
// ------------- MqttClient 实现 -------------
MainWindow::MqttClient::MqttClient(MainWindow* parent)
    : mosqpp::mosquittopp(), m_parent(parent)
{
    mosqpp::lib_init();
    // 不需要设置协议版本，使用默认即可
}

MainWindow::MqttClient::~MqttClient()
{
    loop_stop();      // 停止网络线程
    disconnect();     // 断开连接
    mosqpp::lib_cleanup();
}

bool MainWindow::MqttClient::connectToBroker(const QString &host, int port)
{
    qDebug() << "尝试连接 MQTT broker:" << host << port;
    // 如果有旧连接，先断开（无论是否已连接，调用 disconnect 总是安全的）
    disconnect();
    int rc = connect(host.toStdString().c_str(), port, 60);
    qDebug() << "connect 返回码:" << rc;
    if (rc == MOSQ_ERR_SUCCESS) {
        int loop_rc = loop_start();
        qDebug() << "loop_start 返回:" << loop_rc;
        return true;
    }
    return false;
}

bool MainWindow::MqttClient::subscribeTopic(const QString &topic, int qos)
{
    return subscribe(NULL, topic.toStdString().c_str(), qos) == MOSQ_ERR_SUCCESS;
}

bool MainWindow::MqttClient::publishMessage(const QString &topic, const QString &payload, int qos)
{
    return publish(NULL, topic.toStdString().c_str(), payload.size(), payload.toStdString().c_str(), qos, false) == MOSQ_ERR_SUCCESS;
}

void MainWindow::MqttClient::on_connect(int rc)
{
    qDebug() << "MQTT on_connect 回调, rc=" << rc;
    if (rc == 0) {
        qDebug() << "MQTT 连接成功，订阅主题";
        subscribeTopic("/thing/car/#");
        // 可以发送信号通知主线程连接成功
    } else {
        qDebug() << "MQTT 连接失败，rc=" << rc;
    }
}

void MainWindow::MqttClient::on_disconnect(int rc)
{
    qDebug() << "MQTT on_disconnect 回调, rc=" << rc;
}

void MainWindow::MqttClient::on_message(const struct mosquitto_message *message)
{
    if (message == nullptr || message->payload == nullptr) return;
    QString topic = QString::fromStdString(message->topic);
    QString payload = QString::fromUtf8((const char*)message->payload, message->payloadlen);

    qDebug() << "收到MQTT消息, topic:" << topic << "payload:" << payload;

    QString id = topic.section('/', -1);
    QMetaObject::invokeMethod(m_parent, "parseJsonAndUpdate",
                              Qt::QueuedConnection,
                              Q_ARG(QString, payload),
                              Q_ARG(QString, id));
}
// ---------- MainWindow 实现 ----------
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    qDebug() << "MainWindow 构造函数开始";
    ui->setupUi(this);
    //ui->webView->settings()->setAttribute(QWebEngineSettings::DeveloperExtrasEnabled, true);
    // 创建 WebEngineView 显示地图
   // webView = new QWebEngineView(this);
     //ui->webView->setUrl(QUrl("qrc:/html/map.html"));
  //  setCentralWidget(webView);
    // 加载资源中的地图 HTML（假设已添加到资源 qrc:/html/map.html）
    ui->webView->setUrl(QUrl("qrc:/html/map.html"));

    // 初始化 MQTT 客户端
    mqttClient = new MqttClient(this);

    // 离线检测定时器（每5秒）
    offlineTimer = new QTimer(this);
    connect(offlineTimer, &QTimer::timeout, this, &MainWindow::onTimerCheckTimeout);
    offlineTimer->start(5000);

    // 连接按钮（假设 UI 中有 pushButton_connect）
    //connect(ui->pushButton_connect, &QPushButton::clicked, this, &MainWindow::on_btn_connect_server_clicked);
    qDebug() << "MainWindow 构造函数结束";
}

MainWindow::~MainWindow()
{
    delete mqttClient;
    delete ui;
}

void MainWindow::on_btn_connect_server_clicked()
{
    // 从界面读取服务器地址（假设有 lineEdit_host, lineEdit_port）
     qDebug() << "连接按钮被点击";
    QString host = ui->lineEdit_host->text();
       int port = ui->lineEdit_port->text().toInt();
    if (host.isEmpty()) host = "localhost";
    if (port == 0) port = 1883;
    if (mqttClient->connectToBroker(host, port)) {
        ui->statusbar->showMessage("MQTT 连接成功");
    } else {
        ui->statusbar->showMessage("MQTT 连接失败");
    }
}

void MainWindow::parseJsonAndUpdate(const QString &json, const QString &topicId)
{
    qDebug() << "parseJsonAndUpdate 被调用, JSON:" << json << "topicId:" << topicId;

    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isObject()) {
        qDebug() << "JSON 不是对象";
        return;
    }
    QJsonObject obj = doc.object();

    QString id = obj["seriesNumber"].toString();
    if (id.isEmpty()) {
        qDebug() << "未找到 seriesNumber，使用 topicId";
        id = topicId;
    }
    if (id.isEmpty()) {
        qDebug() << "无法确定车辆 ID";
        return;
    }

    QJsonObject data = obj["serviceData"].toObject();
    if (data.isEmpty()) {
        qDebug() << "serviceData 为空";
        return;
    }
    double lat = data["latitude"].toString().toDouble();
    double lon = data["longitude"].toString().toDouble();
    if (lat == 0.0 && lon == 0.0) {
        qDebug() << "经纬度为0，忽略";
        return;
    }

    double speed = data["speed"].toDouble(0.0);
    qDebug() << "车辆 ID:" << id << "纬度:" << lat << "经度:" << lon << "速度:" << speed;

    // 更新数据结构
    VehicleInfo info;
    info.id = id;
    info.latitude = lat;
    info.longitude = lon;
    info.speed = speed;
    info.online = true;
    info.lastUpdate = QDateTime::currentSecsSinceEpoch();
    vehicles[id] = info;

    updateVehicleOnMap(id, lat, lon);
    updateVehicleList();
}

void MainWindow::updateVehicleOnMap(const QString &id, double lat, double lon)
{
    qDebug() << "更新地图：" << id << lat << lon;
    QString js = QString("updateVehiclePosition('%1', %2, %3);")
                    .arg(id).arg(lat, 0, 'f', 8).arg(lon, 0, 'f', 8);
    ui->webView->page()->runJavaScript(js);
}
void MainWindow::removeVehicleFromMap(const QString &id)
{
    QString js = QString("removeVehicle('%1');").arg(id);
    ui->webView->page()->runJavaScript(js);
}

void MainWindow::onTimerCheckTimeout()
{
    qint64 now = QDateTime::currentSecsSinceEpoch();
    QMutableMapIterator<QString, VehicleInfo> it(vehicles);
    bool changed = false;
    while (it.hasNext()) {
        it.next();
        if (now - it.value().lastUpdate > 10) {
            // 超时，移除地图标记
            removeVehicleFromMap(it.key());
            // 从容器中移除该车辆数据
            it.remove();   // 正确写法
            changed = true;
        }
    }
    if (changed) {
        updateVehicleList();
    }
}

void MainWindow::updateVehicleList()
{
    qDebug() << "更新车辆列表，数量:" << vehicles.size();
    ui->listWidget_vehicles->clear();
    for (auto it = vehicles.constBegin(); it != vehicles.constEnd(); ++it) {
        const VehicleInfo &info = it.value();
        QString status = info.online ? "在线" : "离线";
        QString text = QString("%1 | %2 | 速度: %3 m/s | 经:%4 纬:%5")
                          .arg(info.id)
                          .arg(status)
                          .arg(info.speed, 0, 'f', 2)
                          .arg(info.longitude, 0, 'f', 6)
                          .arg(info.latitude, 0, 'f', 6);
        ui->listWidget_vehicles->addItem(text);
    }
}
// 保留其他槽函数（如上传数据等）可根据需要实现
void MainWindow::on_btn_push_data_clicked()
{
    // 如果有上传功能可自行实现（但本任务无需上传）
}

//void MainWindow::on_pushButton_connect_clicked()
//{

//}
