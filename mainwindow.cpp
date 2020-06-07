#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_serial(new QSerialPort(this))
{

    p.name          = "COM4";
    p.baudRate      = QSerialPort::Baud115200;
    p.dataBits      = QSerialPort::Data8;
    p.parity        = QSerialPort::NoParity;
    p.stopBits      = QSerialPort::OneStop;
    p.flowControl   = QSerialPort::NoFlowControl;

    ui->setupUi(this);

    openSerialPort();
    connect(m_serial, &QSerialPort::errorOccurred, this, &MainWindow::handleError);
    connect(m_serial, &QSerialPort::readyRead, this, &MainWindow::readData);

    // TIMER로 slotTimerAlarm 함수를 계속 호출 할 수 있도록 한다
    timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(slotTimerAlarm()));
    timer->start(25);       //주기: ms단위

    // 영상 처리 부분
    //동영상 파일로부터 부터 데이터 읽어오기 위해 준비
        cap.open("/home/jin/QTproject/QTserialComm/videoplayback.mp4");
        if (!cap.isOpened())        {
            printf("동영상 파일을 열수 없습니다. \n");
        }

        //동영상 플레이시 크기를  320x240으로 지정
        cap.set(CAP_PROP_FRAME_WIDTH,1280);
        cap.set(CAP_PROP_FRAME_HEIGHT,720);

        namedWindow("video", 1);

        // Number of classes in "obj.names"
        // This is very rude method and in theory there must be much more elegant way.
        classes = 0;
        labels = get_labels(names_file);
        while (labels[classes] != nullptr) {
            classes++;
        }
        qDebug() << "Num of Classes " << classes << endl;

        // -----------------------------------------------------------------------------------------------------------------
        // Do actual logic of classes prediction.
        // -----------------------------------------------------------------------------------------------------------------

        // Load Darknet network itself.
        net = load_network(cfg_file, weight_file, 0);
        // In case of testing (predicting a class), set batch number to 1, exact the way it needs to be set in *.cfg file
        set_batch_network(net, 1);
}

MainWindow::~MainWindow()
{
    closeSerialPort();
    free(labels);
    delete ui;
}

void MainWindow::openSerialPort()
{

    m_serial->setPortName(p.name);
    m_serial->setBaudRate(p.baudRate);
    m_serial->setDataBits(p.dataBits);
    m_serial->setParity(p.parity);
    m_serial->setStopBits(p.stopBits);
    m_serial->setFlowControl(p.flowControl);
    if (m_serial->open(QIODevice::ReadWrite)) {
        showStatusMessage(tr("Connected to %1 : %2, databits: %3, paritybits: %4, stopbits: %5, flowcontrol: %6")
                          .arg(p.name).arg(p.baudRate).arg(p.dataBits)
                          .arg(p.parity).arg(p.stopBits).arg(p.flowControl));
    } else {
        QMessageBox::critical(this, tr("error"), "connection error");
        showStatusMessage(tr("Open error"));
    }
}

void MainWindow::closeSerialPort()
{
    if (m_serial->isOpen())
        m_serial->close();
    showStatusMessage(tr("Disconnected"));
}

void MainWindow::writeData(const QByteArray &data)
{
    m_serial->write(QByteArray("S"));
    m_serial->write(data);
    m_serial->write(QByteArray("E"));
}

void MainWindow::readData()
{
//    m_serial->getChar(&oneByteData);
//    qDebug() << oneByteData;
//    qDebug() << "connected";

//    if(oneByteData == 'S'){
//      read_Flag = true;
//    }
//    else if( (oneByteData == 'E') && (read_Flag = true)){
//      read_Flag = false;
//      ui->ReadDataText->setText(data);
//      oneByteData = NULL;
//      data.clear();
//    }
//    else if(read_Flag == true){
//      //여기서 데이터를 받는다.
//      data.append(oneByteData);
//    }

//    data.append(m_serial->readAll());
//    ui->ReadDataText->setText(data);

    readAllData.append(m_serial->readAll());

    int start = -1, end = 0;
    bool endFlag(false);
    for(QByteArray::iterator i=readAllData.begin(); i!=readAllData.end(); ++i){
        start++;

        // 데이터 전송 시작
        if(i[0] == 'S'){
            end = start;
            for(QByteArray::iterator j=i+1; j!=readAllData.end(); ++j){
                end++;

                // 데이터 전송 완료
                if(j[0] == 'E'){
                    // 받은 데이터만큼 지워버린다.
                    readAllData.remove(start, end-start+1);

                    // option) E까지의 모든 데이터 날려버리는 방법
                    // readAllData.remove(0, end+1);
                    endFlag = true;
                    break;
                }

                // 데이터 추가 부분
                realData.append(j[0]);
            }
            if(endFlag) break;
        }
    }


    ui->ReadDataText->setText(realData);
    realData.clear();
}

