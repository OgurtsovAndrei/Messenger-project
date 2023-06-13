#include "interface/mesSetting.h"
#include "interface/welcWindow.h"
#include "interface/bubble.h"
#include "ui_mesSetting.h"

MesSetting::MesSetting(Bubble *msg, MainWindow *mainWin, bool isFile, QWidget *parent) :
    msg(msg), mainWin(mainWin), isFile(isFile),
    QWidget(parent),
    ui(new Ui::MesSetting)
{
    ui->setupUi(this);
    if (isFile) {
        ui->editButton->close();
        if (msg->get_owner_id() != mainWin->get_client_id()) {
            ui->delButton->close();
            setFixedHeight(30);
        }
    }
    else {
        ui->downloadButton->close();
    }
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
    auto status = mainWin->get_client()->delete_message(msg->get_msg_id());
    close();
}


void MesSetting::on_editButton_clicked()
/// TODO sort
{
    if (isFile) {
        show_popUp("Sorry, you can't edit files\n");
        return ;
    }
    mainWin->change_message(msg);
    close();
}

void MesSetting::on_downloadButton_clicked()
{
    if (!isFile) {
        show_popUp("There is no file to download.\n");
        return ;
    }
    mainWin->download_file(msg->get_msg_text().toStdString());
    close();
}
