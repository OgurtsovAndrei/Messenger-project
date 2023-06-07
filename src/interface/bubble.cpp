#include "interface/bubble.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <iostream>
#include <utility>
#include <QPixmap>



Bubble::Bubble(
    const QString &msg_,
    unsigned int msg_id_,
    const ClientInfo &cl_info,
    const bool &incoming_
) : bubLayout(new QGridLayout),
      lbl(new QLabel),
      lbl_name(new QLabel),
      owner_id(cl_info.cl_id),
      msg_id(msg_id_)
{
    lbl->setStyleSheet(
        "QLabel { background-color : #357d50; color : white; border-width: 2px; border-radius: 10px; padding: 6px;}");
    lbl->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    lbl->setWordWrap(true);
    lbl->setText(msg_);

    lbl_name->setStyleSheet("QLabel { color : white; font : 9pt; padding: 6px; }");
    lbl_name->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    lbl_name->setWordWrap(true);
    lbl_name->setText(QString::fromStdString(cl_info.cl_name + " " + cl_info.cl_surname));

    bubLayout->addWidget(lbl_name, 1, 0, 1, 2);
    if (incoming_) {
        bubLayout->addWidget(lbl, 0, 0);
        lbl_name->setAlignment(Qt::AlignLeft);
        bubLayout->setAlignment(Qt::AlignLeft);
    }
    else {
        bubLayout->addWidget(lbl, 0, 1);
        lbl_name->setAlignment(Qt::AlignRight);
        bubLayout->setAlignment(Qt::AlignRight);
    }
    bubLayout->setSpacing(2);
    setLayout(bubLayout);
}

Bubble::Bubble(
    const QString &file_path_,
    const QString &file_name_,
    unsigned int msg_id_,
    const ClientInfo &cl_info,
    const bool &incoming_
) : Bubble(file_name_, msg_id_, cl_info, incoming_){
    lbl_file = new QLabel;
    QPixmap npm("./../images/fileIcon.png");
    QPixmap pm = npm.scaled(QSize(70, 70));
    lbl_file->setPixmap(pm);
    lbl_file->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    lbl_file->setFixedSize(50, 50);
    lbl_file->setScaledContents(true);
    lbl_file->setAlignment(Qt::AlignBottom);
    if (incoming_) {
        bubLayout->addWidget(lbl_file, 0, 1);
    }
    else {
        bubLayout->addWidget(lbl_file, 0, 0);
    }

}

QString Bubble::get_msg_text() const {
    return lbl->text();
}

QString Bubble::get_file_path() const {
    return filePath;
}

unsigned int Bubble::get_owner_id() const {
    return owner_id;
}

unsigned int Bubble::get_msg_id() const {
    return msg_id;
}