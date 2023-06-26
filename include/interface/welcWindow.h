#ifndef WELCWINDOW_HPP
#define WELCWINDOW_HPP

#include <QWidget>
#include "Net/NetClient.hpp"

namespace Ui {
class WelcWindow;
}

class WelcWindow : public QWidget {
    Q_OBJECT

public:
    explicit WelcWindow(
        Net::Client::Client *client_,
        QWidget *parent = nullptr
    );
    ~WelcWindow();

private slots:
    void on_regButton_clicked();

private:
    Ui::WelcWindow *ui;
    Net::Client::Client *client;
};

#endif  // WELCWINDOW_HPP
