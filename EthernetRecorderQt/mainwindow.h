#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "packetparser.h"

#include <QMainWindow>
#include <QStatusBar>
#include <QLineEdit>
#include <QPushButton>
#include <QSerialPort>
#include <QLabel>

#include <chrono>


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void statusCheck();

    void buttonStartClicked();

    void resetStat();

    void tryOpeningComPort();

    void closeComPort();

    void comPortReadyRead();

    void error(const QString& msg);

    PacketParser packetParser_;

    QStatusBar* statusBar_ = nullptr;
    QLineEdit* editComPort_ = nullptr;

    QLabel* labelComPortStatus_ = nullptr;
    QLabel* labelDuration_ = nullptr;
    QLabel* labelBytesReceived_ = nullptr;
    QLabel* labelDataSpeed_ = nullptr;
    QLabel* labelPacketsReceived_ = nullptr;
    QLabel* labelErrorBytes_ = nullptr;

    QPushButton* buttonStart_ = nullptr;
    std::vector<QWidget*> widgetsEnabledAtConfig_;

    bool isRunning_{false};
    QSerialPort* comPort_ = nullptr;

    size_t bytesReceived_{0};
    std::chrono::steady_clock::time_point firstRxTime_;
};
#endif // MAINWINDOW_H
