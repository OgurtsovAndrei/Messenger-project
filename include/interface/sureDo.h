#ifndef SUREDO_H
#define SUREDO_H

#include <QDialog>

namespace Ui {
class SureDo;
}

class SureDo : public QDialog
{
    Q_OBJECT

public:
    explicit SureDo(QString text, QWidget *parent = nullptr);
    ~SureDo();

    void set_text(QString text);

private:
    Ui::SureDo *ui;
};

#endif // SUREDO_H
