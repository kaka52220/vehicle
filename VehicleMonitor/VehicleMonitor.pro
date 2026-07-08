QT += core widgets webenginewidgets

CONFIG += c++11

TARGET = VehicleMonitor
TEMPLATE = app

# 添加 mosquittopp 库
LIBS += -lmosquittopp

SOURCES += main.cpp mainwindow.cpp
HEADERS += mainwindow.h
FORMS += mainwindow.ui

# 资源文件
RESOURCES += \
    res.qrc
RESOURCES += res.qrc
CONFIG += console
