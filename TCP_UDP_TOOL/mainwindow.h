#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory.h>
#include <QMainWindow>
#include <QMessageBox>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QFileDialog>
#include <QTime>
#include <vector>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void log(QString text) const;

private slots:
    void on_server_listen_clicked();
    void on_server_disconnect_clicked();
    void on_clearBut_clicked();
    void on_sendBut_clicked();
    void on_client_connect_clicked();
    void on_client_disconnect_clicked();
    void on_clearRecMsg_clicked();
    void on_action_triggered();

    void udpReadMsg();
    void receiveMsg();

    void on_logClear_clicked();

    void on_aboutSoftware_triggered();

private:
    Ui::MainWindow *ui;
    std::unique_ptr<QTcpServer> server_;
    std::vector<QTcpSocket*> acceptSockets;
    std::unique_ptr<QTcpSocket> clientSocket_;
    std::unique_ptr<QUdpSocket> udpListenSocket_;
    std::unique_ptr<QUdpSocket> udpConnectSocket_;
};
#endif // MAINWINDOW_H
