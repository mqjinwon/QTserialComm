#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_serial(new QSerialPort(this)){

    ui->setupUi(this);

    p.name          = DEVICE04;
    p.baudRate      = QSerialPort::Baud115200;
    p.dataBits      = QSerialPort::Data8;
    p.parity        = QSerialPort::NoParity;
    p.stopBits      = QSerialPort::OneStop;
    p.flowControl   = QSerialPort::NoFlowControl;

    openSerialPort();
    connect(m_serial, &QSerialPort::errorOccurred, this, &MainWindow::handleError);
    connect(m_serial, &QSerialPort::readyRead, this, &MainWindow::readData);

    // TIMER로 slotTimerAlarm 함수를 계속 호출 할 수 있도록 한다
    timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(slotTimerAlarm()));
    timer->start(100);       //주기: ms단위

    // 영상 처리 부분
    //동영상 파일로부터 부터 데이터 읽어오기 위해 준비
        cap.open(0);
        if (!cap.isOpened())        {
            printf("동영상 파일을 열수 없습니다. \n");
        }

        //동영상 플레이시 크기를  640x360으로 지정
        cap.set(CAP_PROP_FRAME_WIDTH,frame_width);
        cap.set(CAP_PROP_FRAME_HEIGHT,frame_height);

#ifdef SAVE_VIDEO
        writer.open("output.avi", VideoWriter::fourcc('M', 'J', 'P', 'G'), 10, Size(frame_width,frame_height), true);
#endif


        // Number of classes in "obj.names"
        // This is very rude method and in theory there must be much more elegant way.
        classes = 0;
        labels = get_labels(names_file);
        while (labels[classes] != nullptr) {
            classes++;
        }
#ifdef  DEBUG
        qDebug() << "Num of Classes " << classes << endl;
#endif

        // -----------------------------------------------------------------------------------------------------------------
        // Do actual logic of classes prediction.
        // -----------------------------------------------------------------------------------------------------------------

        // Load Darknet network itself.
        net = load_network(cfg_file, weight_file, 0);
        // In case of testing (predicting a class), set batch number to 1, exact the way it needs to be set in *.cfg file
        set_batch_network(net, 1);



        //Error message setting
        QString titleText = "<P><b><div align='center'><font color='#ff0000' font size=10>";
        titleText.append("WARNING!");
        titleText.append("</font></div></b></P></br>");

        QString mainText = "<P><b><div align='center'><font color='#000000' font size=5>";
        mainText.append("PLEASE ADD THE WATER");
        mainText.append("</font></div></b></P></br>");

        errorMassage.resize(320, 240);
        errorMassage.setStyleSheet("background-color: rgb(255, 255, 255)");
        errorMassage.setWindowTitle("WARNING");
        errorMassage.setStandardButtons(QMessageBox::Ok);
        errorMassage.setDefaultButton(QMessageBox::Ok);

        errorMassage.setText(titleText);
        errorMassage.setInformativeText(mainText);
        errorMassage.setIconPixmap(QPixmap("./waterERR.png"));

#ifdef SETDIST
        namedWindow("setDist image");
        setMouseCallback("setDist image", mouse_callback, 0);
#endif
}

MainWindow::~MainWindow(){
    closeSerialPort();
    free(labels);
    cap.release();

#ifdef SAVE_VIDEO
    writer.release();
#endif

    destroyAllWindows();
    delete ui;
}

void MainWindow::openSerialPort(){

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

void MainWindow::closeSerialPort(){
    if (m_serial->isOpen())
        m_serial->close();
    showStatusMessage(tr("Disconnected"));
}

void MainWindow::writeData(const QByteArray &data){
    m_serial->write(QByteArray("S"));
    m_serial->write(data);
    m_serial->write(QByteArray("E"));
}

void MainWindow::readData(){

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
                    //readAllData.remove(start, end-start+1);

                    // option) E까지의 모든 데이터 날려버리는 방법
                    readAllData.remove(0, end+1);
                    endFlag = true;

                    if(realData == "Y1" || realData == "Y2" || realData == "Y3"){
                        mode = START_DETECT;
                        coffeeNum = (int)(realData[1]-48);
                    }
                    if(realData == "X"){
                        mode = WATER_ERROR;

                    }
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

void MainWindow::handleError(QSerialPort::SerialPortError error){
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, tr("Critical Error"), m_serial->errorString());
        closeSerialPort();
    }
}

