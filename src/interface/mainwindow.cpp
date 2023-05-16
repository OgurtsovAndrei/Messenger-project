#include "interface/mainwindow.h"
#include "./ui_mainwindow.h"
#include "Net/NetClient.hpp"
#include "Status.hpp"
#include "interface/addGroup.h"
#include "interface/bubble.h"
#include "interface/welcWindow.h"
#include "interface/chatInfo.h"
#include "interface/mesSetting.h"
//#include "User.hpp"

#include <QStringListModel>
#include <QListWidget>
#include <QLineEdit>
#include <QTimer>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    this->setWindowTitle("ИШО");
    /// TODO set user info
    ///
    update_chats();

    connect(ui->sendButton, &QPushButton::clicked, this, [&]{
        QString msg = ui->newMessageInput->toPlainText();
        if (msg.isEmpty()) {
            return;
        }
        Status send_status = client.send_message_to_another_user(select_chat_id, 100000, msg.toStdString());
        json json_data = json::parse(send_status.message());
        addMessage(msg, json_data["m_message_id"]);
        ui->newMessageInput->setPlainText("");
    });

    /* Проинициализоровать
    * ChatList
    * Messagewindow без send блока
    * Поставить таймер с update
    *
    */

    auto *chat_timer = new QTimer(this);
    connect(chat_timer, &QTimer::timeout, [&]{
      auto [status, chats] = client.get_last_n_dialogs(100);
      if (chats.size() != chats_id_map.size()) {
          std::cout << "Chats updates...\n";
          update_chats();
      }
    });

    chat_timer->start(10000);

    auto *message_timer = new QTimer(this);
    connect(message_timer, &QTimer::timeout, this, [&]{on_chatsList_itemClicked();});

    message_timer->start(10000);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::update_chats() {
    ui->chatsList->clear();
    auto [status, chats] = client.get_last_n_dialogs(100);
    std::cout << chats.size() << "\n";
    for (const auto &chat : chats) {
        QString new_chat;
        if (chat.m_is_group) {
            new_chat = QString::fromStdString(chat.m_name);
        }
        else {
//            auto frt_user = (*chat.m_users)[0];
//            auto sec_user = (*chat.m_users)[1];
//            if (frt_user.m_user_id == get_client_id()) {
//                std::swap(frt_user, sec_user);
//            }
//            new_chat = QString::fromStdString(frt_user.m_name + " " + frt_user.m_surname);
            new_chat = QString::fromStdString(chat.m_name );

        }
        ui->chatsList->addItem(new_chat);
        chats_id_map[new_chat] = chat.m_dialog_id;
    }
}

void MainWindow::on_chatsList_itemClicked(QListWidgetItem *item)
{
    if (item != nullptr) {
        select_chat_id = chats_id_map[item->text()];
        ui->chatName->setText(item->text());
    }
    auto [status, messages] = client.get_n_messages(100, select_chat_id);
    if (ui->messagesList->count() == messages.size()) {
        return ;
    }
    ui->messagesList->clear();
    for (const auto &mess : messages) {
        bool incoming = false;
        if (mess.m_user_id != get_client_id()) {
            incoming = true;
        }
        std::cout << mess.m_text << "\n";
        addMessage(QString::fromStdString(mess.m_text), mess.m_message_id, incoming);
    }
}

void MainWindow::on_findButton_clicked()
{
    std::string find_chat = ui->findLine->text().toStdString();
    if (find_chat.empty()) {
        return;
    }
    auto [status, sec_client] = client.get_user_id_by_login(find_chat);
    if (!status) {
        return;
    }
    unsigned int sec_id = sec_client.m_user_id;
    if (client.make_dialog(sec_client.m_name, "RSA", 1000, false, {get_client_id(), sec_id})) {
        std::cout << client.get_last_n_dialogs(100, select_chat_id).second.size() << "\n";
        update_chats();
    }
    ///TODO search user and start dialog
}

void MainWindow::addMessage(const QString &msg, const int mess_id, const bool &incoming)
{
    auto *item = new QListWidgetItem(nullptr, mess_id);
    auto *bub = new Bubble(msg, incoming);
    ui->messagesList->addItem(item);
    ui->messagesList->setItemWidget(item, bub);
    item->setSizeHint(bub->sizeHint());
    ui->messagesList->scrollToBottom();
}

void MainWindow::on_groupButton_clicked()
{
   auto *add_gr = new AddGroup(this);
   add_gr->show();
}

void MainWindow::on_chatName_clicked()
{
    auto *ch_info = new ChatInfo(select_chat_id, this);
    ch_info->show();
}

unsigned int MainWindow::get_client_id() const {
    return cl_info.cl_id;
}

void MainWindow::set_client_info(const database_interface::User& cl) {
    cl_info = ClientInfo(cl.m_name, cl.m_surname, cl.m_login, cl.m_user_id);
}

void MainWindow::on_profileButton_clicked()
{
    auto *ch_info = new ChatInfo(select_chat_id, this);
    ch_info->show();
}


void MainWindow::on_messagesList_itemDoubleClicked(QListWidgetItem *item)
{
    auto *mess = new MesSetting(item, this);
    mess->show();
}

void MainWindow::del_message(QListWidgetItem *item) {
    ui->messagesList->removeItemWidget(item);
}
