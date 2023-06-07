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
#include "FileWorker.hpp"
#include <filesystem>
//#include "User.hpp"

#include <QStringListModel>
#include <QListWidget>
#include <QLineEdit>
#include <QTimer>
#include <QLabel>
#include <QFile>
#include <QFileDialog>
#include <QTextStream>

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
    if (msg.isEmpty() && select_chat_id == -1) {
        return;
    }
    if (msg.isEmpty() && select_chat_id != -1) {
        show_popUp("Message must not be empty");
        return;
    }
    if (select_chat_id == -1) {
        show_popUp("Choose a chat please");
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
    json json_data = json::parse(send_status.message());
    addMessage(msg, json_data["m_message_id"], cl_info);
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
            chat_item->setText(get_sec_user_name_surname(chat.m_dialog_id));
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
    for (const auto &msg : messages) {
        auto [st, user] = client.get_user_by_id(msg.m_user_id);
        if (!st) {
            show_popUp("Problems displaying messages from some users.");
            continue;
        }
        ClientInfo sec_user_info(user);
        addMessage(QString::fromStdString(msg.m_text), msg.m_message_id, sec_user_info);
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
        auto *popUp = new PopUp("Sorry, user with login '" + find_chat + "' doesn't exists", this);
//        popUp->show();
        return;
    }
    unsigned int sec_id = sec_client.m_user_id;
    std::string dialog_name = cl_info.cl_login + "+" + sec_client.m_login;
    if (client.make_dialog(dialog_name, 1000, false, {get_client_id(), sec_id})) {
        update_chats();
    }
}

void MainWindow::addMessage(const QString &msg, unsigned int msg_id, const ClientInfo &sec_user_info, bool isFile)
{
    bool incoming = false;
    if (sec_user_info.cl_id != get_client_id()) {
        incoming = true;
    }
    auto *item = new QListWidgetItem(nullptr, isFile);
    Bubble *bub;
    if (isFile) {
        bub = new Bubble(msg, extract_file_name(msg), msg_id, cl_info, incoming);
    }
    else {
        bub = new Bubble(msg, msg_id, sec_user_info, incoming);
    }
    ui->messagesList->addItem(item);
    ui->messagesList->setItemWidget(item, bub);
    item->setSizeHint(bub->sizeHint());
    ui->messagesList->scrollToBottom();
}

void MainWindow::change_message(QListWidgetItem *msg) {
    if (msg->type()) { // msg->type() = isFile
        show_popUp("Sorry, you can't edit files");
        return ;
    }
    auto bub = dynamic_cast<Bubble*>(ui->messagesList->itemWidget(msg));
    if (bub->get_owner_id() != get_client_id()) {
        show_popUp("You cannot edit another user's messages");
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
    if (select_chat_id == -1) {
        return;
    }
    auto *ch_info = new ChatInfo(select_chat_id, this);
    ch_info->show();
}

unsigned int MainWindow::get_client_id() const {
    return cl_info.cl_id;
}

void MainWindow::set_client_info(const database_interface::User& cl) {
    cl_info = ClientInfo(cl);
    update_chats();
}

void MainWindow::set_change_msg_is(int msg_id) {
    change_msg_id = msg_id;
}

void MainWindow::on_profileButton_clicked()
{
    auto *ch_info = new ChatInfo(this);
    ch_info->show();
}

void MainWindow::on_messagesList_itemDoubleClicked(QListWidgetItem *msg)
{
    auto *mess = new MesSetting(msg, this, msg->type()); // msg->type() = isFile
    mess->show();
}

void show_popUp(const std::string &err_msg) {
    auto *popUp = new PopUp(err_msg);
    popUp->show();
}

QString MainWindow::get_client_name_surname() const {
    return QString::fromStdString(cl_info.cl_name + " " + cl_info.cl_surname);
}

void MainWindow::on_fileButton_clicked() {
    if (select_chat_id == -1) {
        show_popUp("Choose a chat please");
        return;
    }
    QString file_path = QFileDialog::getOpenFileName(this, "Choose File");
    if (file_path.isEmpty()) {
        return;
    }
//    ui->newMessageInput->setPlainText("File '" + extract_file_name(file_path) + "' has been selected");
//    std::cout << "we are upload file correct" << '\n';
    FileWorker::File file(file_path.toStdString());
    std::cout << "filepath: "<< file_path.toStdString() << "\n";
    auto st = client.upload_file(file);
    if (!st) {
        show_popUp("We were unable to send the file.\n Don't worry that's on us.");
    }
//    connect(ui->sendButton, &QPushButton::clicked, this, [&]{
//        std::cout << "we are upload file correct" << '\n';
//            FileWorker::File file(file_path.toStdString());
//            auto st = client.upload_file(file);
//            if (!st) {
//                show_popUp("We were unable to send the file.\n Don't worry that's on us.");
//            }
//        });


//    std::cout << "filename=" << filename.toStdString() << "\n";
}

QString MainWindow::get_sec_user_name_surname(int dialog_id) const {
    auto [status, users] = client.get_users_in_dialog(dialog_id);
    if (!status || users.size() != 2) {
        show_popUp("This dialog is corrupted.\nFor help, please contact at.\nWe will try to help you");
    }
    for (const auto& us : users) {
        if (us.m_user_id != get_client_id()) {
            return QString::fromStdString(us.m_name + " " + us.m_surname);
        }
    }
    return get_client_name_surname();
}

QString extract_file_name(const QString &file_path) {
    QRegularExpression re("(/([^/]*$))");
    QRegularExpressionMatch match_file_name = re.match(file_path);
    if (!match_file_name.hasMatch()) {
        show_popUp("File doesn't contain name");
    }
    return match_file_name.captured(2);
}
