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
    explicit MesSetting(QListWidgetItem *msg, MainWindow *mainWin, bool isFile, QWidget *parent = nullptr);
    ~MesSetting();

private slots:
    void on_delButton_clicked();

    void on_editButton_clicked();

    void on_downloadButton_clicked();

private:
    Ui::MesSetting *ui;
    QListWidgetItem *msg;
    MainWindow *mainWin;
};

#endif // MESSETTING_H
