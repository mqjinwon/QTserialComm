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
}

MainWindow::~MainWindow()
{
    closeSerialPort();
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
