#ifndef ABOUTSOFTWARE_H
#define ABOUTSOFTWARE_H

#include <QWidget>

namespace Ui {
class AboutSoftware;
}

class AboutSoftware : public QWidget
{
    Q_OBJECT

public:
    explicit AboutSoftware(QWidget *parent = nullptr);
    ~AboutSoftware();

private:
    Ui::AboutSoftware *ui;
};

#endif // ABOUTSOFTWARE_H
