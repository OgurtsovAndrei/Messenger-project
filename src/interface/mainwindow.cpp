#include "interface/mainwindow.h"
#include "./ui_mainwindow.h"
#include "Net/NetClient.hpp"
#include "Status.hpp"
#include "interface/sureDo.h"
#include "interface/bubble.h"
#include "interface/chatInfo.h"
#include "interface/mesSetting.h"
#include "interface/popUp.h"
#include "FileWorker.hpp"
#include <filesystem>
#include <algorithm>

#include <QStringListModel>
#include <QListWidget>
#include <QLineEdit>
#include <QTimer>
#include <QLabel>
#include <QFile>
#include <QFileDialog>
#include <QTextStream>

MainWindow::MainWindow(Net::Client::Client *client_, QWidget *parent)
    : client(client_), QMainWindow(parent), sure_add_group(new SureDo()), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    this->setWindowTitle("ИШО");
    std::cout << get_client_name_surname().toStdString() << "\n";

    auto *chat_timer_update_all = new QTimer(this);
    connect(chat_timer_update_all, &QTimer::timeout, this, [&]{update_chats(); });

    chat_timer_update_all->start(10000);

    auto *chat_timer = new QTimer(this);
    connect(chat_timer, &QTimer::timeout, this, [&]{
      auto [status, chats] = client->get_last_n_dialogs(100);
      if (chats.size() != ui->chatsList->count()) {
          update_chats();
      }
    });

    chat_timer->start(1000);

    auto *message_timer = new QTimer(this);
    connect(message_timer, &QTimer::timeout, this, [&]{ update_messages(true);});

    message_timer->start(100);

    connect(sure_add_group, &QDialog::accepted, this, [this]() {
        this->add_group(sure_add_group->get_line().toStdString());
    });
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::on_sendButton_clicked() {
    if (select_chat_id == -1) {
        show_popUp("Choose a chat please\n");
        return;
    }

    QString msg = ui->newMessageInput->toPlainText();
    if (file_cancel_mode) {
        sendFile();
        return;
    }
    if (msg.isEmpty() && select_chat_id == -1) {
        return;
    }
    if (msg.isEmpty() && select_chat_id != -1) {
        show_popUp("Message must not be empty\n");
        return;
    }
    if (send_edit_mode) {
        auto status = client->change_sent_message(change_msg_id, msg.toStdString());
        update_messages();
        send_edit_mode = false;
        ui->sendButton->setText("Send");
        ui->newMessageInput->setPlainText("");
        return;
    }
    Status send_status = client->send_message_to_another_user(select_chat_id, "", msg.toStdString());
    if (!send_status) {
        show_popUp("We were unable to send your message.\n");
        return ;
    }
    json json_data = json::parse(send_status.message());
    addMessage(msg, json_data["m_message_id"], cl_info);
    ui->newMessageInput->clear();
}

void MainWindow::update_chats(int n) {
    std::cout << "Chats updates...\n";
    ui->chatsList->clear();
    auto [status, chats] = client->get_last_n_dialogs(n);
    bool select_chat_was_delete = true;
    for (const auto &chat : chats) {
        if (chat.m_dialog_id == select_chat_id) {
            select_chat_was_delete = false;
        }
        auto chat_item = new QListWidgetItem(nullptr, chat.m_dialog_id);
        if (chat.m_is_group) {
            chat_item->setText(QString::fromStdString(chat.m_name));
        }
        else {
            chat_item->setText(get_second_user_name_surname(chat.m_dialog_id));
        }
        ui->chatsList->addItem(chat_item);
    }
    if (select_chat_was_delete && !chats.empty()) {
        auto *null_item = ui->chatsList->item(0);
        select_chat_id = null_item->type();
        ui->chatName->setText(null_item->text());
        update_messages();
    }
    std::cout << "Finish \n";
}