void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, tr("Critical Error"), m_serial->errorString());
        closeSerialPort();
    }
}

void MainWindow::showStatusMessage(const QString &message)
{
    ui->m_status->setText(message);
}

void MainWindow::on_BTwriteData_clicked()
{
    QString data = ui->WritePlainText->toPlainText();
    writeData(data.toUtf8());
}

void MainWindow::on_BTeraseWrite_clicked()
{
    ui->WritePlainText->clear();
}

void MainWindow::on_BTeraseRead_clicked()
{
    ui->ReadDataText->clear();
}

void MainWindow::slotTimerAlarm()
{
    //웹캡으로부터 한 프레임을 읽어옴
    cap >> frame;

    Mat rectImg(frame);

    image sized = letterbox_image(mat_to_image(frame), net->w, net->h);

    // Uncomment this if you need sort predicted result.
    //    layer l = net->layers[net->n - 1];

    // Get actual data associated with test image.
    float *frame_data = sized.data;

    // Do prediction.
    double time = what_time_is_it_now();
    network_predict(*net, frame_data);
    qDebug() << "'" << input << "' predicted in " << (what_time_is_it_now() - time) << " sec.";

    // Get number fo predicted classes (objects).
    int num_boxes = 0;
    detection *detections = get_network_boxes(net, frame.cols, frame.rows, thresh, hier_thresh, nullptr, 1, &num_boxes, 0);
    qDebug() << "Detected " << num_boxes << " obj, class " << detections->classes;


    // Uncomment this if you need sort predicted result.
    //    do_nms_sort(detections, num_boxes, l.classes, nms);

    // -----------------------------------------------------------------------------------------------------------------
    // Print results.
    // -----------------------------------------------------------------------------------------------------------------

    // Iterate over predicted classes and print information.
    for (int8_t i = 0; i < num_boxes; ++i) {
        for (uint8_t j = 0; j < classes; ++j) {
            if (detections[i].prob[j] > thresh) {
                // More information is in each detections[i] item.
                qDebug() << labels[j] << " " << (int16_t) (detections[i].prob[j] * 100) << "\t\t";
                qDebug() << " x: " << detections[i].bbox.x << " y: " << detections[i].bbox.y
                     << " w: " << detections[i].bbox.w  << " h: " << detections[i].bbox.h <<endl;

                rectangle(rectImg, Point((detections[i].bbox.x - detections[i].bbox.w/2.)*rectImg.cols,
                                         (detections[i].bbox.y - detections[i].bbox.h)*rectImg.rows),
                                   Point((detections[i].bbox.x + detections[i].bbox.w/2.)*rectImg.cols,
                                    (detections[i].bbox.y + detections[i].bbox.h)*rectImg.rows), Scalar(155, 155, 155) );
            }
        }
    }

    // -----------------------------------------------------------------------------------------------------------------
    // Free resources.
    // -----------------------------------------------------------------------------------------------------------------

    free_detections(detections, num_boxes);
    free_image(sized);

    QImage imgIn= QImage((uchar*) rectImg.data, rectImg.cols, rectImg.rows, rectImg.step, QImage::Format_RGB888).rgbSwapped();
    //QImage imgIn((uchar*)rectImg.data, rectImg.cols, rectImg.rows, rectImg.step1(), QImage::Format_RGB32);

    ui->jImg->setPixmap(QPixmap::fromImage(imgIn));
    ui->jImg->show();
}
