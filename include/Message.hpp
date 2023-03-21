#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>

namespace database_interface {

struct Message {
    int m_message_id;
    int m_date_time;
    std::string m_text;
    std::string m_file_path;
    int m_dialog_id;

    Message(
        int message_id,
        int date_time,
        const std::string &text,
        const std::string &file_path,
        int dialog_id
    )
        : m_message_id(message_id),
          m_date_time(date_time),
          m_text(text),
          m_file_path(file_path),
          m_dialog_id(dialog_id) {
    }
};

}  // namespace database_interface
#endif  // MESSAGE_HPP