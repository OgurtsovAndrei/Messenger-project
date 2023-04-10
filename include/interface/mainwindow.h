#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QMap>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

/*
 * fill chats list
 * add chat
 * open chat (fill it)
 * scroll chat
 * send message
*/

private slots:
    void fill_chats();

    void on_chatsList_itemClicked(QListWidgetItem *item = nullptr);

private:
    Ui::MainWindow *ui;
    QMap<QListWidgetItem, int> chats_id_map;
    int select_chat_id;

};

#endif // MAINWINDOW_H
