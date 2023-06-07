#include "interface/addGroup.h"
#include "interface/welcWindow.h"
#include "ui_addGroup.h"

AddGroup::AddGroup(MainWindow *mainWin, QWidget *parent) : mainWin(mainWin),
    QDialog(parent),
    ui(new Ui::AddGroup)
{
    ui->setupUi(this);
}

AddGroup::~AddGroup()
{
    delete ui;
}

void AddGroup::on_addGroupBox_accepted()
{
    std::string name_group = ui->nameGroupLine->text().toStdString();
    if (name_group.empty()) {
        return;
    }
    unsigned int cl_id = mainWin->get_client_id();
    std::cout << name_group << "\n";
    if (client.make_dialog(name_group, 1000,true, {cl_id})) {
        mainWin->update_chats();
    }
}

