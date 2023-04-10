#ifndef REGISTER_H
#define REGISTER_H

#include <QWidget>
#include "../../src/general/NetTest/netClient.hpp"

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

    void on_readyButton_clicked();

private:
    Ui::Register *ui;
    bool regVersion = true;
};

extern Net::Client::Client client;

#endif // REGISTER_H
