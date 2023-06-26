#ifndef MESSETTING_HPP
#define MESSETTING_HPP

#include <QWidget>
#include "QListWidgetItem"
#include "interface/bubble.h"
#include "mainwindow.h"

namespace Ui {
class MesSetting;
}

class MesSetting : public QWidget {
    Q_OBJECT

public:
    explicit MesSetting(
        Bubble *msg,
        MainWindow *mainWin,
        bool isFile,
        QWidget *parent = nullptr
    );
    ~MesSetting();

private slots:
    void on_delButton_clicked();

    void on_editButton_clicked();

    void on_downloadButton_clicked();

private:
    Ui::MesSetting *ui;
    Bubble *msg;
    bool isFile;
    MainWindow *mainWin;
};

#endif  // MESSETTING_HPP
