#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QStringListModel>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    this->setWindowTitle("ИШО");

    this->setStyleSheet(
        ".QPushButton {background-color : #294d42;border-width: 2px; "
        "border-style: solid; border-radius: 5px; padding: 6px;} ");
    ui->newMessageInput->setStyleSheet(
        ".QTextEdit {background-color : #294d52;border-width: 2px; "
        "border-radius: 5px; padding: 6px;}");
    ui->chatsList->setStyleSheet(
        ".QListWidget {background-color : #294d62;border-width: 2px; "
        "border-radius: 5px; padding: 6px;}");
    ui->messagesList->setStyleSheet(
        ".QListWidget {background-color : #294d72;border-width: 2px; "
        "border-radius: 5px; padding: 6px;}");


    ui->chatsList->addItems({"User1 ", "User2", "User3"});

    fill_chats();



//    ui->chatsList->setModel(new QStringListModel(List));


//    ui->chatsList->setEditTriggers(QAbstractItemView::AnyKeyPressed | QAbstractItemView::DoubleClicked);
//    ui->sendButton->setVisible(false);
//    ui->newMessageInput->setVisible(false); //or true - later in the code
/* Проинициализоровать
 * ChatList
 * Messagewindow без send блока
 * Поставить таймер с update
 *
 */
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::fill_chats() {
}