void MainWindow::showStatusMessage(const QString &message){
    ui->m_status->setText(message);
}

void MainWindow::on_BTwriteData_clicked(){
    QString data = ui->WritePlainText->toPlainText();
    writeData(data.toUtf8());
}

void MainWindow::on_BTeraseWrite_clicked(){
    ui->WritePlainText->clear();
}

void MainWindow::on_BTeraseRead_clicked(){
    ui->ReadDataText->clear();
}

void MainWindow::slotTimerAlarm(){
#ifdef  DEBUG
#endif

#ifdef DEBUG
    if(detectFlag){
        getDetectedImg();
        ui->jImg->setPixmap(QPixmap::fromImage(detectedImg));
        ui->jImg->show();

        writeData(circleData);
        circleData.clear();
    }
#endif

    switch(mode){

    case READY:
        //just wait until get serial comm "CM"
#ifndef DEBUG
        //웹캡으로부터 한 프레임을 읽어옴
        cap >> frame;
        detectedImg = QImage((uchar*) frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888).rgbSwapped();
        ui->jImg->setPixmap(QPixmap::fromImage(detectedImg));
        ui->jImg->show();
#endif
        break;

    case START_DETECT:
        //if get serial comm "Y-"
        if(!sameCupFlag){
            if(detectFlag){
                getDetectedImg();
                ui->jImg->setPixmap(QPixmap::fromImage(detectedImg));
                ui->jImg->show();
            }
        }else{
            mode  = SEND_DATA;
        }

        break;

    case SEND_DATA:
        //send circle data to cortex
        writeData(circleData);
        sameCupFlag = false;
        mode = READY;
        break;

    case WATER_ERROR:
        errorMassage.show();
        errorMassage.exec();
        mode = READY;
        break;
    default:
        break;
    }
}

void MainWindow::on_isDetectBT_clicked(){
    if(detectFlag)  detectFlag = false;
    else            detectFlag = true;

}

