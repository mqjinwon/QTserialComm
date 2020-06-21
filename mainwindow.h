#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QMessageBox>
#include <QDebug>
#include <QTimer>
#include <QErrorMessage>
#include <QPixmap>
#include <iostream>
#include "darknet.h"
#include "network.h"
#include "opencv4/opencv2/opencv.hpp"
#include "imgformat.h"
#include <vector>

using namespace cv;

// Serial communication device
#define DEVICE01    "/dev/ttyS0"
#define DEVICE02    "/dev/ttyS1"




#define DEVICE03    "/dev/ttyS2"
#define DEVICE04    "/dev/ttyUSB0"
#define DEVICE05    "/dev/ttyUSB1"

// parameter
//#define SAVE_VIDEO      // save output
#define DETECTED        // use undected image
#define DEBUG           // use qDebug etc...
//#define SETDIST         // for setting distance


#define REALWIDTH 605.  //mm
#define REALHEIGHT 340. //mm
#define ZEROX 192.      //pixel
#define ZEROY 98.       //pixel


// -----------------------------------------------------------------------------------------------------------------
// Define constants that were used when Darknet network was trained.
// This is pretty much hardcoded code zone, just to give an idea what is needed.
// -----------------------------------------------------------------------------------------------------------------

// Path to configuration file.
static char *cfg_file = const_cast<char *>("./yolov3-tiny.cfg");
// Path to weight file.
static char *weight_file = const_cast<char *>("./yolov3-tiny-obj_40000.weights");
// Path to a file describing classes names.
static char *names_file = const_cast<char *>("./cup.names");
// This is an image to test.
//static char *input = const_cast<char *>("./dog.jpg");
// Define thresholds for predicted class.
static float thresh = 0.5;
static float hier_thresh = 0.5;
// Uncomment this if you need sort predicted result.
//    float nms = 0.45;


struct DetectedData{
    float x, y;     // centerpoint
    float w, h;     // maybe not used
    float prob;     // object probability
};

void mouse_callback(int event, int x, int y, int flags, void *userdata);

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow;}
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

    void slotTimerAlarm();          //Run on a control cycle

    void on_isDetectBT_clicked();

    QByteArray getDetectedImg();        //return yolo result image

    void on_changeConstant_clicked();

    //0 : left first, 1 : right first
    int cup_sorting(DetectedData* left, DetectedData* right, float threshold = 1.);

private:

    // what process has to do
    enum MODE{
        READY = 0,
        START_DETECT,
        SEND_DATA,
        WATER_ERROR
    };

    uint8_t mode = READY;

    void showStatusMessage(const QString &message);

    Ui::MainWindow *ui;

    //serial data variable
    QSerialPort     *m_serial;
    Settings        p;
    bool            read_Flag = false;
    QByteArray      readAllData = NULL;
    QByteArray      realData = NULL;
    QTimer          *timer;

    int             coffeeNum = 1;

    QByteArray      circleData;

    // video data variable
    uint16_t        frame_width = 640;
    uint16_t        frame_height = 360;
    VideoCapture    cap;
    Mat             frame;
    QImage          detectedImg;
    size_t          classes;
    char            **labels;
    network         *net;
    float           maxTime = 0;
    bool            detectFlag = true;
    QMessageBox     errorMassage;
    float           CAMERA_CONSTANT = -0.18;
    int             validCupNum = 0;
    bool            sameCupFlag = false;

#ifdef SAVE_VIDEO
    VideoWriter writer;
#endif


};
#endif // MAINWINDOW_H
