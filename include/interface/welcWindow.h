#ifndef WELCWINDOW_H
#define WELCWINDOW_H

#include <QWidget>

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

#endif // WELCWINDOW_H
