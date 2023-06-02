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
    unsigned int owner_id;

public:
    explicit Bubble(const QString &msg, const QString &cl_name_sur, unsigned int owner_id, const bool &incoming);

    QString get_msg_text();

    unsigned int get_owner_id();
};

#endif // BUBBLE_H
