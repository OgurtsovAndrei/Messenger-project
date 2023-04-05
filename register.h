#ifndef REGISTER_H
#define REGISTER_H

#include <QWidget>

namespace Ui {
class Register;
}

class Register : public QWidget
{
    Q_OBJECT

public:
    explicit Register(QWidget *parent = nullptr);
    ~Register();

    void delRegInfo();

private slots:
    void on_cancelButton_clicked();

private:
    Ui::Register *ui;
};

#endif // REGISTER_H
