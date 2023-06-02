#include "interface/mainwindow.h"
#include "./ui_mainwindow.h"
#include "Net/NetClient.hpp"
#include "Status.hpp"
#include "interface/addGroup.h"
#include "interface/bubble.h"
#include "interface/welcWindow.h"
#include "interface/chatInfo.h"
#include "interface/mesSetting.h"
#include "interface/popUp.h"
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
    update_chats();

    auto *chat_timer = new QTimer(this);
    connect(chat_timer, &QTimer::timeout, this, [&]{
      auto [status, chats] = client.get_last_n_dialogs(100);
      if (chats.size() != ui->chatsList->count()) {
          std::cout << "Chats updates...\n";
          update_chats();
          std::cout << "Finish \n";
      }
    });

    chat_timer->start(10000);

    auto *message_timer = new QTimer(this);
    connect(message_timer, &QTimer::timeout, this, [&]{on_chatsList_itemClicked();});

    message_timer->start(100);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::on_sendButton_clicked()
{
    QString msg = ui->newMessageInput->toPlainText();
    if (msg.isEmpty()) {
        auto *popUp = new PopUp("Message must not be empty", this);
        popUp->show();
        return;
    }
    if (ui->sendButton->text() == "Edit") {
        auto status = client.change_sent_message(change_msg_id, msg.toStdString());
        std::cout << "status" << status.message() << "\n";
        ui->sendButton->setText("Send");
        ui->newMessageInput->setPlainText("");
        return;
    }
    Status send_status = client.send_message_to_another_user(select_chat_id, 100000, msg.toStdString());
    QString name_sur = QString::fromStdString(get_client_name_surname());
    json json_data = json::parse(send_status.message());
    addMessage(msg, json_data["m_message_id"], name_sur, json_data["m_user_id"]);
    ui->newMessageInput->setPlainText("");
}

void MainWindow::update_chats(int n) {
    ui->chatsList->clear();
    auto [status, chats] = client.get_last_n_dialogs(n);
    for (const auto &chat : chats) {
        auto chat_item = new QListWidgetItem(nullptr, chat.m_dialog_id);
        if (chat.m_is_group) {
            chat_item->setText(QString::fromStdString(chat.m_name));
        }
        else {
//            auto frt_user = (*chat.m_users)[0];
//            auto sec_user = (*chat.m_users)[1];
//            if (frt_user.m_user_id == get_client_id()) {
//                std::swap(frt_user, sec_user);
//            }
//            new_chat = QString::fromStdString(frt_user.m_name + " " + frt_user.m_surname);
            chat_item->setText(QString::fromStdString(chat.m_name ));

        }
        ui->chatsList->addItem(chat_item);
    }
}

void MainWindow::on_chatsList_itemClicked(QListWidgetItem *item)
{
    if (item != nullptr) {
        select_chat_id = item->type();
        ui->chatName->setText(item->text());
    }
    auto [status, messages] = client.get_n_messages(200, select_chat_id);
    if (ui->messagesList->count() == messages.size()) {
        return ;
    }
    ui->messagesList->clear();
    std::reverse(messages.begin(), messages.end());
    for (const auto &mess : messages) {
        bool incoming = false;
        auto [st, us] = client.get_user_by_id(mess.m_user_id);
        if (!st) {
            auto *popUp = new PopUp("Problems with displaying some messages.", this);
            popUp->show();
            continue;
        }
        QString name_sur = QString::fromStdString(us.m_name + " " + us.m_surname);
        if (mess.m_user_id != get_client_id()) {
            incoming = true;
        }
        addMessage(QString::fromStdString(mess.m_text), mess.m_message_id, name_sur, mess.m_user_id, incoming);
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
        std::cout << status.message() << "\n";
        auto *popUp = new PopUp("Sorry, user with login '" + find_chat + "' doesn't exists", this);
//        popUp->show();
        return;
    }
    unsigned int sec_id = sec_client.m_user_id;
    if (client.make_dialog(sec_client.m_name, "RSA", 1000, false, {get_client_id(), sec_id})) {
        update_chats();
    }
}

void MainWindow::addMessage(const QString &msg, const int mess_id, const QString &name_sur, unsigned int ow_id, const bool &incoming)
{
    auto *item = new QListWidgetItem(nullptr, mess_id);
    auto *bub = new Bubble(msg, name_sur, ow_id, incoming);
    ui->messagesList->addItem(item);
    ui->messagesList->setItemWidget(item, bub);
    item->setSizeHint(bub->sizeHint());
    ui->messagesList->scrollToBottom();
}

void MainWindow::change_message(QListWidgetItem *mes) {
    auto bub = dynamic_cast<Bubble*>(ui->messagesList->itemWidget(mes));
    if (bub->get_owner_id() != get_client_id()) {
        auto *popUp = new PopUp("You cannot edit another user's messages", this);
        popUp->show();
        return ;
    }
    ui->sendButton->setText("Edit");
    ui->newMessageInput->setText(bub->get_msg_text());
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

void MainWindow::set_change_msg_is(int msg_id) {
    change_msg_id = msg_id;
}

void MainWindow::on_profileButton_clicked()
{
    auto *ch_info = new ChatInfo(this);
    ch_info->show();
}

void MainWindow::on_messagesList_itemDoubleClicked(QListWidgetItem *item)
{
    auto *mess = new MesSetting(item, this);
    mess->show();
}

std::string MainWindow::get_client_name_surname() const {
    return cl_info.cl_name + " " + cl_info.cl_surname;
}

