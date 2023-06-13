#include "interface/chatSetting.h"
#include "ui_chatSetting.h"
#include "interface/welcWindow.h"
#include "interface/mainwindow.h"
#include "interface/sureDo.h"
#include <QDialog>

ChatSetting::ChatSetting(unsigned int sec_cl_id_, QString sec_cl_name_, std::string dialog_name_, ChatInfo *chat_info_, QWidget *parent) :
    sec_cl_id(sec_cl_id_), sec_cl_name(std::move(sec_cl_name_)), dialog_name(std::move(dialog_name_)), chat_info(chat_info_), sure_del(new SureDo()),
    QWidget(parent),
    ui(new Ui::ChatSetting)
{
    client_id = chat_info->get_owner_id();
    dialog_id = chat_info->get_dialog_id();
    ui->setupUi(this);
    setFixedHeight(60);
    sure_del->close_line();
    this->setWindowFlag(Qt::Popup, true);
    auto cursor_point = QWidget::mapFromGlobal(QCursor::pos());
    setGeometry(cursor_point.x(), cursor_point.y(), 130, 60);

    connect(sure_del, &QDialog::accepted, this, [=]() {
        auto status = client.del_user_from_dialog(static_cast<int>(sec_cl_id), static_cast<int>(dialog_id));
        if (!status) {
            show_popUp("We were unable to delete user from dialog.\n");
        }
        chat_info->update_dialog();
    });
}

ChatSetting::ChatSetting(unsigned int client_id_, unsigned int sec_cl_id_, std::string dialog_name_, QWidget *parent) :
    client_id(client_id_), sec_cl_id(sec_cl_id_), dialog_name(std::move(dialog_name_)),
    QWidget(parent),
    ui(new Ui::ChatSetting)
{
    ui->setupUi(this);
    ui->delMemButton->close();
    setFixedHeight(30);
    this->setWindowFlag(Qt::Popup, true);
    auto cursor_point = QWidget::mapFromGlobal(QCursor::pos());
    setGeometry(cursor_point.x(), cursor_point.y(), 130, 30);
}

ChatSetting::~ChatSetting()
{
    delete ui;
}

void ChatSetting::on_startChatButton_clicked()
{
    if (!client.make_dialog(dialog_name, 1000, false, {client_id, sec_cl_id})) {
        show_popUp("We were unable to create new dialog.\n");
    }
    close();
}


void ChatSetting::on_delMemButton_clicked()
{
    if (chat_info->get_owner_id() == sec_cl_id) {
        show_popUp("you cannot remove yourself from the dialog.\n");
        return ;
    }
    sure_del->set_text("Are you sure you want to remove '" + sec_cl_name + "' from dialog?");
    sure_del->show();
}

