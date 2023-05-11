#ifndef CHATINFO_H
#define CHATINFO_H

#include <QDialog>
#include "database/Dialog.hpp"

namespace Ui {
class ChatInfo;
}

class ChatInfo : public QDialog
{
    Q_OBJECT

public:
    explicit ChatInfo(database_interface::Dialog *dialog, QWidget *parent = nullptr);
    ~ChatInfo();

private slots:
    void on_addMemButton_clicked();

private:
    Ui::ChatInfo *ui;
    database_interface::Dialog *dialog;
};

#endif // CHATINFO_H
