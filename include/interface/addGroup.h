#ifndef ADDGOUP_H
#define ADDGOUP_H

#include <QDialog>
#include "interface/mainwindow.h"

namespace Ui {
class AddGroup;
}

class AddGroup : public QDialog
{
    Q_OBJECT

public:
    explicit AddGroup(MainWindow *mainWin = nullptr, QWidget *parent = nullptr);
    ~AddGroup();

private slots:
    void on_addGroupBox_accepted();

private:
    Ui::AddGroup *ui;
    MainWindow *mainWin;
};

#endif // ADDGOUP_H
