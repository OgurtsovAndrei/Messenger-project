#ifndef CHATSETTING_HPP
#define CHATSETTING_HPP

#include <interface/sureDo.h>
#include <QDialog>
#include <QWidget>
#include "Net/NetClient.hpp"
#include "interface/chatInfo.h"
#include "mesSetting.h"

namespace Ui {
class ChatSetting;
}

class ChatSetting : public QWidget {
    Q_OBJECT

public:
    explicit ChatSetting(
        Net::Client::Client *client_,
        unsigned int sec_cl_id_,
        QString sec_cl_name_,
        std::string dialog_name_,
        ChatInfo *chatInfo_,
        QWidget *parent = nullptr
    );
    explicit ChatSetting(
        Net::Client::Client *client_,
        unsigned int client_id_,
        unsigned int sec_cl_id_,
        std::string dialog_name_,
        QWidget *parent = nullptr
    );
    ~ChatSetting() override;

private slots:
    void on_startChatButton_clicked();

    void on_delMemButton_clicked();

private:
    Ui::ChatSetting *ui;
    SureDo *sure_del;
    unsigned int dialog_id = 0;
    unsigned int client_id = 0;
    unsigned int sec_cl_id = 0;
    QString sec_cl_name;
    std::string dialog_name;
    ChatInfo *chat_info;
    Net::Client::Client *client;
};

#endif  // CHATSETTING_HPP