void MainWindow::update_messages(bool update_by_timer, int n) {
    auto [status, messages] = client->get_n_messages(n, select_chat_id);
    if (update_by_timer && messages_all_identical(messages)) {
        return ;
    }
    msg_in_current_chat = messages;
    ui->messagesList->clear();
    std::reverse(messages.begin(), messages.end());
    std::sort(messages.begin(), messages.end(), [](const database_interface::Message& a, const database_interface::Message& b) {
                  return a.m_message_id < b.m_message_id;
              });
    for (const auto &msg : messages) {
        auto [st, user] = client->get_user_by_id(msg.m_user_id);
        if (!st) {
            show_popUp("Problems displaying messages from some users.\n");
            continue;
        }
        ClientInfo sec_user_info(user);
        addMessage(QString::fromStdString(msg.m_text), msg.m_message_id, sec_user_info, (!msg.m_file_name.empty()));
    }
}

bool MainWindow::messages_all_identical(const std::vector<database_interface::Message> &messages) {
    if (messages.size() != msg_in_current_chat.size()) {
        return false;
    }
    for (int i = 0; i < messages.size(); ++i) {
        if (messages[i].m_text != msg_in_current_chat[i].m_text) {
            return false;
        }
    }
    return true;
}

QString extract_file_name(const QString &file_path) {
    QRegularExpression re("(/([^/]*$))");
    QRegularExpressionMatch match_file_name = re.match(file_path);
    if (!match_file_name.hasMatch()) {
        show_popUp("File doesn't contain name.\n");
    }
    return match_file_name.captured(2);
}

void MainWindow::on_chatsList_itemClicked(QListWidgetItem *item)
{
    select_chat_id = item->type();
    ui->chatName->setText(item->text());
    update_messages();
}

void MainWindow::on_findButton_clicked()
{
    std::string find_chat = ui->findLine->text().toStdString();
    if (find_chat.empty()) {
        return;
    }
    auto [status, sec_client] = client->get_user_id_by_login(find_chat);
    if (!status) {
        show_popUp("Sorry, user with login '" + find_chat + "' doesn't exists");
        return;
    }
    unsigned int sec_id = sec_client.m_user_id;
    std::string dialog_name = cl_info.cl_login + "+" + sec_client.m_login;
    if (client->make_dialog(dialog_name, 1000, false, {get_client_id(), sec_id})) {
        update_chats();
        ui->findLine->clear();
    }
    else {
        show_popUp("We were unable to create new dialog.\n");
    }
}

void MainWindow::addMessage(const QString &msg, unsigned int msg_id, const ClientInfo &send_user_info, bool isFile)
{
    bool incoming = false;
    if (send_user_info.cl_id != get_client_id()) {
        incoming = true;
    }
    auto *item = new QListWidgetItem(nullptr, isFile);
    Bubble *bub;
    if (isFile) {
        bub = new Bubble(msg, msg_id, send_user_info, incoming, isFile);
    }
    else {
        bub = new Bubble(msg, msg_id, send_user_info, incoming);
    }
    ui->messagesList->addItem(item);
    ui->messagesList->setItemWidget(item, bub);
    item->setSizeHint(bub->sizeHint());
    ui->messagesList->scrollToBottom();
}

void MainWindow::change_message(Bubble *bub) {
    if (bub->get_owner_id() != get_client_id()) {
        show_popUp("You cannot edit another user's messages\n");
        return ;
    }
    send_edit_mode = true;
    ui->sendButton->setText("Edit");
    ui->newMessageInput->setText(bub->get_msg_text());
}

void MainWindow::on_groupButton_clicked()
{
    sure_add_group->set_text("Enter group name, please.");
    sure_add_group->clear_line();
    sure_add_group->show();
}

void MainWindow::add_group(const std::string &group_name) {
    if (group_name.empty()) {
        return;
    }
    if (client->make_dialog(group_name, 1000,true, {get_client_id()})) {
        update_chats();
    }
}

void MainWindow::on_chatName_clicked()
{
    if (select_chat_id == -1) {
        return;
    }
    auto *ch_info = new ChatInfo(select_chat_id, this, this);
    ch_info->show();
}

unsigned int MainWindow::get_client_id() const {
    return cl_info.cl_id;
}

