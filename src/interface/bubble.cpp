#include "../../include/interface/bubble.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <iostream>

Bubble::Bubble(const QString &msg, const bool &incoming) : lbl(new QLabel), bubLayout(new QHBoxLayout)
{
/// TODO addicon
    lbl->setStyleSheet(
        "QLabel { background-color : #357d50; color : white; border-width: "
        "2px; border-radius: 10px; padding: 6px;}");
    lbl->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    lbl->setWordWrap(true);
    lbl->setText(msg);
    bubLayout->addWidget(lbl);
    if (incoming) {
        bubLayout->setAlignment(Qt::AlignLeft);
    }
    else {
        bubLayout->setAlignment(Qt::AlignRight);
    }
    bubLayout->setSpacing(3);
    setLayout(bubLayout);
}
