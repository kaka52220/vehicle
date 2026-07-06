#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QDebug>
#include <QTextCursor>
#include <QPushButton>
#include <QApplication>
#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QShowEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <mosquittopp.h>
#include <cstring>

// =====================================================
// MQTT 客户端类（继承 mosquittopp）
// =====================================================
class MqttClient : public mosqpp::mosquittopp
{
public:
    MqttClient(const char *id) : mosqpp::mosquittopp(id) {}

    void on_connect(int rc) override
    {
        if (rc == 0) {
            qDebug() << "✅ MQTT 连接成功";
        } else {
            qDebug() << "❌ MQTT 连接失败，错误码:" << rc;
        }
    }

    void on_disconnect(int rc) override
    {
        qDebug() << "⚠️ MQTT 已断开连接";
    }

    void on_publish(int mid) override
    {
        qDebug() << "📤 消息已发布，mid:" << mid;
    }
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_timer(nullptr)
    , m_isAutoUploading(false)
    , m_initialized(false)
    , m_mqttClient(nullptr)
    , m_isConnected(false)
{
    ui->setupUi(this);

    // ===== 初始化 mosquitto 库 =====
    mosqpp::lib_init();

    setWindowTitle("🚗 无人车数据监测系统");
    setMinimumSize(400, 350);

    // ===== 初始化下拉框 =====
    ui->comboBox->addItems({"🚗 无人车", "🚁 无人机"});
    ui->comboBox_2->addItems({"🟢 在线", "🔴 离线"});
    ui->comboBox_3->addItems({"🟢 在线", "🔴 离线"});
    ui->comboBox_4->addItems({"🟢 在线", "🔴 离线"});
    ui->comboBox_5->addItems({"🟢 正常运行", "🟡 检测中", "🔴 异常"});

    // ===== 设置电量范围 =====
    ui->spinBox->setRange(0, 100);
    ui->spinBox->setValue(100);

    // ===== 设置欧拉角范围 =====
    ui->doubleSpinBox->setRange(-180, 180);
    ui->doubleSpinBox_2->setRange(-180, 180);
    ui->doubleSpinBox_3->setRange(-180, 180);

    // ===== 设置默认值 =====
    ui->lineEdit->setText("wlw20230303002 008");
    ui->lineEdit_2->setText("114.53435053");
    ui->lineEdit_3->setText("30.39655027");
    ui->lineEdit_4->setText("3.5");

    // ===== 初始化定时器 =====
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &MainWindow::onAutoUpload);

    // ===== 设置按钮文字 =====
    ui->pushButton_4->setText("▶️ 持续上传");
    ui->pushButton_5->setText("⏹️ 停止上传");

    // ===== 设置按钮初始状态 =====
    ui->pushButton_4->setEnabled(false);  // 初始不可用，需要先连接服务器
    ui->pushButton_5->setEnabled(false);

    // ===== 设置按钮样式 =====
    ui->pushButton_4->setStyleSheet(
        "background-color: #95a5a6; color: white; font-weight: bold; border-radius: 5px;"
    );
    ui->pushButton_5->setStyleSheet(
        "background-color: #95a5a6; color: white; font-weight: bold; border-radius: 5px;"
    );
    ui->pushButton->setStyleSheet(
        "background-color: #3498db; color: white; font-weight: bold; border-radius: 5px;"
    );
    ui->pushButton_2->setStyleSheet(
        "background-color: #2ecc71; color: white; font-weight: bold; border-radius: 5px;"
    );
    ui->pushButton_3->setStyleSheet(
        "background-color: #95a5a6; color: white; font-weight: bold; border-radius: 5px;"
    );

    // ===== 初始日志 =====
    ui->textEdit->clear();
    appendLog("🚀 系统启动");
    appendLog("💡 初始化完成");
    appendLog("💡 EMQX 服务器: 10.134.2.113:1884");
    appendLog("💡 点击「连接服务器」连接到 EMQX");
    appendLog("💡 窗口可任意缩放，UI自动适配");

    // ===== 状态栏 =====
    statusBar()->showMessage("✅ 就绪");
}

