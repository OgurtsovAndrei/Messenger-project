#include "interface/chatSetting.h"
#include "ui_chatSetting.h"
#include "interface/welcWindow.h"
#include "interface/mainwindow.h"
#include "interface/sureDo.h"
#include <QComboBox>
#include <QDialog>

ChatSetting::ChatSetting(unsigned int client_id, unsigned int sec_cl_id, unsigned int dialog_id, QString sec_cl_name, std::string dialog_name, QWidget *parent) :
    client_id(client_id), sec_cl_id(sec_cl_id), dialog_id(dialog_id), sec_cl_name(std::move(sec_cl_name)), dialog_name(std::move(dialog_name)), sure_del(new SureDo("")),
    QWidget(parent),
    ui(new Ui::ChatSetting)
{
    ui->setupUi(this);
    setFixedHeight(60);
    this->setWindowFlag(Qt::Popup, true);
    auto cursor_point = QWidget::mapFromGlobal(QCursor::pos());
    setGeometry(cursor_point.x(), cursor_point.y(), 130, 60);

    connect(sure_del, &QDialog::accepted, this, [&]() {
        auto status = client.del_user_from_dialog(sec_cl_id, dialog_id);
        if (!status) {
            show_popUp("We were unable to delite user from dialog.\n");
        }
    });
}

ChatSetting::ChatSetting(unsigned int client_id, unsigned int sec_cl_id, std::string dialog_name, QWidget *parent) :
    client_id(client_id), sec_cl_id(sec_cl_id), dialog_name(std::move(dialog_name)),
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
    if (client_id == sec_cl_id) {
        show_popUp("you cannot remove yourself from the dialog.\n");
        return ;
    }
    sure_del->set_text("Are you sure you want to remove '" + sec_cl_name + "' from dialog?");
    sure_del->show();
}

