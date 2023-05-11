#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include "Status.hpp"
#include <string>
#include <list>
#include <utility>
#include "TextWorker.hpp"
#include "nlohmann/json.hpp"

namespace database_interface {

    struct Message {
        int m_message_id;
        int m_date_time;
        std::string m_text;
        std::string m_file_path;
        int m_dialog_id{};
        int m_user_id;
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(Message, m_message_id, m_date_time, m_text, m_file_path, m_dialog_id, m_user_id);

        static Message *m_edit_message;
        static std::list<Message> *m_message_list;

        explicit Message() : m_message_id(0),
                             m_date_time(0),
                             m_dialog_id(0),
                             m_user_id(0) {}

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

        explicit Message(
                std::string text,
                std::string file_path,
                int dialog_id,
                int user_id
        )
                : m_text(std::move(text)),
                  m_file_path(std::move(file_path)),
                  m_dialog_id(dialog_id),
                  m_user_id(user_id) {
        }

        explicit Message(int message_id) : m_message_id(message_id) {
        }

        [[nodiscard]] std::string to_strint() const {
            // FIXME ужасно не оптимально по трафику данные так гонять, но пока что как есть, другого не дано.
            // В качестве альтернативы вижу конвертацию в reinterpret_cast<unsigned char*>, но тут возникают со стрингами проблемы
            // Если есть предложения, пишите.
            std::vector<std::string> message_data{std::to_string(m_message_id), std::to_string(m_date_time),
                                                  m_text, m_file_path, std::to_string(m_dialog_id),
                                                  std::to_string(m_user_id)};
            return convert_text_vector_to_text(message_data);
        }

        static Status parse_to_message(const std::string &text_data, Message &ref_to_answer_message) {
            std::vector<std::string> message_data = convert_to_text_vector_from_text(text_data);
            bool data_is_correct = true;
            data_is_correct &= is_number(message_data[0]);
            data_is_correct &= is_number(message_data[1]);
            data_is_correct &= is_number(message_data[4]);
            data_is_correct &= is_number(message_data[5]);

            if (!data_is_correct) {
                return Status(false, "Bad data: one of int expected fields is not int");
            }

            ref_to_answer_message.m_message_id = std::stoi(message_data[0]);
            ref_to_answer_message.m_date_time = std::stoi(message_data[1]);
            ref_to_answer_message.m_text = message_data[2];
            ref_to_answer_message.m_file_path = message_data[3];
            ref_to_answer_message.m_dialog_id = std::stoi(message_data[4]);
            ref_to_answer_message.m_user_id = std::stoi(message_data[5]);
            return Status(true, "");
        }

        static int callback_get_message_by_id(void *NotUsed, int argc, char **argv, char **azColName);

        static int callback_get_message_list(void *NotUsed, int argc, char **argv, char **azColName);
    };

}  // namespace database_interface
#endif  // MESSAGE_HPP