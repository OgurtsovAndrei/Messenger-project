#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <list>
#include <utility>

namespace database_interface {

struct Message {
    int m_message_id;
    int m_date_time;
    std::string m_text;
    std::string m_file_path;
    int m_dialog_id{};
    int m_user_id;

    static Message *m_edit_message;
    static std::list<Message> *m_message_list;

    explicit Message(
        int message_id,
        int date_time,
        std::string text,
        std::string file_path,
        int dialog_id,
        int user_id
    )
        : m_message_id(message_id),
          m_date_time(date_time),
          m_text(std::move(text)),
          m_file_path(std::move(file_path)),
          m_dialog_id(dialog_id),
          m_user_id(user_id) {
    }

    explicit Message(
            int message_id,
            int date_time,
            std::string text,
            std::string file_path,
            int user_id
    )
            : m_message_id(message_id),
              m_date_time(date_time),
              m_text(std::move(text)),
              m_file_path(std::move(file_path)),
              m_user_id(user_id) {
    }

    explicit Message(
            int date_time,
            std::string text,
            std::string file_path,
            int dialog_id,
            int user_id
    )
            : m_date_time(date_time),
              m_text(std::move(text)),
              m_file_path(std::move(file_path)),
              m_dialog_id(dialog_id),
              m_user_id(user_id) {
    }

    explicit Message(int message_id) : m_message_id(message_id){
    }

    static int callback_get_message_by_id(void *NotUsed, int argc, char **argv, char **azColName);

    static int callback_get_message_list(void *NotUsed, int argc, char **argv, char **azColName);
};

}  // namespace database_interface
#endif  // MESSAGE_HPP