MainWindow::~MainWindow()
{
    if (m_timer) {
        m_timer->stop();
    }

    // 断开 MQTT 连接
    if (m_mqttClient) {
        m_mqttClient->disconnect();
        delete m_mqttClient;
        m_mqttClient = nullptr;
    }

    mosqpp::lib_cleanup();
    delete ui;
}

// =====================================================
// 显示事件
// =====================================================
void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    if (!m_initialized) {
        saveInitialGeometries();
        m_initialized = true;
        updateWidgetSize();
    }
}

// =====================================================
// 保存所有控件的初始几何信息
// =====================================================
void MainWindow::saveInitialGeometries()
{
    QList<QGroupBox*> groupBoxes = this->findChildren<QGroupBox*>();
    foreach(QGroupBox *groupBox, groupBoxes) {
        WidgetGeometry info;
        info.geometry = groupBox->geometry();
        info.font = groupBox->font();
        m_initialGeometries[groupBox->objectName()] = info;
    }

    QList<QPushButton*> buttons = this->findChildren<QPushButton*>();
    foreach(QPushButton *button, buttons) {
        WidgetGeometry info;
        info.geometry = button->geometry();
        info.font = button->font();
        m_initialGeometries[button->objectName()] = info;
    }

    QList<QLabel*> labels = this->findChildren<QLabel*>();
    foreach(QLabel *label, labels) {
        WidgetGeometry info;
        info.geometry = label->geometry();
        info.font = label->font();
        m_initialGeometries[label->objectName()] = info;
    }

    QList<QComboBox*> comboBoxes = this->findChildren<QComboBox*>();
    foreach(QComboBox *comboBox, comboBoxes) {
        WidgetGeometry info;
        info.geometry = comboBox->geometry();
        info.font = comboBox->font();
        m_initialGeometries[comboBox->objectName()] = info;
    }

    QList<QLineEdit*> lineEdits = this->findChildren<QLineEdit*>();
    foreach(QLineEdit *lineEdit, lineEdits) {
        WidgetGeometry info;
        info.geometry = lineEdit->geometry();
        info.font = lineEdit->font();
        m_initialGeometries[lineEdit->objectName()] = info;
    }

    QList<QSpinBox*> spinBoxes = this->findChildren<QSpinBox*>();
    foreach(QSpinBox *spinBox, spinBoxes) {
        WidgetGeometry info;
        info.geometry = spinBox->geometry();
        info.font = spinBox->font();
        m_initialGeometries[spinBox->objectName()] = info;
    }

    QList<QDoubleSpinBox*> doubleSpinBoxes = this->findChildren<QDoubleSpinBox*>();
    foreach(QDoubleSpinBox *doubleSpinBox, doubleSpinBoxes) {
        WidgetGeometry info;
        info.geometry = doubleSpinBox->geometry();
        info.font = doubleSpinBox->font();
        m_initialGeometries[doubleSpinBox->objectName()] = info;
    }

    QList<QCheckBox*> checkBoxes = this->findChildren<QCheckBox*>();
    foreach(QCheckBox *checkBox, checkBoxes) {
        WidgetGeometry info;
        info.geometry = checkBox->geometry();
        info.font = checkBox->font();
        m_initialGeometries[checkBox->objectName()] = info;
    }

    if (ui->textEdit) {
        WidgetGeometry info;
        info.geometry = ui->textEdit->geometry();
        info.font = ui->textEdit->font();
        m_initialGeometries["textEdit"] = info;
    }
}

// =====================================================
// 窗口缩放事件
// =====================================================
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (m_initialized) {
        updateWidgetSize();
    }
}

