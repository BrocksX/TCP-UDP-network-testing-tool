#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "aboutsoftware.h"

const short CONNECT_WAIT_SEC = 5;
const short TCP_MODE = -2;
const short UDP_MODE = -3;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("TCP/UDP Testing Tool");
    this->setStyleSheet("QMainWindow {background-color:rgb(255, 255, 255)}");
    connect(ui->ListenBut, SIGNAL(clicked()), this, SLOT(on_server_listen_clicked()));
    connect(ui->connectBut, SIGNAL(clicked()), this, SLOT(on_client_connect_clicked()));
    ui->TcpBut->setChecked(true);
    ui->sendBut->setEnabled(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_server_listen_clicked()
{
    QString port_str = ui->portEdit->text();
    if(port_str.isEmpty())
    {
        QMessageBox::information(this, "提示", "请输入端口号");
        return;
    }
    bool ok = true;
    int port = port_str.toInt(&ok);
    if(!ok || port > 65535)
    {
        QMessageBox::information(this, "提示", "请输入正确的端口号");
        return;
    }
    if(ui->modeSelect->checkedId() == TCP_MODE)// TCP设置
    {
        server_ = std::unique_ptr<QTcpServer>(new QTcpServer());
        if(server_->listen(QHostAddress::Any, port))
        {
            log("TCP SERVER: listening");
            connect(server_.get(), &QTcpServer::newConnection, [&]()
            {
                QTcpSocket *socket = server_->nextPendingConnection();
                acceptSockets.emplace_back(socket);
                connect(socket, SIGNAL(readyRead()), this, SLOT(receiveMsg()));
                connect(socket, &QTcpSocket::disconnected, [&]()
                {
                    log("TCP: client disconnected");
                });
                log("TCP SERVER: new connection");
            });
            ui->ListenBut->setText("断开");
            ui->portEdit->setEnabled(false);
            ui->ipEdit->setEnabled(false);
            ui->tarPortEdit->setEnabled(false);
            ui->connectBut->setEnabled(false);
            ui->TcpBut->setEnabled(false);
            ui->UdpBut->setEnabled(false);
            ui->sendBut->setEnabled(true);
            disconnect(ui->ListenBut, SIGNAL(clicked()), this, SLOT(on_server_listen_clicked()));
            connect(ui->ListenBut, SIGNAL(clicked()), this, SLOT(on_server_disconnect_clicked()));
        }
        else
        {
            QMessageBox::information(this, "提示", "监听失败，请检查防火墙");
            return;
        }
    }
    else if (ui->modeSelect->checkedId() == UDP_MODE) // UDP设置
    {
        udpListenSocket_ = std::unique_ptr<QUdpSocket>(new QUdpSocket());
        if(udpListenSocket_->bind(port))
        {
            log("UDP SERVER: listening");
            connect(udpListenSocket_.get(),SIGNAL(readyRead()),this, SLOT(udpReadMsg()));
            ui->ListenBut->setText("断开");
            ui->portEdit->setEnabled(false);
            ui->TcpBut->setEnabled(false);
            ui->UdpBut->setEnabled(false);
            disconnect(ui->ListenBut, SIGNAL(clicked()), this, SLOT(on_server_listen_clicked()));
            connect(ui->ListenBut, SIGNAL(clicked()), this, SLOT(on_server_disconnect_clicked()));
        }
    }
}

void MainWindow::on_server_disconnect_clicked()
{
    disconnect(ui->ListenBut, SIGNAL(clicked()), this, SLOT(on_server_disconnect_clicked()));
    connect(ui->ListenBut, SIGNAL(clicked()), this, SLOT(on_server_listen_clicked()));
    ui->ListenBut->setText("监听");
    ui->portEdit->setEnabled(true);
    ui->ipEdit->setEnabled(true);
    ui->tarPortEdit->setEnabled(true);
    ui->connectBut->setEnabled(true);
    ui->TcpBut->setEnabled(true);
    ui->UdpBut->setEnabled(true);
    ui->sendBut->setEnabled(false);
    if(ui->modeSelect->checkedId() == TCP_MODE && server_)
    {
        log("TCP SERVER: stop listening");
        server_->close();
        for(const auto s : acceptSockets)
            s->close();
        acceptSockets.clear();
    }
    if(ui->modeSelect->checkedId() == UDP_MODE && udpListenSocket_)
    {
        log("UDP SERVER: stop listening");
        udpListenSocket_->close();
    }
}


void MainWindow::on_clearBut_clicked()
{
    ui->textEdit->clear();
}


void MainWindow::on_sendBut_clicked()
{
    if(ui->modeSelect->checkedId() == TCP_MODE)
    {
        bool hadWirte = false;
        if(clientSocket_ && clientSocket_->state() == QAbstractSocket::ConnectedState)
        {
            clientSocket_->write(ui->textEdit->toPlainText().toLocal8Bit());
            clientSocket_->waitForBytesWritten();
            hadWirte = true;
        }
        for(const auto & s : acceptSockets)
        {
            if(s->state() == QAbstractSocket::ConnectedState)
            {
                s->write(ui->textEdit->toPlainText().toLocal8Bit());
                s->waitForBytesWritten();
                hadWirte = true;
            }
        }
        if(hadWirte)
        {
            log("TCP send message");
            qDebug()<<"SEND: "<<ui->textEdit->toPlainText().toLocal8Bit();
        }
        else
        {
            QMessageBox::information(this, "提示", "没有可发送信息的目标");
        }
    }
    else if (ui->modeSelect->checkedId() == UDP_MODE && udpConnectSocket_)
    {
        QByteArray dataGram = ui->textEdit->toPlainText().toLocal8Bit();
        udpConnectSocket_->writeDatagram(dataGram.data(), dataGram.size(), QHostAddress(ui->ipEdit->text()), ui->tarPortEdit->text().toInt());
        log("UDP send message");
    }
    else
    {
        QMessageBox::information(this, "提示", "没有可发送信息的目标");
    }
}

void MainWindow::on_client_connect_clicked()
{
    QString port_str = ui->tarPortEdit->text();
    if(port_str.isEmpty())
    {
        QMessageBox::information(this, "提示", "请输入端口号");
        return;
    }
    bool ok = true;
    int port = port_str.toInt(&ok);
    if(!ok || port > 65535)
    {
        QMessageBox::information(this, "提示", "请输入正确的端口号");
        return;
    }
    QString ip_str = ui->ipEdit->text();
    if(ip_str.isEmpty())
    {
        QMessageBox::information(this, "提示", "请输入目标主机IP地址");
        return;
    }
    QHostAddress addr = QHostAddress(ip_str);
    if(ui->modeSelect->checkedId() == TCP_MODE)// TCP设置
    {
        this->clientSocket_ = std::unique_ptr<QTcpSocket>(new QTcpSocket());
        clientSocket_->connectToHost(addr,port);
        if(clientSocket_->waitForConnected(CONNECT_WAIT_SEC * 1000))
        {
            log("TCP CLIENT: connected");
            ui->portEdit->setEnabled(false);
            ui->ListenBut->setEnabled(false);
            ui->ipEdit->setEnabled(false);
            ui->tarPortEdit->setEnabled(false);
            ui->connectBut->setText("断开");
            ui->TcpBut->setEnabled(false);
            ui->UdpBut->setEnabled(false);
            ui->sendBut->setEnabled(true);
            connect(clientSocket_.get(), SIGNAL(readyRead()), this, SLOT(receiveMsg()));
            disconnect(ui->connectBut, SIGNAL(clicked()), this, SLOT(on_client_connect_clicked()));
            connect(ui->connectBut, SIGNAL(clicked()), this, SLOT(on_client_disconnect_clicked()));
            connect(clientSocket_.get(), &QTcpSocket::disconnected, [&]()
            {
                log("TCP: disconnected");
            });
        }
        else
        {
            QMessageBox::information(this, "提示", "连接失败，请检查IP和端口设置");
            return;
        }
    }
    else if (ui->modeSelect->checkedId() == UDP_MODE)
    {
        log("UDP client activated");
        ui->ipEdit->setEnabled(false);
        ui->tarPortEdit->setEnabled(false);
        ui->connectBut->setText("断开");
        ui->TcpBut->setEnabled(false);
        ui->UdpBut->setEnabled(false);
        ui->sendBut->setEnabled(true);
        udpConnectSocket_ = std::unique_ptr<QUdpSocket>(new QUdpSocket());
        disconnect(ui->connectBut, SIGNAL(clicked()), this, SLOT(on_client_connect_clicked()));
        connect(ui->connectBut, SIGNAL(clicked()), this, SLOT(on_client_disconnect_clicked()));
    }
}

void MainWindow::on_client_disconnect_clicked()
{
    disconnect(ui->connectBut, SIGNAL(clicked()), this, SLOT(on_client_disconnect_clicked()));
    connect(ui->connectBut, SIGNAL(clicked()), this, SLOT(on_client_connect_clicked()));
    ui->ListenBut->setEnabled(true);
    ui->portEdit->setEnabled(true);
    ui->ipEdit->setEnabled(true);
    ui->tarPortEdit->setEnabled(true);
    ui->connectBut->setText("连接");
    ui->TcpBut->setEnabled(true);
    ui->UdpBut->setEnabled(true);
    ui->sendBut->setEnabled(false);
    if(ui->modeSelect->checkedId() == TCP_MODE && clientSocket_)
    {
        log("TCP connection close");
        clientSocket_->close();
    }
    if(ui->modeSelect->checkedId() == UDP_MODE && udpConnectSocket_)
    {
        log("UDP connection close");
        udpConnectSocket_->close();
    }
}

void MainWindow::receiveMsg()
{
    log("TCP receive message");
    ui->msgDisplay->append(QString( qobject_cast<QTcpSocket*>(sender())->readAll() ));
}

void MainWindow::udpReadMsg()
{
    log("UDP receive message");
    while(udpListenSocket_->hasPendingDatagrams())
    {
        QByteArray dataGram;
        dataGram.resize(udpListenSocket_->pendingDatagramSize());
        udpListenSocket_->readDatagram(dataGram.data(),dataGram.size());
        ui->msgDisplay->insertPlainText(dataGram);
    }
}

void MainWindow::on_clearRecMsg_clicked()
{
    ui->msgDisplay->clear();
}

void MainWindow::on_action_triggered()
{
    QFileDialog fileDialog;
    QString fileName = fileDialog.getSaveFileName(this,tr("Open File"),"",tr("Text File(*.txt)"));
    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this,tr("错误"),tr("打开文件失败"));
        return;
    }
    else
    {
        QTextStream textStream(&file);
        QString str = ui->msgDisplay->toPlainText();
        textStream<<str;
        QMessageBox::warning(this,tr("提示"),tr("保存文件成功"));
        file.close();
    }
}

void MainWindow::log(QString text) const
{
    QString info = QTime::currentTime().toString("hh:mm:ss");
    info += " - ";
    info += text;
    ui->logBrowser->append(info);
}


void MainWindow::on_logClear_clicked()
{
    ui->logBrowser->clear();
}


void MainWindow::on_aboutSoftware_triggered()
{
    AboutSoftware *about = new AboutSoftware();
    about->setWindowModality(Qt::ApplicationModal);
    about->show();
}

