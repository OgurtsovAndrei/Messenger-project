#include "interface/popUp.h"
#include "ui_popUp.h"
#include "QScreen"
#include "QApplication"

PopUp::PopUp(const std::string& error_msg, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PopUp)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Popup);
    QRect screenRect = QApplication::primaryScreen()->geometry();
    int x = (screenRect.width() - width()) / 2;
    int y = (screenRect.height() - height()) / 2;
    this->move(x, y);
    ui->msgLabel->setText(QString::fromStdString(error_msg));
    ui->msgLabel->adjustSize();
    ui->msgLabel->setAlignment(Qt::AlignCenter);
    ui->msgLabel->setWordWrap(true);
}

PopUp::~PopUp()
{
    delete ui;
}

void PopUp::change_error_on_success() {
    ui->mainLabel->setText("SUCCESS");
}
