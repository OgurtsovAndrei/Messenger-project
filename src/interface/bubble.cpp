#include "../../include/interface/bubble.h"
#include "ui_bubble.h"

Bubble::Bubble(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Bubble)
{
    ui->setupUi(this);
    QPixmap pm("/Users/arina/hse/project/Messenger-project/src/interface/pic.jpg");
    ui->label->setPixmap(pm);
}

Bubble::~Bubble()
{
    delete ui;
}
