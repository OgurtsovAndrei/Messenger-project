#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "netClient.hpp"
#include "register.h"
#include "status.hpp"

#include <QStringListModel>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  this->setWindowTitle("ИШО");
  /// TODO set user info
  ///
  ui->chatsList->addItems({"user1", "user2"});

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

  auto *chat_timer = new QTimer(this);
  connect(chat_timer, &QTimer::timeout, [&](){
      auto [status, chats] = client.get_last_n_dialogs(100);
      if (chats.size() != chats_id_map.size()) {
          ui->chatsList->clear();
          fill_chats();
      }
  });

  chat_timer->start(10000);

  auto *message_timer = new QTimer(this);
  connect(message_timer, &QTimer::timeout, [&]{on_chatsList_itemClicked();});

  message_timer->start(100);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::fill_chats() {
    auto [status, chats] = client.get_last_n_dialogs(100);
    for (const auto &chat : chats) {
        QListWidgetItem new_chat(QString::fromStdString(chat.m_name));
        ui->chatsList->addItem(&new_chat);
        chats_id_map[new_chat] = chat.m_dialog_id;
    }
}

void MainWindow::on_chatsList_itemClicked(QListWidgetItem *item)
{
    if (item != nullptr) {
        select_chat_id = chats_id_map[*item];
    }
    ui->messagesList->clear();
    auto [status, messages] = client.get_n_messages(100, select_chat_id);
    for (const auto &mess : messages) {
        ui->messagesList->addItems({QString::fromStdString(mess.m_text)});
    }
}
