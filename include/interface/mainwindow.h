#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QMap>
#include "clientinfo.h"
#include "database/User.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QString get_cur_chat_name() const;

    void set_client_info(const database_interface::User& cl);

    unsigned int get_client_id() const;

    std::string get_client_name_surname() const;

    void update_chats(int n = 100);

    void change_message(QListWidgetItem *mes);

    void set_change_msg_is(int msg_id);

private slots:

    void on_chatsList_itemClicked(QListWidgetItem *item = nullptr);

    void on_findButton_clicked();

    void addMessage(const QString &msg, const int mess_id, const QString &name_sur, unsigned int ow_id, const bool &incoming = false);

    void on_groupButton_clicked();

    void on_chatName_clicked();

    void on_profileButton_clicked();

    void on_messagesList_itemDoubleClicked(QListWidgetItem *item);

    void on_sendButton_clicked();

private:
    Ui::MainWindow *ui;
    ClientInfo cl_info;
    int select_chat_id = -1;
    int num_submited_mes = 0;
    int change_msg_id = -1;

};

#endif // MAINWINDOW_H
