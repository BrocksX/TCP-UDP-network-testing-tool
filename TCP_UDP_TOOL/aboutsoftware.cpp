#include "aboutsoftware.h"
#include "ui_aboutsoftware.h"

AboutSoftware::AboutSoftware(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AboutSoftware)
{
    ui->setupUi(this);
    this->setWindowTitle("TCP/UDP Testing Tool");
    ui->author->setOpenExternalLinks(true);
    ui->author->setText("<a href = https://github.com/BrocksX>Author: https://github.com/BrocksX");
}

AboutSoftware::~AboutSoftware()
{
    delete ui;
}
