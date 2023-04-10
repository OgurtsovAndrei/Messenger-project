#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "userUI.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void fill_chats();

/*
 * fill chats list
 * add chat
 * open chat (fill it)
 * scroll chat
 * send message
*/

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
