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
    QString filePath;

//    void set_settings_layout_name(const QString &cl_name_sur, const bool &incoming);
public:
    explicit Bubble(const QString &msg_, unsigned int msg_id_, const ClientInfo &cl_info, const bool &incoming_);

    explicit Bubble(const QString &file_path_, const QString &file_name_, unsigned int msg_id_, const ClientInfo &cl_info, const bool &incoming_);

    explicit Bubble(const QString &msg_, const QString &cl_name_sur_, unsigned int owner_id_, const bool &incoming_);

    explicit Bubble(const QString &msg_, QString filePath_, const QString &cl_name_sur_, unsigned int owner_id_, const bool &incoming_);

    [[nodiscard]] QString get_msg_text() const;

    [[nodiscard]] unsigned int get_owner_id() const;

    [[nodiscard]] unsigned int get_msg_id() const;

    [[nodiscard]] QString get_file_path() const;
};

#endif // BUBBLE_H