// =====================================================
// 更新所有控件大小
// =====================================================
void MainWindow::updateWidgetSize()
{
    if (!m_initialized || m_initialGeometries.isEmpty()) {
        return;
    }

    int currentWidth = this->width();
    int currentHeight = this->height();

    int baseWidth = 793;
    int baseHeight = 755;

    // 分别计算宽高缩放比例
    float widthScale = (float)currentWidth / baseWidth;
    float heightScale = (float)currentHeight / baseHeight;

    // 分别限制宽高缩放范围
    widthScale = qMax(0.5f, qMin(2.5f, widthScale));
    heightScale = qMax(0.5f, qMin(2.5f, heightScale));

    // 计算字体大小（使用较小的缩放比例，防止字体过大）
    float fontScale = qMin(widthScale, heightScale);
    int baseFontSize = 9;
    int newFontSize = (int)(baseFontSize * fontScale);
    newFontSize = qMax(6, qMin(20, newFontSize));

    // 遍历控件，分别应用宽高缩放
    QMap<QString, WidgetGeometry>::const_iterator i;
    for (i = m_initialGeometries.constBegin(); i != m_initialGeometries.constEnd(); ++i) {
        QString objName = i.key();
        WidgetGeometry initInfo = i.value();

        QRect newRect(
            (int)(initInfo.geometry.x() * widthScale),
            (int)(initInfo.geometry.y() * heightScale),
            (int)(initInfo.geometry.width() * widthScale),
            (int)(initInfo.geometry.height() * heightScale)
        );

        QWidget *widget = this->findChild<QWidget*>(objName);
        if (widget) {
            widget->setGeometry(newRect);
            QFont font = initInfo.font;
            font.setPointSize(newFontSize);
            widget->setFont(font);
        }
    }

    // ===== 手动调整操作区按钮布局 =====
    // 使用宽高缩放的平均值或较小值来调整按钮位置
    float btnScale = qMin(widthScale, heightScale);

    if (ui->groupBox_3) {
        ui->pushButton->setGeometry((int)(20 * btnScale), (int)(30 * btnScale),
                                    (int)(89 * btnScale), (int)(25 * btnScale));
        ui->pushButton_2->setGeometry((int)(120 * btnScale), (int)(30 * btnScale),
                                      (int)(89 * btnScale), (int)(25 * btnScale));
        ui->pushButton_4->setGeometry((int)(220 * btnScale), (int)(30 * btnScale),
                                      (int)(89 * btnScale), (int)(25 * btnScale));
        ui->pushButton_5->setGeometry((int)(320 * btnScale), (int)(30 * btnScale),
                                      (int)(89 * btnScale), (int)(25 * btnScale));
        ui->pushButton_3->setGeometry((int)(420 * btnScale), (int)(30 * btnScale),
                                      (int)(89 * btnScale), (int)(25 * btnScale));
    }

    // 样式表
    QString styleSheet = QString(
        "QMainWindow, QWidget {"
        "   font-size: %1px;"
        "}"
        "QGroupBox {"
        "   font-size: %1px;"
        "   font-weight: bold;"
        "}"
        "QPushButton {"
        "   font-size: %1px;"
        "   padding: %2px %3px;"
        "   border-radius: %4px;"
        "}"
        "QLabel {"
        "   font-size: %1px;"
        "}"
        "QComboBox {"
        "   font-size: %1px;"
        "   padding: %2px %3px;"
        "}"
        "QLineEdit {"
        "   font-size: %1px;"
        "   padding: %2px %3px;"
        "}"
        "QSpinBox {"
        "   font-size: %1px;"
        "}"
        "QDoubleSpinBox {"
        "   font-size: %1px;"
        "}"
        "QTextEdit {"
        "   font-size: %1px;"
        "}"
        "QCheckBox {"
        "   font-size: %1px;"
        "}"
    ).arg(newFontSize)
     .arg((int)(4 * fontScale))
     .arg((int)(8 * fontScale))
     .arg((int)(5 * fontScale));

    this->setStyleSheet(styleSheet);

    statusBar()->showMessage(QString("窗口: %1x%2 | 宽缩放: %3% | 高缩放: %4% | 字体: %5px")
                            .arg(currentWidth)
                            .arg(currentHeight)
                            .arg((int)(widthScale * 100))
                            .arg((int)(heightScale * 100))
                            .arg(newFontSize));
}
// =====================================================
// 1. 连接服务器（连接到 EMQX）
// =====================================================
void MainWindow::on_pushButton_clicked()
{
    if (m_isConnected) {
        appendLog("⚠️ 已经连接到服务器");
        return;
    }

    appendLog("🔄 正在连接 EMQX 服务器...");
    appendLog("   📍 服务器: 10.134.2.113:1884");

    // 获取设备ID作为客户端ID
    QString deviceId = ui->lineEdit->text().trimmed();
    if (deviceId.isEmpty()) {
        appendLog("❌ 错误: 设备ID不能为空");
        return;
    }

    // 创建 MQTT 客户端
    try {
        m_mqttClient = new MqttClient(deviceId.toStdString().c_str());

        // 连接到 EMQX 服务器
        int rc = m_mqttClient->connect("10.134.2.113", 1884, 60);

        if (rc == MOSQ_ERR_SUCCESS) {
            m_isConnected = true;
            appendLog("✅ EMQX 服务器连接成功");
            appendLog("   📱 客户端ID: " + deviceId);

            // 启用上传按钮
            ui->pushButton_4->setEnabled(true);
            ui->pushButton_4->setStyleSheet(
                "background-color: #f39c12; color: white; font-weight: bold; border-radius: 5px;"
            );
            ui->pushButton->setStyleSheet(
                "background-color: #2ecc71; color: white; font-weight: bold; border-radius: 5px;"
            );
            ui->pushButton->setText("✅ 已连接");

            statusBar()->showMessage("✅ 已连接到 EMQX 服务器");
        } else {
            appendLog("❌ EMQX 连接失败，错误码: " + QString::number(rc));
            delete m_mqttClient;
            m_mqttClient = nullptr;
        }
    } catch (const std::exception &e) {
        appendLog("❌ 连接异常: " + QString(e.what()));
        m_mqttClient = nullptr;
    }
}

