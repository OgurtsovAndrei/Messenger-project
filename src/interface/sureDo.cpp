#include "interface/sureDo.h"
#include "ui_sureDo.h"

SureDo::SureDo(QString text, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SureDo)
{
    ui->setupUi(this);
}

SureDo::~SureDo()
{
    delete ui;
}

void SureDo::set_text(QString text) {
    ui->questLabel->setText(text);
}
