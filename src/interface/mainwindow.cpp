#include "../../include/interface/mainwindow.h"
#include "./ui_mainwindow.h"
#include "../../src/general/NetTest/netClient.hpp"
#include "../../include/interface/register.h"
#include "../../include/Status.hpp"
#include "../../include/interface/bubble.h"

#include <QStringListModel>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    this->setWindowTitle("ИШО");
    /// TODO set user info
    ///

    fill_chats();

    //  ui->chatsList->setModel(new QStringListModel(List));

    //    ui->chatsList->setEditTriggers(QAbstractItemView::AnyKeyPressed |
    //    QAbstractItemView::DoubleClicked); ui->sendButton->setVisible(false);
    //    ui->newMessageInput->setVisible(false); //or true - later in the code
    /* Проинициализоровать
    * ChatList
    * Messagewindow без send блока
    * Поставить таймер с update
    *
    */

//    auto *chat_timer = new QTimer(this);
//    connect(chat_timer, &QTimer::timeout, [&](){
//      auto [status, chats] = client.get_last_n_dialogs(100);
//      if (chats.size() != chats_id_map.size()) {
//          ui->chatsList->clear();
//          fill_chats();
//      }
//    });
//
//    chat_timer->start(10000);
//
//    auto *message_timer = new QTimer(this);
//    connect(message_timer, &QTimer::timeout, [&]{on_chatsList_itemClicked();});
//
//    message_timer->start(100);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::fill_chats() {
    auto [status, chats] = client.get_last_n_dialogs(100);
    std::cout << chats.size() << "\n";
    for (const auto &chat : chats) {
        QString new_chat(QString::fromStdString(chat.m_name));
        ui->chatsList->addItem(new_chat);
        chats_id_map[new_chat] = chat.m_dialog_id;
    }
}

void MainWindow::on_chatsList_itemClicked(QListWidgetItem *item)
{
    if (item != nullptr) {
        select_chat_id = chats_id_map[item->text()];
    }
    ui->messagesList->clear();
    std::cout << "Message size: ";
    auto [status, messages] = client.get_n_messages(100, select_chat_id);
    std::cout << messages.size() << "\n";
    for (const auto &mess : messages) {
        ui->messagesList->addItem(QString::fromStdString(mess.m_text));
    }
}

void MainWindow::on_findButton_clicked()
{
    std::string find_chat = ui->findLine->text().toStdString();
    if (find_chat.empty()) {
        return;
    }
    unsigned int second_user = std::stoi(find_chat);
    std::cout << "Size client\n";
    if (client.make_dialog(find_chat, "RSA", 1000, false, {client_id, second_user})) {
        std::cout << client.get_last_n_dialogs(100, select_chat_id).second.size() << "\n";
        ui->chatsList->clear();
        fill_chats();
    }
    ///TODO search user and start dialog
}

void MainWindow::on_sendButton_clicked()
{
    QString msg = ui->newMessageInput->toPlainText();
    if (msg.isEmpty()) {
        return;
    }
    std::cout << "send in: ";
    std::cout << select_chat_id << "\n";
    Status a = client.send_message_to_another_user(select_chat_id, 1000, msg.toStdString());
    std::cout << bool(a) <<
        "\n" <<
        a.message() << "\n";
    auto *item = new QListWidgetItem(msg);
    ui->messagesList->addItem(item);
//    auto *bub = new Bubble(msg);
//    ui->messagesList->setItemWidget(item, bub);
    ui->newMessageInput->setPlainText("");
    ui->messagesList->scrollToBottom();

}

void MainWindow::set_client_id(const int &id) {
    client_id = id;
}
