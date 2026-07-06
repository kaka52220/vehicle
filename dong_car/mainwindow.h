#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <mosquittopp.h>
#include <QTimer>
#include <QDateTime>
class MqttClient;  // 前向声明
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateTime();
    void update1Time();

    void on_btn_page1_clicked();

    void on_btn_page2_clicked();

    void on_btn_page3_clicked();

    void on_btn_connect_server_clicked();

    void on_btn_push_data_clicked();

    void on_btn_clear_log_clicked();

    void on_comboBox_4_activated(const QString &arg1);

    void on_checkBox_3_stateChanged(int arg1);

    void on_horizontalSlider_actionTriggered(int action);

   // void timerRefreshInfo();
private:
    Ui::MainWindow *ui;
    QTimer *timeTimer;
    QTimer *pushdata_timer;
//    QTimer *m_timerRefresh;

    bool m_refreshRunning;
//    void refreshAnd();
    #define mqtt_server_address "10.134.2.113"
    #define mqtt_port "1884"
    //#define mqtt_topic " "
/*    Q_DISABLE_COPY(QmlMqttClie
 * nt)
    QMqttClient m_client*/;
    MqttClient *m_mqttClient;
    bool m_isConnected;
} ;
#endif // MAINWINDOW_H
