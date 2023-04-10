#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "netClient.hpp"
#include "status.hpp"
#include "register.h"

#include <QStringListModel>

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
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::fill_chats() {
//  Net::Client::Client client("localhost", "12345");
//  client.make_secure_connection();
  auto [status, chats] = client.get_last_n_dialogs(100);
  for (const auto& chat : chats) {
    ui->chatsList->addItems({QString::fromStdString(chat.m_name)});
  }
}