// =====================================================
// 2. 手动上传数据
// =====================================================
void MainWindow::on_pushButton_2_clicked()
{
    if (!m_isConnected || !m_mqttClient) {
        appendLog("❌ 请先连接服务器");
        return;
    }
    uploadData();
}

// =====================================================
// 3. 清空日志
// =====================================================
void MainWindow::on_pushButton_3_clicked()
{
    ui->textEdit->clear();
    appendLog("🗑️ 日志已清空");
    statusBar()->showMessage("🗑️ 日志已清空");
}

// =====================================================
// 4. 开始持续上传 ⭐
// =====================================================
void MainWindow::on_pushButton_4_clicked()
{
    if (!m_isConnected || !m_mqttClient) {
        appendLog("❌ 请先连接服务器");
        return;
    }

    if (m_isAutoUploading) {
        appendLog("⚠️ 已经在持续上传中");
        return;
    }

    int interval = 100;  // 100ms = 0.1秒
    m_timer->start(interval);
    m_isAutoUploading = true;

    ui->pushButton_4->setEnabled(false);
    ui->pushButton_4->setStyleSheet(
        "background-color: #95a5a6; color: white; font-weight: bold; border-radius: 5px;"
    );
    ui->pushButton_5->setEnabled(true);
    ui->pushButton_5->setStyleSheet(
        "background-color: #e74c3c; color: white; font-weight: bold; border-radius: 5px;"
    );

    appendLog(QString("▶️ 开始持续上传（每 %1 秒一次）").arg(interval / 1000.0));
    statusBar()->showMessage("▶️ 持续上传中...");
}

// =====================================================
// 5. 停止上传 ⭐
// =====================================================
void MainWindow::on_pushButton_5_clicked()
{
    if (!m_isAutoUploading) {
        appendLog("⚠️ 当前没有在持续上传");
        return;
    }

    m_timer->stop();
    m_isAutoUploading = false;

    ui->pushButton_5->setEnabled(false);
    ui->pushButton_5->setStyleSheet(
        "background-color: #95a5a6; color: white; font-weight: bold; border-radius: 5px;"
    );
    ui->pushButton_4->setEnabled(true);
    ui->pushButton_4->setStyleSheet(
        "background-color: #f39c12; color: white; font-weight: bold; border-radius: 5px;"
    );

    appendLog("⏹️ 停止持续上传");
    statusBar()->showMessage("⏹️ 已停止");
}

// =====================================================
// 定时器自动上传
// =====================================================
void MainWindow::onAutoUpload()
{
    uploadData();
}