QByteArray MainWindow::getDetectedImg(){
    std::vector<DetectedData*>* datas = new std::vector<DetectedData*>;
    DetectedData* data;

    if (!cap.isOpened())        {
        printf("connect video cam!!! \n");
        cap.open(0);\
        //동영상 플레이시 크기를  320x240으로 지정
        cap.set(CAP_PROP_FRAME_WIDTH,frame_width);
        cap.set(CAP_PROP_FRAME_HEIGHT,frame_height);
    }
    else{

        //웹캡으로부터 한 프레임을 읽어옴
        cap >> frame;

        Mat dstImg;

        dstImg = frame.clone();

        //up, down flip - for y axis coordinate--------------------------------important!!
        flip(dstImg, dstImg, 0);

        // down constraint
        Scalar avg = mean(dstImg) * 0.2;
        dstImg = dstImg * 0.8 + avg[0];

#ifdef SAVE_VIDEO
        writer.write(dstImg);
#endif

#ifdef SETDIST
        imshow("setDist image", dstImg);
        waitKey(1);
#endif

// send image
#ifndef DETECTED
        detectedImg = QImage((uchar*) dstImg.data, dstImg.cols, dstImg.rows, dstImg.step, QImage::Format_RGB888).rgbSwapped();

// send detected image
#else

        Mat rectImg(dstImg);

        image sized = letterbox_image(mat_to_image(dstImg), net->w, net->h);

        // Get actual data associated with test image.
        float *frame_data = sized.data;

        // Do prediction.
        double time = what_time_is_it_now();
        network_predict(*net, frame_data);
#ifdef  DEBUG
        qDebug() << " predicted in " << (what_time_is_it_now() - time) << " sec.";

        if(maxTime < (what_time_is_it_now() - time)){
            maxTime = (what_time_is_it_now() - time);
            qDebug() << " maxTime: " << maxTime << " sec.-------------------------";
        }
#endif

        // Get number fo predicted classes (objects).
        int num_boxes = 0;
        detection *detections = get_network_boxes(net, dstImg.cols, dstImg.rows, thresh, hier_thresh, nullptr, 1, &num_boxes, 1);
#ifdef  DEBUG
        qDebug() << "Detected " << num_boxes << " obj, class " << detections->classes;
#endif
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

                    float detX = detections[i].bbox.x;
                    float detY = detections[i].bbox.y;
                    float detW = detections[i].bbox.w;
                    float detH = detections[i].bbox.h;

#ifdef  DEBUG
                    qDebug() << labels[j] << " " << (int16_t) (detections[i].prob[j] * 100) << "%\t"
                                << " x: " << detX*rectImg.cols << " y: " << detY*rectImg.rows
                                << " w: " << detW*rectImg.cols  << " h: " << detH*rectImg.rows << endl;
#endif

                    bool isSame(false);

                    float errorX = (detX - 0.5) * CAMERA_CONSTANT;
                    float errorY = (detY - 0.5) * CAMERA_CONSTANT;


                    //is detection object existed previous?
                    for(int _data = 0; _data < datas->size(); _data++){
                        float nearTh = pow((*datas)[_data]->x - detX, 2) + pow((*datas)[_data]->y - detY, 2)
                                    + pow((*datas)[_data]->w - detW, 2) + pow((*datas)[_data]->h - detH, 2);
#ifdef  DEBUG
                        qDebug() << "nearth" << nearTh;
#endif

                        // it is near to previous saved object
                        if(nearTh < 0.02){
                            isSame = true;

                            // if new obj'prob is bigger than previous obj'prob
                            if(detections[i].prob[j] > (*datas)[_data]->prob){
                                (*datas)[_data]->x = detX + errorX;
                                (*datas)[_data]->y = detY + errorY;
                                (*datas)[_data]->w = detW;
                                (*datas)[_data]->h = detH;
                                (*datas)[_data]->prob = detections[i].prob[j];
                            }
                            break;
                        }

                    }

                    // difference object
                    if(!isSame){
                        rectangle(rectImg, Point((detX - detW/2.)*rectImg.cols, (detY - detH/2.)*rectImg.rows),
                                           Point((detX + detW/2.)*rectImg.cols, (detY + detH/2.)*rectImg.rows),
                                           Scalar(255, 0, 0), 3 );

#ifdef  DEBUG
                        // draw center point
                        line(rectImg, Point(detX*rectImg.cols, detY*rectImg.rows),
                             Point(detX*rectImg.cols, detY*rectImg.rows), Scalar(0, 0, 255), 5);

                        // draw real center point
                        line(rectImg, Point( (detX + errorX)*rectImg.cols, (detY + errorY)*rectImg.rows),
                             Point((detX + errorX)*rectImg.cols, (detY + errorY)*rectImg.rows), Scalar(0, 255, 0), 5);
#endif

                        // push data
                        data        = new DetectedData;
                        data->x     = detX + errorX;
                        data->y     = detY + errorY;
                        data->w     = detW;
                        data->h     = detH;
                        data->prob  = detections[i].prob[j];

                        datas->push_back(data);
                    }

                }
            }
        }

        // -----------------------------------------------------------------------------------------------------------------
        // Free resources.
        // -----------------------------------------------------------------------------------------------------------------

        free_detections(detections, num_boxes);
        free_image(sized);

        detectedImg = QImage((uchar*) rectImg.data, rectImg.cols, rectImg.rows, rectImg.step, QImage::Format_RGB888).rgbSwapped();

        // make serial data & sorting x axis big to small


        if(datas->size() != 0){
            for(int i=0; i< datas->size()-1; i++){
                int tmp = i;

                for(int j=i+1; j<datas->size(); j++){

                    //if right first
                    if(cup_sorting((*datas)[i], (*datas)[j], 0.25) == 1)     tmp = j;
                }

                DetectedData tmpData = *(*datas)[i];

                //swapping
                (*datas)[i]->h = (*datas)[tmp]->h;
                (*datas)[i]->w = (*datas)[tmp]->w;
                (*datas)[i]->x = (*datas)[tmp]->x;
                (*datas)[i]->y = (*datas)[tmp]->y;
                (*datas)[i]->prob = (*datas)[tmp]->prob;

                (*datas)[tmp]->h = tmpData.h;
                (*datas)[tmp]->w = tmpData.w;
                (*datas)[tmp]->x = tmpData.x;
                (*datas)[tmp]->y = tmpData.y;
                (*datas)[tmp]->prob = tmpData.prob;

            }
        }

        for(int i=0; i < datas->size(); i++){
            float x = round( ((*datas)[i]->x - ZEROX / (float)frame.cols) * REALWIDTH);
            float y = round( ((*datas)[i]->y - ZEROY / (float)frame.rows) * REALHEIGHT);

#ifdef DEBUG
            qDebug("real axis // x : %f, y: %f", x, y);
#endif


            if(x >= 4 && y >= 4){
                QByteArray  axisData;

                circleData.push_back("X");

                circleData.push_back(axisData.setNum(x));

                circleData.append("Y");

                circleData.push_back(axisData.setNum(y));

                validCupNum++;
            }

        }



