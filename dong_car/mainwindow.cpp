#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>



//#include "QString"
//#include "iostream"
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
{
    ui->setupUi(this);

    timeTimer = new QTimer(this);
    connect(timeTimer, &QTimer::timeout, this, &MainWindow::updateTime);
    timeTimer->start(1000);

    pushdata_timer = new QTimer(this);
    connect(pushdata_timer, &QTimer::timeout, this, &MainWindow::update1Time);
    //pushdata_timer->start(2000);
       pushdata_timer->stop();

//    m_timerRefresh = new QTimer(this);
//    m_timerRefresh->setInterval(5000); // 5000ms = 5stimeTimer
//    connect(m_timerRefresh, &QTimer::timeout, this, &MainWindow::timerRefreshInfo);
}

//void MainWindow::timerRefreshInfo()
//{
//    refreshAnd();
//}

MainWindow::~MainWindow()
{
    delete ui;
}

//--------------------page change-----------------------//
//GO to page 1
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
//------------------------------------------------------//

//-------------------TIME-----------------------//
void MainWindow::updateTime()
{

    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString showText = QString("TIME：%1").arg(currentTime);

    ui->label_time1->setText(showText);
    ui->label_time2->setText(showText);
    ui->label_time3->setText(showText);

//    QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
//    ui->textEdit_log->append(QString("[%1]suscced upload").arg(time));

//    QString ID = ui->lineEdit->text();
//    ui->textEdit_log->append(QString("[%1]ID：%2").arg(time).arg(ID));

//    QString Lat = ui->LinEdit_latitude->text();
//    ui->textEdit_log->append(QString("[%1]Latitude：%2").arg(time).arg(Lat));

//    QString aut = ui->lineEdit_longitude->text();
//    ui->textEdit_log->append(QString("[%1]Longitude：%2").arg(time).arg(aut));
}
void MainWindow::update1Time()
{
        QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
        ui->textEdit_log->append(QString("[%1]suscced upload").arg(time));

        QString ID = ui->lineEdit->text();
        ui->textEdit_log->append(QString("[%1]ID：%2").arg(time).arg(ID));


        QString Lat = ui->LinEdit_latitude->text();
//        in_lAt = Qint(Lat);
//        int in_lAt = ui->LinEdit_latitude->value();
        int in_lAt =Lat.toInt();
        if(in_lAt != 140)
        {
        ui->textEdit_log->append(QString("[%1]Latitude：%2").arg(time).arg(Lat));
        }
        else {
        ui->textEdit_log->append(QString("[%1]Latitude：Alarm").arg(time));
        }

        QString aut = ui->lineEdit_longitude->text();
        ui->textEdit_log->append(QString("[%1]Longitude：%2").arg(time).arg(aut));
}

void MainWindow::on_btn_connect_server_clicked()
{

        QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
        ui->textEdit_log->append(QString("[%1]server connect").arg(time));
//        QString data = ui->lineEdit->text();
//        ui->textEdit_log->append(QString("[%1]server connect，ID：%2").arg(time).arg(data));

}

void MainWindow::on_btn_push_data_clicked()
{
    if (!m_refreshRunning)
        {
            pushdata_timer->start(2000);
            m_refreshRunning = true;
            ui->textEdit_log->append("open 1s");
            //ui->btnRefreshStat->setText("stop");
        }
        else
        {
            pushdata_timer->stop();
            m_refreshRunning = false;
            ui->textEdit_log->append("close 1s");
          //  ui->btnRefreshStat->setText("开启自动刷新");
        }
//    QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
//    ui->textEdit_log->append(QString("[%1]suscced upload").arg(time));

//    QString ID = ui->lineEdit->text();
//    ui->textEdit_log->append(QString("[%1]ID：%2").arg(time).arg(ID));

//    QString Lat = ui->LinEdit_latitude->text();
//    ui->textEdit_log->append(QString("[%1]Latitude：%2").arg(time).arg(Lat));

//    QString aut = ui->lineEdit_longitude->text();
//    ui->textEdit_log->append(QString("[%1]Longitude：%2").arg(time).arg(aut));

//    QString data = ui->lineEdit->text();
//    ui->textEdit_log->append(QString("[%1]server connect，ID：%2").arg(time).arg(data));

//    QString data = ui->lineEdit->text();
//    ui->textEdit_log->append(QString("[%1]server connect，ID：%2").arg(time).arg(data));
}

void MainWindow::on_btn_clear_log_clicked()
{
    ui->textEdit_log->clear();
}

void MainWindow::on_comboBox_4_activated(const QString &arg1)
{
    QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
    int idx = ui->comboBox_4->currentIndex();
       if(idx == 0)
        {
            ui->textEdit_log->append(QString("[%1]BeiDouposition : online").arg(time));
        }
        else if(idx == 1)
        {
            ui->textEdit_log->append(QString("[%1]BeiDouposition : Outline").arg(time));
        }
}


void MainWindow::on_checkBox_3_stateChanged(int arg1)
{
      QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
        if (arg1 == Qt::Checked)
        {
            ui->textEdit_log->append(QString("[%1] open lowpoer alarm").arg(time));
        }
        else if (arg1 == Qt::Unchecked)
        {
            ui->textEdit_log->append(QString("[%1]  close lowpoer alarm").arg(time));
        }
}

void MainWindow::on_horizontalSlider_actionTriggered(int action)
{
    QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
        int sliderVal = ui->horizontalSlider->value();
        ui->textEdit_log->append(QString("[%1]speed %2").arg(time).arg(sliderVal));
}

//-------------------------MQTT----------------------------------//
//MqttClient::MqttClient(const char* clientId, QTextEdit* logWidget)
//    : mosqpp::mosquittopp(clientId), m_log(logWidget)
//{
//    username_pw_set("admin", "public");
//}
























