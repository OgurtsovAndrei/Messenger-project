#ifndef MESSETTING_H
#define MESSETTING_H

#include <QWidget>
#include "QListWidgetItem"
#include "mainwindow.h"

namespace Ui {
class MesSetting;
}

class MesSetting : public QWidget
{
    Q_OBJECT

public:
    explicit MesSetting(QListWidgetItem *mes, MainWindow *mainWin, QWidget *parent = nullptr);
    ~MesSetting();

private slots:
    void on_delButton_clicked();

private:
    Ui::MesSetting *ui;
    QListWidgetItem *mes;
    MainWindow *mainWin;
};

#endif // MESSETTING_H
