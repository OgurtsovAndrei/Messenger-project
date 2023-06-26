#ifndef BUBBLE_H
#define BUBBLE_H

#include <QGridLayout>
#include <QString>
#include <QWidget>
#include <QLabel>
#include "clientinfo.h"

class Bubble : public QWidget {
private:
    QGridLayout *bubLayout;
    QLabel *lbl;
    QLabel *lbl_name;
    QLabel *lbl_file;
    unsigned int owner_id;
    unsigned int msg_id;

//    void set_settings_layout_name(const QString &cl_name_sur, const bool &incoming);
public:
    explicit Bubble(const QString &msg_, unsigned int msg_id_, const ClientInfo &cl_info, const bool &incoming_);

    explicit Bubble(const QString &file_name_, unsigned int msg_id_, const ClientInfo &cl_info, const bool &incoming_, bool isFile_);

    [[nodiscard]] QString get_msg_text() const;

    [[nodiscard]] unsigned int get_owner_id() const;

    [[nodiscard]] unsigned int get_msg_id() const;
};

#endif // BUBBLE_H
