#include "interface/sureDo.h"
#include "ui_sureDo.h"

SureDo::SureDo(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SureDo)
{
    ui->setupUi(this);
}

SureDo::~SureDo()
{
    delete ui;
}

void SureDo::set_text(const QString& text) {
    ui->questLabel->setText(text);
}

void SureDo::close_line() const {
    ui->lineEdit->close();
}

QString SureDo::get_line() const {
    return ui->lineEdit->text();
}