// =====================================================
// 打包 JSON 数据
// =====================================================
QByteArray MainWindow::packJsonData()
{
    QJsonObject json;

    // 获取当前时间戳（秒）
    json["reportTime"] = QString::number(QDateTime::currentSecsSinceEpoch());

    // 终端ID
    json["seriesNumber"] = ui->lineEdit->text().trimmed();

    // 终端类型：无人车为1，无人机为2
    QString deviceType = ui->comboBox->currentText();
    json["serviceType"] = deviceType.contains("无人车") ? "1" : "2";

    // 服务数据
    QJsonObject serviceData;

    // 激光雷达状态：在线=1，离线=0
    serviceData["lidarStatus"] = ui->comboBox_2->currentText().contains("在线") ? 1 : 0;

    // 摄像头状态：在线=1，离线=0
    serviceData["cameraStatus"] = ui->comboBox_3->currentText().contains("在线") ? 1 : 0;

    // 北斗定位状态：在线=1，离线=0
    serviceData["rtkStatus"] = ui->comboBox_4->currentText().contains("在线") ? 1 : 0;

    // 经纬度
    serviceData["latitude"] = ui->lineEdit_3->text().toDouble();
    serviceData["longitude"] = ui->lineEdit_2->text().toDouble();

    // 电量
    serviceData["power"] = ui->spinBox->value();

    // 报警状态（位运算）
    int alarmStatus = 0;
    if (ui->checkBox->isChecked()) alarmStatus |= 0x01;  // 超速报警
    if (ui->checkBox_2->isChecked()) alarmStatus |= 0x02; // 偏离路线
    if (ui->checkBox_3->isChecked()) alarmStatus |= 0x04; // 电量过低

    serviceData["alarmStatus"] = alarmStatus;

    json["serviceData"] = serviceData;

    // 服务类型
    json["serviceType"] = "updateCarStatus";

    QJsonDocument doc(json);
    return doc.toJson(QJsonDocument::Compact);
}

// =====================================================
// 独立的上传逻辑（手动和自动共用）
// =====================================================
void MainWindow::uploadData()
{
    if (!m_isConnected || !m_mqttClient) {
        appendLog("❌ 错误: 未连接到服务器");
        return;
    }

    QString deviceId = ui->lineEdit->text().trimmed();
    if (deviceId.isEmpty()) {
        appendLog("❌ 错误: 设备ID不能为空");
        return;
    }

    // ===== 1. 打包 JSON 数据 =====
    QByteArray jsonData = packJsonData();

    // ===== 2. 构建 Topic =====
    QString topic = "/thing/car/" + deviceId;

    // ===== 3. 发布消息 =====
    int rc = m_mqttClient->publish(NULL, topic.toStdString().c_str(),
                                    jsonData.size(), jsonData.data(), 1, false);

    if (rc == MOSQ_ERR_SUCCESS) {
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");

        if (m_isAutoUploading) {
            appendLog(QString("⏱️ [%1] 已上传 -> Topic: %2")
                      .arg(timestamp)
                      .arg(topic));
        } else {
            appendLog("─────────────────────────────────────");
            appendLog(QString("📤 上传数据 [%1]").arg(timestamp));
            appendLog(QString("   📱 设备ID: %1").arg(deviceId));
            appendLog(QString("   🚗 终端类型: %1").arg(ui->comboBox->currentText()));
            appendLog(QString("   📍 经度: %1").arg(ui->lineEdit_2->text()));
            appendLog(QString("   📍 纬度: %1").arg(ui->lineEdit_3->text()));
            appendLog(QString("   🔋 电量: %1%").arg(ui->spinBox->value()));
            appendLog(QString("   📡 激光雷达: %1").arg(ui->comboBox_2->currentText()));
            appendLog(QString("   📷 摄像头: %1").arg(ui->comboBox_3->currentText()));
            appendLog(QString("   🛰️ 北斗定位: %1").arg(ui->comboBox_4->currentText()));
            appendLog(QString("   📤 Topic: %1").arg(topic));
            appendLog(QString("   📦 JSON: %1").arg(QString(jsonData)));
            appendLog("─────────────────────────────────────");
        }

        statusBar()->showMessage(QString("📤 已上传到 %1").arg(topic));
    } else {
        appendLog("❌ 上传失败，错误码: " + QString::number(rc));
    }
}

// =====================================================
// 添加日志
// =====================================================
void MainWindow::appendLog(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    ui->textEdit->append(QString("[%1] %2").arg(timestamp).arg(message));

    QTextCursor cursor = ui->textEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->textEdit->setTextCursor(cursor);
}