//        float worldbiggestX   = 65535;
//        //for iteration
//        for(int i=0; i < datas->size(); i++){

//            float biggestX   = 0;
//            float biggestIDX = 0;
//            float x = 0, y = 0;
//            for(int _data = 0; _data < datas->size(); _data++){

//                x = round((*datas)[_data]->x * REALWIDTH);
//                y = round((*datas)[_data]->y * REALHEIGHT);

//                if(x > biggestX && worldbiggestX > x){
//                    biggestX   = y;
//                    biggestIDX = _data;
//                }
//            }

//            QByteArray  axisData;

//            x = round( ((*datas)[biggestIDX]->x - ZEROX / (float)frame.cols) * REALWIDTH);
//            y = round( ((*datas)[biggestIDX]->y - ZEROY / (float)frame.rows) * REALHEIGHT);

//#ifdef DEBUG
//            qDebug("real axis // x : %f, y: %f", x, y);
//#endif

//            if(x >0 && y >0){
//                circleData.push_back("X");

//                circleData.push_back(axisData.setNum(x));

//                circleData.append("Y");

//                circleData.push_back(axisData.setNum(y));

//                validCupNum++;

//                worldbiggestX = y;
//            }


//        }

        QString cupInfostring;
        // coffee is same valid cup num
        if(coffeeNum == validCupNum){
            sameCupFlag = true;

            cupInfostring = "<P><b><div align='center'><font color='#000000' font size=10>";
            cupInfostring.append("good job!");
            cupInfostring.append("</font></div></b></P></br>");
        }
        else{
            cupInfostring = "<P><b><div align='center'><font color='#000000' font size=10>";
            cupInfostring.append("please add " + QString::number(coffeeNum - validCupNum) + " more cups");
            cupInfostring.append("</font></div></b></P></br>");


            circleData.clear();
        }

        ui->cupInfo->setText(cupInfostring);

        validCupNum = 0;

#ifdef DEBUG
        qDebug() << "data" << circleData;
#endif

        for(auto i : *datas){
            delete i;
        }
        delete datas;

#endif
    }

    return circleData;
}

void MainWindow::on_changeConstant_clicked(){
    CAMERA_CONSTANT = ui->cameraConstant->toPlainText().toFloat();
}

int MainWindow::cup_sorting(DetectedData* left, DetectedData* right, float threshold){
    if(abs(left->y - right->y) < threshold){

        if(left->x <= right->x)  return 0;
        else                    return 1;

    }else{

        if(left->y <= right->y)  return 0;
        else                    return 1;

    }
}

void mouse_callback(int event, int x, int y, int flags, void *userdata)
{
    if( event == EVENT_LBUTTONDBLCLK)
        qDebug() << "x: " << x << "y: " << y;
}
