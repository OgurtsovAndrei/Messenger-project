#ifndef BUBBLE_H
#define BUBBLE_H

#include <QVBoxLayout>
#include <QString>
#include <QWidget>
#include <QLabel>

class Bubble : public QWidget {
private:
    QVBoxLayout *bubLayout;
    QLabel *lbl;
    QLabel *lbl_name;

public:
    explicit Bubble(const QString &msg, const QString &cl_name_sur, const bool &incoming = false);

};

#endif // BUBBLE_H
