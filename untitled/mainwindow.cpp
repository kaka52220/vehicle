#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 初始化运行日志
    ui->textEdit->append("[10:00:01] 系统启动.");
    ui->textEdit->append("[10:00:02] 初始化完成");

    // 下拉框预设选项
    ui->comboBox->addItems({"无人车","无人机"});
    ui->comboBox_2->addItems({"在线","离线"});
    ui->comboBox_3->addItems({"在线","离线"});
    ui->comboBox_4->addItems({"在线","离线"});

    // 电量范围0~100，默认99
    ui->spinBox->setRange(0,100);
    ui->spinBox->setValue(99);

    // 初始化输入框默认值
    ui->lineEdit->setText("XS09290320");
    ui->lineEdit_2->setText("114.53435053");
    ui->lineEdit_3->setText("30.39655027");
    // 速度框（如果你的速度框是lineEdit_4）
    ui->lineEdit_4->setText("3.5");
}

MainWindow::~MainWindow()
{
    delete ui;
}

// 1. 连接服务器按钮
void MainWindow::on_pushButton_clicked()
{
    QString t = QDateTime::currentDateTime().toString("hh:mm:ss");
    ui->textEdit->append(QString("[%1] 服务器连接成功").arg(t));
}

// 2. 上传数据按钮
void MainWindow::on_pushButton_2_clicked()
{
    QString t = QDateTime::currentDateTime().toString("hh:mm:ss");

    // 读取终端信息
    QString id = ui->lineEdit->text();
    QString type = ui->comboBox->currentText();
    QString lon = ui->lineEdit_2->text();
    QString lat = ui->lineEdit_3->text();

    // 传感器状态
    QString lidar = ui->comboBox_2->currentText();
    QString cam = ui->comboBox_3->currentText();
    QString bds = ui->comboBox_4->currentText();
    int power = ui->spinBox->value();
    // 速度框用lineEdit_4，如果你速度框不是这个，改成你对应的名字
    QString speed = ui->lineEdit_4->text();

    // 报警信息
    QString alarmText;
    if(ui->checkBox->isChecked())    alarmText += "超速报警 ";
    if(ui->checkBox_2->isChecked())  alarmText += "偏离路线 ";
    if(ui->checkBox_3->isChecked())  alarmText += "电量过低 ";
    if(alarmText.isEmpty()) alarmText = "无告警";

    // 拼接日志
    QString logInfo = QString(
    "[%1]终端ID:%2 终端类型:%3 经度:%4 纬度:%5\n"
    "激光雷达:%6 摄像头:%7 北斗:%8 电量:%9%% 速度:%10m/s\n"
    "报警信息:%11\n")
    .arg(t).arg(id).arg(type).arg(lon).arg(lat)
    .arg(lidar).arg(cam).arg(bds).arg(power).arg(speed).arg(alarmText);

    ui->textEdit->append(logInfo);
}

// 3. 清空日志按钮
void MainWindow::on_pushButton_3_clicked()
{
    ui->textEdit->clear();
    QString t = QDateTime::currentDateTime().toString("hh:mm:ss");
    ui->textEdit->append(QString("[%1] 日志已清空").arg(t));
}
