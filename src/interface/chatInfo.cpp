#include "interface/chatInfo.h"
#include "interface/welcWindow.h"
#include "ui_chatInfo.h"

ChatInfo::ChatInfo(database_interface::Dialog *dialog, QWidget *parent) : dialog(dialog),
    QDialog(parent),
    ui(new Ui::ChatInfo)
{
    ui->setupUi(this);
    this->setWindowTitle("Chat Info");
    ui->userNameLabel->setText(QString::fromStdString(dialog->m_name));
    ui->userNameLabel->setWordWrap(true);
    if (!dialog->m_is_group) {
        ui->addMemButton->hide();
        ui->memList->hide();
        ui->addMemLine->hide();
        ui->memLabel->hide();
        setFixedSize(400, 75);

    }
    else {
//        get_users_in_dialog
    }
}

ChatInfo::~ChatInfo()
{
    delete ui;
}

void ChatInfo::on_addMemButton_clicked()
{
    std::string add_mem = ui->addMemLine->text().toStdString();
    if (add_mem.empty()) {
        return;
    }
    auto [status, sec_client] = client.get_user_id_by_login(add_mem);
    if (!status) {
        return;
    }
}

