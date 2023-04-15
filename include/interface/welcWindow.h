#ifndef WELCWINDOW_H
#define WELCWINDOW_H

#include <QWidget>
#include "../../src/general/NetTest/netClient.hpp"

namespace Ui {
class WelcWindow;
}

class WelcWindow : public QWidget
{
    Q_OBJECT

public:
    explicit WelcWindow(QWidget *parent = nullptr);
    ~WelcWindow();

private slots:
    void on_regButton_clicked();

private:
    Ui::WelcWindow *ui;
};

extern Net::Client::Client client;

#endif // WELCWINDOW_H
