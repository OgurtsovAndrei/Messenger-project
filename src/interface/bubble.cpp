#include "../../include/interface/bubble.h"
#include "ui_bubble.h"

Bubble::Bubble(const QString &msg, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Bubble)
{
    ui->setupUi(this);
    QPixmap pm("/Users/arina/hse/project/Messenger-project/src/interface/pic.jpg");
    ui->label->setPixmap(pm);
    ui->textBrowser->setText(msg);
}

Bubble::~Bubble()
{
    delete ui;
}
