#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QMap>
#include "clientinfo.h"
#include "database/User.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QString get_cur_chat_name() const;

    void set_client_info(const database_interface::User& cl);

    unsigned int get_client_id() const;

    void update_chats();

private slots:

    void on_chatsList_itemClicked(QListWidgetItem *item = nullptr);

    void on_findButton_clicked();

    void addMessage(const QString &msg, const int mess_id, const QString &name_sur, const bool &incoming = false);

    void on_groupButton_clicked();

    void on_chatName_clicked();

    void on_profileButton_clicked();

    void on_messagesList_itemDoubleClicked(QListWidgetItem *item);

private:
    Ui::MainWindow *ui;
    QMap<QString, int> chats_id_map;
    ClientInfo cl_info;
    int select_chat_id = 0;

};

#endif // MAINWINDOW_H
