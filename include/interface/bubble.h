#ifndef BUBBLE_H
#define BUBBLE_H

#include <QHBoxLayout>
#include <QString>
#include <QWidget>
#include <QLabel>

class Bubble : public QWidget {
private:
    QHBoxLayout *bubLayout;
    QLabel *lbl;

public:
    explicit Bubble(const QString &msg, const bool &incoming = false);

};

#endif // BUBBLE_H
