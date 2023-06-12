#ifndef CHATSETTING_H
#define CHATSETTING_H

#include <QWidget>
#include <QDialog>
#include <interface/sureDo.h>

namespace Ui {
class ChatSetting;
}

class ChatSetting : public QWidget
{
    Q_OBJECT

public:
    explicit ChatSetting(unsigned int client_id, unsigned int sec_cl_id, unsigned int dialog_id, QString sec_cl_name, std::string dialog_name, QWidget *parent = nullptr);
    explicit ChatSetting(unsigned int client_id, unsigned int sec_cl_id, std::string dialog_name, QWidget *parent = nullptr);
    ~ChatSetting();

private slots:
    void on_startChatButton_clicked();

    void on_delMemButton_clicked();

private:
    Ui::ChatSetting *ui;
    SureDo *sure_del;
    unsigned int dialog_id;
    unsigned int client_id;
    unsigned int sec_cl_id;
    QString sec_cl_name;
    std::string dialog_name;
};

#endif // CHATSETTING_H
