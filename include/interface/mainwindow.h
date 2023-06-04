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

    void set_client_info(const database_interface::User& cl);

    [[nodiscard]] unsigned int get_client_id() const;

    void update_chats(int n = 100);

    void change_message(QListWidgetItem *msg);

    void set_change_msg_is(int msg_id);

    [[nodiscard]] std::string get_client_name_surname() const;

private slots:

    void on_chatsList_itemClicked(QListWidgetItem *item = nullptr);

    void on_findButton_clicked();

    void on_groupButton_clicked();

    void on_chatName_clicked();

    void on_profileButton_clicked();

    void on_messagesList_itemDoubleClicked(QListWidgetItem *msg);

    void on_sendButton_clicked();

    void on_fileButton_clicked();

private:
    Ui::MainWindow *ui;
    ClientInfo cl_info;
    int select_chat_id = -1;
    int num_submited_mes = 0;
    int change_msg_id = -1;

    void addMessage(
        const QString &msg,
        unsigned int msg_id,
        const ClientInfo &sec_user_info,
        bool isFile = false
    );
};

void show_popUp(const std::string &err_msg);

QString extract_file_name(const QString &file_path);

#endif // MAINWINDOW_H
