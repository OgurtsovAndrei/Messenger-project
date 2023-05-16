#include "interface/mesSetting.h"
#include "interface/welcWindow.h"
#include "ui_mesSetting.h"

MesSetting::MesSetting(QListWidgetItem *mes, MainWindow *mainWin, QWidget *parent) :
    mes(mes), mainWin(mainWin),
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
    auto status = client.delete_message(mes->type());
    if (status) {
        mainWin->update_chats();
    }
}

