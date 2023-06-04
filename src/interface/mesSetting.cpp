#include "interface/mesSetting.h"
#include "interface/welcWindow.h"
#include "interface/bubble.h"
#include "ui_mesSetting.h"

MesSetting::MesSetting(QListWidgetItem *msg, MainWindow *mainWin, QWidget *parent) :
    msg(msg), mainWin(mainWin),
    QWidget(parent),
    ui(new Ui::MesSetting)
{
    ui->setupUi(this);
    this->setWindowFlag(Qt::Popup, true);
    auto cursor_point = QWidget::mapFromGlobal(QCursor::pos());
    setGeometry(cursor_point.x(), cursor_point.y(), 130, 60);
}

MesSetting::~MesSetting()
{
    delete ui;
}

void MesSetting::on_delButton_clicked()
{
    auto status = client.delete_message(dynamic_cast<Bubble*>(msg)->get_msg_id());
    close();
}


void MesSetting::on_editButton_clicked()
{
    mainWin->set_change_msg_is(dynamic_cast<Bubble*>(msg)->get_msg_id());
    mainWin->change_message(msg);
    close();
}

void MesSetting::on_downloadButton_clicked()
{
    mainWin->set_change_msg_is(dynamic_cast<Bubble*>(msg)->get_msg_id());
    mainWin->change_message(msg);
    close();
}


void MesSetting::close_downoload() {
    ui->downloadButton->close();
    setFixedHeight(60);
}

void MesSetting::close_edit() {
    ui->editButton->close();
    setFixedHeight(60);
}
