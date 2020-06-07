#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QMessageBox>
#include <QDebug>
#include <QTimer>
#include <iostream>
#include "darknet.h"
#include "network.h"
#include "opencv4/opencv2/opencv.hpp"
#include "image_opencv.h"
#include "imgformat.h"

using namespace cv;

// -----------------------------------------------------------------------------------------------------------------
// Define constants that were used when Darknet network was trained.
// This is pretty much hardcoded code zone, just to give an idea what is needed.
// -----------------------------------------------------------------------------------------------------------------

// Path to configuration file.
static char *cfg_file = const_cast<char *>("/home/jin/QTproject/QTserialComm/yolov3-tiny.cfg");
// Path to weight file.
static char *weight_file = const_cast<char *>("/home/jin/QTproject/QTserialComm/yolov3-tiny.weights");
// Path to a file describing classes names.
static char *names_file = const_cast<char *>("/home/jin/QTproject/QTserialComm/coco.names");
// This is an image to test.
static char *input = const_cast<char *>("/home/jin/QTproject/QTserialComm/dog.jpg");
// Define thresholds for predicted class.
static float thresh = 0.5;
static float hier_thresh = 0.5;
// Uncomment this if you need sort predicted result.
//    float nms = 0.45;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }

QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT

    struct Settings {
        QString name;
        qint32 baudRate;
        QString stringBaudRate;
        QSerialPort::DataBits dataBits;
        QString stringDataBits;
        QSerialPort::Parity parity;
        QString stringParity;
        QSerialPort::StopBits stopBits;
        QString stringStopBits;
        QSerialPort::FlowControl flowControl;
        QString stringFlowControl;
        bool localEchoEnabled;
    };

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openSerialPort();
    void closeSerialPort();
    void writeData(const QByteArray &data);
    void readData();

    void handleError(QSerialPort::SerialPortError error);

    void on_BTwriteData_clicked();

    void on_BTeraseWrite_clicked();

    void on_BTeraseRead_clicked();

    void slotTimerAlarm();

private:

    void showStatusMessage(const QString &message);

    Ui::MainWindow *ui;
    QSerialPort *m_serial;
    Settings p;
    bool read_Flag = false;
    QByteArray readAllData = NULL;
    QByteArray realData = NULL;
    QTimer *timer;

    // video
    VideoCapture    cap;
    Mat             frame;
    size_t          classes;
    char            **labels;
    network         *net;

};
#endif // MAINWINDOW_H
