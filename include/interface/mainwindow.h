#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QMap>
#include "clientinfo.h"
#include "database/User.hpp"
#include "interface/bubble.h"
#include "interface/sureDo.h"
#include "Net/NetClient.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(Net::Client::Client *client_, QWidget *parent = nullptr);
    ~MainWindow();

    void set_client_info(const database_interface::User& cl);

    [[nodiscard]] unsigned int get_client_id() const;

    [[nodiscard]] int get_cl_encryption_id() const;

    [[nodiscard]] Net::Client::Client *get_client();

    void update_chats(int n = 100);

    void update_messages(bool update_by_timer = false);

    void change_message(Bubble *bub);

    void download_file(const std::string &file_name);

    [[nodiscard]] QString get_client_name_surname() const;

    [[nodiscard]] QString get_second_user_name_surname(int dialog_id);

private slots:

    void on_chatsList_itemClicked(QListWidgetItem *item = nullptr);

    void on_findButton_clicked();

    void on_groupButton_clicked();

    void on_chatName_clicked();

    void on_profileButton_clicked();

    void on_messagesList_itemDoubleClicked(QListWidgetItem *item);

    void on_sendButton_clicked();

    void on_fileButton_clicked();

private:
    Net::Client::Client *client;
    Ui::MainWindow *ui;
    ClientInfo cl_info;
    int select_chat_id = -1;
    int change_msg_id = -1;
    bool send_edit_mode = false;
    bool file_cancel_mode = false;
    QString uploaded_file_name;
    SureDo* sure_add_group;
    std::vector<database_interface::Message> msg_in_current_chat;


    void addMessage(
        const QString &msg,
        unsigned int msg_id,
        const ClientInfo &send_user_info,
        bool isFile = false
    );

    void sendFile();

    void add_group(const std::string &group_name);

    [[nodiscard]] bool messages_all_identical(const std::vector<database_interface::Message> &messages);
};

void show_popUp(const std::string &err_msg);

void show_success_popUp(const std::string &suc_msg);

#endif // MAINWINDOW_H