void MainWindow::set_client_info(const database_interface::User& cl) {
    cl_info = ClientInfo(cl);
    update_chats();
    update_messages();
}

void MainWindow::on_profileButton_clicked()
{
    auto *ch_info = new ChatInfo(this, this);
    ch_info->show();
}

void MainWindow::on_messagesList_itemDoubleClicked(QListWidgetItem *item)
{
    auto *bub = dynamic_cast<Bubble*>(ui->messagesList->itemWidget(item));
    if (bub->get_owner_id() != get_client_id() && !item->type()) {
        return ;
    }
    auto *mesSet = new MesSetting(bub, this, item->type(), this); // item->type() = isFile
    change_msg_id = bub->get_msg_id();
    mesSet->show();
}

void show_popUp(const std::string &err_msg) {
    auto *popUp = new PopUp(err_msg + "For help, please contact us.\nWe will try to solve your problem");
    popUp->adjustSize();
    popUp->show();
}

void show_success_popUp(const std::string &suc_msg) {
    auto popUp = new PopUp(suc_msg);
    popUp->setStyleSheet("QWidget {background: #386E7C; border-radius: 6px; }");
    popUp->adjustSize();
    popUp->change_error_on_success();
    popUp->show();
}

QString MainWindow::get_client_name_surname() const {
    return QString::fromStdString(cl_info.cl_name + " " + cl_info.cl_surname);
}

void MainWindow::on_fileButton_clicked() {
    if (select_chat_id == -1) {
        show_popUp("Choose a chat please\n");
        return;
    }
    if (file_cancel_mode) {
        uploaded_file_name = "";
        file_cancel_mode = false;
        ui->newMessageInput->clear();
        ui->fileButton->setText("File");
        return ;
    }
    QString file_path = QFileDialog::getOpenFileName(this, "Choose File");
    if (file_path.isEmpty()) {
        return;
    }
    FileWorker::File file(file_path.toStdString());
    auto st = client->upload_file(file);
    if (!st) {
        show_popUp("We were unable to upload the file.\n ");
        return ;
    }
    uploaded_file_name = extract_file_name(file_path);
    ui->newMessageInput->setPlainText("File " + uploaded_file_name + " was upload. For send press Send");
    ui->fileButton->setText("Cancel");
    file_cancel_mode = true;
}

QString MainWindow::get_second_user_name_surname(int dialog_id) {
    auto [status, users] = client->get_users_in_dialog(dialog_id);
    if (!status || users.size() != 2) {
        show_popUp("This dialog is corrupted.\n");
    }
    for (const auto& us : users) {
        if (us.m_user_id != get_client_id()) {
            return QString::fromStdString(us.m_name + " " + us.m_surname);
        }
    }
    return get_client_name_surname();
}

int MainWindow::get_cl_encryption_id() const {
    return cl_info.cl_encryption_id;
}

void MainWindow::sendFile() {
    Status send_status = client->send_message_to_another_user(select_chat_id, uploaded_file_name.toStdString(), uploaded_file_name.toStdString());
    if (!send_status) {
        show_popUp("We were unable to send your file.\n");
        return ;
    }
    json json_data = json::parse(send_status.message());
    addMessage(uploaded_file_name, json_data["m_message_id"], cl_info, true);
    uploaded_file_name = "";
    file_cancel_mode = false;
    ui->newMessageInput->clear();
    ui->fileButton->setText("File");
}

void MainWindow::download_file(const std::string &file_name) {
    QString file_download_path = QFileDialog::getExistingDirectory(this, "Choose directory");
    if (file_download_path.isEmpty()) {
        return;
    }
    auto [status, file_to_save] = client->download_file(file_name);
    if (!status) {
        show_popUp("We were unable to download file.\n");
        return ;
    }
    Status save_status = file_to_save.save(file_download_path.toStdString());
    if (!save_status) {
        show_popUp("We were unable to save file.\n");
        return ;
    }
    show_success_popUp("File '" + file_name + "' was successfully saved to directory.\n DIR: " + file_download_path.toStdString() + "\n");
}

Net::Client::Client *MainWindow::get_client() {
    return client;
}
