#ifndef CHATINFO_H
#define CHATINFO_H

#include <QDialog>
#include <interface/mainwindow.h>
#include "interface/sureDo.h"

namespace Ui {
class ChatInfo;
}

class ChatInfo : public QDialog
{
    Q_OBJECT

public:
    explicit ChatInfo(int dialog_id_, MainWindow *mainWin, QWidget *parent = nullptr);
    explicit ChatInfo(MainWindow *mainWin, QWidget *parent = nullptr);
    ~ChatInfo();

    [[nodiscard]] int get_owner_id() const;

    [[nodiscard]] int get_dialog_id() const;

    void update_dialog();

private slots:
    void on_addMemButton_clicked();

    void close_group_buttons();

    void on_encrOptions_activated(int index);

    void on_memList_itemDoubleClicked(QListWidgetItem *item);

    void on_changeNameButton_clicked();

    void on_changeSnameButton_clicked();

    void on_changeLogButton_clicked();

private:
    Ui::ChatInfo *ui;
    int dialog_id{};
    int owner_id{};
    MainWindow *mainWin;
    SureDo* sure_change;
    enum class LAST_MODIFIED{ NAME, SURNAME, LOGIN, NONE };
    LAST_MODIFIED last_mod = LAST_MODIFIED::NONE;
};

#endif // CHATINFO_H
