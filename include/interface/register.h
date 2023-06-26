#ifndef REGISTER_HPP
#define REGISTER_HPP

#include <QWidget>
#include "interface/mainwindow.h"

namespace Ui {
class Register;
}

class Register : public QWidget {
    Q_OBJECT

public:
    explicit Register(Net::Client::Client *client_, QWidget *parent = nullptr);
    ~Register();

    void delRegInfo();

private slots:
    void on_cancelButton_clicked();

    void on_readyButton_clicked();

    bool sign_up();

private:
    Ui::Register *ui;
    bool regVersion = true;
    Net::Client::Client *client;
};

bool incorrect_log_or_pas(const QString &log, const QString &pas);

bool incorrect_name_or_surname(const QString &name);

#endif  // REGISTER_HPP
