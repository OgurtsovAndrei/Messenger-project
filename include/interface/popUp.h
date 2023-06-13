#ifndef POPUP_H
#define POPUP_H

#include <QWidget>

namespace Ui {
class PopUp;
}

class PopUp : public QWidget
{
    Q_OBJECT

public:
    explicit PopUp(const std::string& error_msg, QWidget *parent = nullptr);
    ~PopUp();

    void change_error_on_success();

private:
    Ui::PopUp *ui;
};

#endif // POPUP_H
