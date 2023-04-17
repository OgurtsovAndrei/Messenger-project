#include "../../include/interface/bubble.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>

Bubble::Bubble(const QString &msg, const bool &incoming)
{
    auto *bubLayout = new QHBoxLayout();
    auto *lbl = new QLabel;
/// TODO addicon
    lbl->setStyleSheet(
        "QLabel { background-color : #357d50; color : white; border-width: "
        "2px; border-radius: 10px; padding: 6px;}");
    lbl->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    if (incoming) {
        lbl->setAlignment(Qt::AlignLeft);
    }
    lbl->setText(msg);
    lbl->setWordWrap(true);
    bubLayout->addWidget(lbl);
    setLayout(bubLayout);
}
