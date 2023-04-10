#include "../../include/interface/mainwindow.h"
#include <QStringListModel>
#include "ui_mainWindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    this->setWindowTitle("ИШО");

    this->setStyleSheet(
        ".QPushButton {background-color : #EE6070;border-width: 2px; "
        "border-style: solid; border-radius: 10px; padding: 6px;} ");
    ui->newMessageInput->setStyleSheet(
        ".QTextEdit {background-color : #EE6080;border-width: 2px; "
        "border-radius: 10px; padding: 6px;}");
    ui->chatsList->setStyleSheet(
        ".QListWidget {background-color : #EE6060;border-width: 2px; "
        "border-radius: 10px; padding: 6px;}");
    ui->messagesList->setStyleSheet(
        ".QListWidget {background-color : #606060;border-width: 2px; "
        "border-radius: 10px; padding: 6px;}");


    ui->chatsList->addItems({"User1 ", "User2", "User3"});

//    ui->chatsList->setModel(new QStringListModel(List));


//    ui->chatsList->setEditTriggers(QAbstractItemView::AnyKeyPressed | QAbstractItemView::DoubleClicked);
//    ui->sendButton->setVisible(false);
//    ui->newMessageInput->setVisible(false); //or true - later in the code
/* Проинициализоровать
 * ChatList
 * Messagewindow без send блока
 *
 *
 */
}

MainWindow::~MainWindow()
{
    delete ui;
}

