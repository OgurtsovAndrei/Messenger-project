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

    void on_readyButton_clicked();

    bool sign_up();

    bool incorrect_log_or_pas(const QString &log, const QString &pas);

    bool incorrect_name_or_surname(const QString &name, const QString &sname);

private:
    Ui::Register *ui;
    bool regVersion = true;
};

#endif // REGISTER_H
