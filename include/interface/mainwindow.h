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

    void set_client_id(const int &id);

/*
 * fill chats list
 * add chat
 * open chat (fill it)
 * scroll chat
 * send message
*/

private slots:
    void update_chats();

    void on_chatsList_itemClicked(QListWidgetItem *item = nullptr);

    void on_findButton_clicked();

    void addMessage(const QString &msg, const bool &incoming = false);

private:
    Ui::MainWindow *ui;
    QMap<QString, int> chats_id_map;
    unsigned int client_id;
    int select_chat_id;

};

#endif // MAINWINDOW_H
