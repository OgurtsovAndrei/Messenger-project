#ifndef CHATINFO_H
#define CHATINFO_H

#include <QDialog>
#include <interface/mainwindow.h>

namespace Ui {
class ChatInfo;
}

class ChatInfo : public QDialog
{
    Q_OBJECT

public:
    explicit ChatInfo(int dialog_id, MainWindow *mainWin, QWidget *parent = nullptr);
    explicit ChatInfo(MainWindow *mainWin, QWidget *parent = nullptr);
    ~ChatInfo();

private slots:
    void on_addMemButton_clicked();

    void close_group_buttons();

    void on_encrOptions_activated(int index);

    void on_memList_itemDoubleClicked(QListWidgetItem *item);

private:
    Ui::ChatInfo *ui;
    int dialog_id{};
    int owner_id{};
    MainWindow *mainWin;
};

#endif // CHATINFO_H
