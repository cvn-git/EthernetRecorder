#include "mainwindow.h"

#include <QBoxLayout>
#include <QGridLayout>
#include <QToolBar>
#include <QGroupBox>
#include <QMessageBox>
#include <QTimer>
#include <QDebug>


namespace {

void addListItem(QGridLayout* layout, const QString& label, QWidget* widget)
{
    auto rowIdx = layout->rowCount();

    auto labelWidget = new QLabel(label);
    labelWidget->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(labelWidget, rowIdx, 0);

    widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout->addWidget(widget, rowIdx, 1);
}

}   // anonymous namespace


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Main widget
    auto mainWidget = new QWidget();
    setCentralWidget(mainWidget);
    auto mainLayout = new QVBoxLayout(mainWidget);

    // Status bar
    auto toolBar = new QToolBar();
    addToolBar(Qt::BottomToolBarArea, toolBar);

    statusBar_ = new QStatusBar();
    toolBar->addWidget(statusBar_);

    // Config items
    auto groupConfig = new QGroupBox(tr("Configuration"));
    mainLayout->addWidget(groupConfig);
    auto layoutConfig = new QGridLayout(groupConfig);

    editComPort_ = new QLineEdit("COM5");
    addListItem(layoutConfig, "COM port:", editComPort_);
    widgetsEnabledAtConfig_.push_back(editComPort_);

    // Stat items
    auto groupStat = new QGroupBox(tr("Statistics"));
    mainLayout->addWidget(groupStat);
    auto layoutStat = new QGridLayout(groupStat);

    labelComPortStatus_ = new QLabel();
    addListItem(layoutStat, tr("COM port:"), labelComPortStatus_);

    labelDuration_ = new QLabel();
    addListItem(layoutStat, tr("Duration (s):"), labelDuration_);

    labelBytesReceived_ = new QLabel();
    addListItem(layoutStat, tr("Bytes received:"), labelBytesReceived_);

    labelDataSpeed_ = new QLabel();
    addListItem(layoutStat, tr("Data speed (KB/s):"), labelDataSpeed_);

    labelPacketsReceived_ = new QLabel();
    addListItem(layoutStat, tr("Packets received:"), labelPacketsReceived_);

    labelErrorBytes_ = new QLabel();
    addListItem(layoutStat, tr("Error bytes:"), labelErrorBytes_);

    // Start button
    buttonStart_ = new QPushButton(tr("Start"));
    mainLayout->addWidget(buttonStart_);
    connect(buttonStart_, &QPushButton::clicked, this, &MainWindow::buttonStartClicked);

    // Padding widget
    auto paddingWidget = new QWidget();
    paddingWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    mainLayout->addWidget(paddingWidget);

    // Status timer
    auto timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::statusCheck);
    timer->start(1000);

    if constexpr(sizeof(EthRecHeader) != ETH_REC_HEADER_BYTES)
    {
        error(tr("Invalid message header size"));
    }
}

MainWindow::~MainWindow()
{
    if (isRunning_)
    {
        buttonStartClicked();
    }
}

void MainWindow::statusCheck()
{
    if (isRunning_)
    {
        if (comPort_ == nullptr)
        {
            tryOpeningComPort();
        }
        else
        {
            // Check if the port is still open
            if (!comPort_->isOpen())
            {
                closeComPort();
            }
        }
    }

    if (comPort_ == nullptr)
    {
        labelComPortStatus_->setText(tr("Disconnected"));
    }
    else
    {
        double speed = 0;
        double duration = 0;
        if (bytesReceived_ > 0)
        {
            duration = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - firstRxTime_).count();
            speed = bytesReceived_ / (duration * 1024);
        }

        //statusBarComPort_->showMessage(tr("COM port connected: %1 byte(s) received (%2 KB/s)").arg(bytesReceived_).arg(speed, 0, 'f', 1));
        labelComPortStatus_->setText(tr("Connected"));
        labelDuration_->setText(tr("%1").arg(duration, 0, 'f', 1));
        labelBytesReceived_->setText(QString::number(bytesReceived_));
        labelDataSpeed_->setText(tr("%1").arg(speed, 0, 'f', 1));
        labelPacketsReceived_->setText(QString::number(packetParser_.receivedPackets()));
        labelErrorBytes_->setText(QString::number(packetParser_.errorBytes()));
    }
}

void MainWindow::buttonStartClicked()
{
    for (auto widget : widgetsEnabledAtConfig_)
    {
        widget->setEnabled(isRunning_);
    }

    if (!isRunning_)
    {
        buttonStart_->setText(tr("Stop"));
        isRunning_ = true;

        statusBar_->showMessage("Recording started");
    }
    else
    {
        buttonStart_->setText(tr("Start"));
        closeComPort();
        isRunning_ = false;

        statusBar_->showMessage("Recording stopped");
    }
}

void MainWindow::resetStat()
{
    packetParser_.reset();
    bytesReceived_ = 0;
}

void MainWindow::tryOpeningComPort()
{
    if (comPort_ != nullptr)
    {
        return;
    }

    comPort_ = new QSerialPort(this);
    comPort_->setPortName(editComPort_->text());
#if 0
    comPort_->setBaudRate(115200);
    comPort_->setDataBits(QSerialPort::Data8);
    comPort_->setStopBits(QSerialPort::OneStop);
    comPort_->setParity(QSerialPort::NoParity);
    comPort_->setFlowControl(QSerialPort::SoftwareControl);
#endif
    if (comPort_->open(QIODevice::ReadWrite))
    {
        connect(comPort_, &QSerialPort::readyRead, this, &MainWindow::comPortReadyRead);
        connect(comPort_, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError err){
            if (err && comPort_)
            {
                error(tr("COM port error #%1: %2").arg(err).arg(comPort_->errorString()));
                closeComPort();
            }
        });
        comPort_->setDataTerminalReady(true);

        resetStat();
    }
    else
    {
        comPort_->deleteLater();
        comPort_ = nullptr;
    }
}

void MainWindow::closeComPort()
{
    if (comPort_ != nullptr)
    {
        comPort_->deleteLater();
        comPort_ = nullptr;
    }
}

void MainWindow::comPortReadyRead()
{
    if (comPort_ == nullptr)
    {
        return;
    }

    while (comPort_->bytesAvailable() > 0)
    {
        auto data = comPort_->readAll();
        if (data.isEmpty())
        {
            return;
        }

#if 0
        qDebug() << "Receive " << data.size() << " bytes";
        qDebug() << QString::fromUtf8(data);
#endif

        if (bytesReceived_ == 0)
        {
            firstRxTime_ = std::chrono::steady_clock::now();
        }
        bytesReceived_ += data.size();

        packetParser_.parseRawStream(data);
    }
}

void MainWindow::error(const QString& msg)
{
    QMessageBox::critical(this, tr("Error"), msg);
}
