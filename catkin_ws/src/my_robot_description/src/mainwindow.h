#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QDateTime>
#include <QTextEdit>
#include <QObject>
#include <mosquittopp.h>
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// ------------------------------------------------------------
// MQTT 客户端类（继承 QObject 和 mosquittopp）
// ------------------------------------------------------------
class MqttClient : public QObject, public mosqpp::mosquittopp
{
    Q_OBJECT
public:
    // 删掉 QTextEdit* 入参，不再传UI控件
    explicit MqttClient(const char *id = "qt_client", QObject *parent = nullptr);
    ~MqttClient();

    bool connectToBroker(const QString &host, int port, int keepalive = 60);
    bool publishMessage(const QString &topic, const QString &payload, int qos = 1, bool retain = false);
    bool isConnected() const { return m_connected; }

protected:
    void on_connect(int rc) override;
    void on_disconnect(int rc) override;
    void on_publish(int mid) override;

signals:
    void connected();
    void disconnected();
    void messagePublished(int mid);
    void logMsg(QString text); // 新增：日志信号，主线程接收更新UI

private:
    bool m_connected = false; // 仅保存连接状态，不存UI指针
};

// ------------------------------------------------------------
// 主窗口类
// ------------------------------------------------------------
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateTime();
    void update1Time();
    //--------------------------------------------------------------//
    void updateLatLon(double lat, double lon);//ROS NEW add
    //--------------------------------------------------------------//
    void on_btn_page1_clicked();
    void on_btn_page2_clicked();
    void on_btn_page3_clicked();
    void on_btn_connect_server_clicked();
    void on_btn_push_data_clicked();
    void on_btn_clear_log_clicked();
    void on_checkBox_lowpower_stateChanged(int arg1);
    void on_horizontalSlider_actionTriggered(int action);

    void on_comboBox_BeiDou_activated(const QString &arg1);

private:
    Ui::MainWindow *ui;
    QTimer *timeTimer;
    QTimer *pushdata_timer;
    bool m_refreshRunning = false;

    MqttClient *mqttClient;   // MQTT 客户端对象
};

#endif // MAINWINDOW_H
