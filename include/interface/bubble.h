#ifndef BUBBLE_H
#define BUBBLE_H

#include <QWidget>

namespace Ui {
class Bubble;
}

class Bubble : public QWidget
{
    Q_OBJECT

public:
    explicit Bubble(const QString &msg = "", QWidget *parent = nullptr);
    ~Bubble();

private:
    Ui::Bubble *ui;
};

#endif // BUBBLE_H
