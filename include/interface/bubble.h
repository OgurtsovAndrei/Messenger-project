#ifndef BUBBLE_H
#define BUBBLE_H

#include <QWidget>
#include <QString>

class Bubble : public QWidget {
public:
    Bubble(const QString &msg, const bool &incoming = false);
};

#endif // BUBBLE_H
