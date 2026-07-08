#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWebEngineView>
#include <QTimer>
#include <QMap>
#include <mosquittopp.h>
#include <QWebEngineSettings>
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct VehicleInfo {
    QString id;
    double latitude;
    double longitude;
    double speed;
    bool online;
    qint64 lastUpdate;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btn_connect_server_clicked();
    void on_btn_push_data_clicked();   // 如果仍有上传功能，可保留
    void onTimerCheckTimeout();
    void updateVehicleList();
  void parseJsonAndUpdate(const QString &json, const QString &topicId);
  //  void on_pushButton_connect_clicked();

private:
    Ui::MainWindow *ui;
    //QWebEngineView *webView;
    QTimer *offlineTimer;

    // MQTT 客户端（使用 mosquittopp）
    class MqttClient : public mosqpp::mosquittopp {
    public:
        MqttClient(MainWindow* parent);
        ~MqttClient();
        bool connectToBroker(const QString &host, int port);
        bool subscribeTopic(const QString &topic, int qos=1);
        bool publishMessage(const QString &topic, const QString &payload, int qos=1);
        void on_connect(int rc) override;
        void on_disconnect(int rc) override;
        void on_message(const struct mosquitto_message *message) override;
    private:
        MainWindow* m_parent;
    };

    MqttClient* mqttClient;
    QMap<QString, VehicleInfo> vehicles;

    //void parseJsonAndUpdate(const QString &json, const QString &id);
    void updateVehicleOnMap(const QString &id, double lat, double lon);
    void removeVehicleFromMap(const QString &id);

    // 用于线程安全更新 UI
    void doUpdateVehicleList();
    void doUpdateMap(const QString &id, double lat, double lon);
    void doRemoveFromMap(const QString &id);
};

#endif // MAINWINDOW_H
