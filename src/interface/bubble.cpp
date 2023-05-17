#include "interface/bubble.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <iostream>

Bubble::Bubble(const QString &msg, const QString &cl_name_sur, const bool &incoming) : lbl(new QLabel), lbl_name(new QLabel), bubLayout(new QVBoxLayout)
{
/// TODO addicon
    lbl->setStyleSheet(
        "QLabel { background-color : #357d50; color : white; border-width: 2px; border-radius: 10px; padding: 6px;}");
    lbl->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    lbl->setWordWrap(true);
    lbl->setText(msg);

    lbl_name->setStyleSheet("QLabel { color : white; font : 9pt; padding: 6px; }");
    lbl_name->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    lbl_name->setWordWrap(true);
    lbl_name->setText(cl_name_sur);

    bubLayout->addWidget(lbl);
    bubLayout->addWidget(lbl_name);
    if (incoming) {
        lbl_name->setAlignment(Qt::AlignLeft);
        bubLayout->setAlignment(Qt::AlignLeft);
    }
    else {
        lbl_name->setAlignment(Qt::AlignRight);
        bubLayout->setAlignment(Qt::AlignRight);
    }
    bubLayout->setSpacing(2);
    setLayout(bubLayout);
}
