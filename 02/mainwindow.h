#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QResizeEvent>
#include <QShowEvent>

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

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void on_pushButton_clicked();      // 连接服务器
    void on_pushButton_2_clicked();    // 手动上传数据
    void on_pushButton_3_clicked();    // 清空日志
    void on_pushButton_4_clicked();    // 持续上传
    void on_pushButton_5_clicked();    // 停止上传
    void onAutoUpload();               // 自动上传


private:
    Ui::MainWindow *ui;
    QTimer *m_timer;
    bool m_isAutoUploading;

    struct WidgetGeometry {
        QRect geometry;
        QFont font;
    };
    QMap<QString, WidgetGeometry> m_initialGeometries;
    bool m_initialized;

    // MQTT 相关
    MqttClient *m_mqttClient;
    bool m_isConnected;

    void appendLog(const QString &message);
    void uploadData();
    void updateWidgetSize();
    void saveInitialGeometries();
    QByteArray packJsonData();  // 打包 JSON 数据
};

#endif // MAINWINDOW_H
