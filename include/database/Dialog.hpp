#ifndef DIALOG_HPP
#define DIALOG_HPP

#include <string>
#include <utility>
#include <vector>
#include <list>
#include "User.hpp"
#include "../Status.hpp"
#include "../TextWorker.hpp"

namespace database_interface {

    struct Dialog {
        int m_dialog_id;
        std::string m_name;
        std::string m_encryption;
        int m_date_time;
        int m_owner_id;
        bool m_is_group;

        static std::vector<User> *m_users;
        static std::list<Dialog> *m_dialogs;

        explicit Dialog(int dialog_id) : m_dialog_id(dialog_id) {}

        explicit Dialog(
                std::string name,
                std::string encryption,
                int owner,
                bool is_group
        )
                : m_name(std::move(name)),
                  m_encryption(std::move(encryption)),
                  m_owner_id(owner),
                  m_is_group(is_group),
                  m_dialog_id(-1) {
        }

        explicit Dialog(
                std::string name,
                std::string encryption,
                int date_time,
                int owner,
                bool is_group
        )
                : m_name(std::move(name)),
                  m_encryption(std::move(encryption)),
                  m_date_time(date_time),
                  m_owner_id(owner),
                  m_is_group(is_group),
                  m_dialog_id(-1) {
        }

        explicit Dialog(
                int dialog_id,
                std::string name,
                std::string encryption,
                int date_time,
                int owner,
                bool is_group) :
                m_dialog_id(dialog_id),
                m_name(std::move(name)),
                m_encryption(std::move(encryption)),
                m_date_time(date_time),
                m_owner_id(owner),
                m_is_group(is_group) {
        }

        static int callback_get_dialog_users(void *NotUsed, int argc, char **argv, char **azColName);

        static int callback_get_dialogs(void *NotUsed, int argc, char **argv, char **azColName);

        [[nodiscard]] std::string to_string() const {
            std::vector<std::string> dialog_data{std::to_string(m_dialog_id), m_name,
                                                 m_encryption, std::to_string(m_date_time),
                                                 std::to_string(m_owner_id), std::to_string(m_is_group)};
            return std::move(convert_text_vector_to_text(dialog_data));
        }

        static Status parse_to_dialog(const std::string &text_data, Dialog &ref_to_answer_dialog) {
            std::vector<std::string> dialog_data = convert_to_text_vector_from_text(text_data);
            bool data_is_correct = true;
            data_is_correct &= is_number(dialog_data[0]);
            data_is_correct &= is_number(dialog_data[3]);
            data_is_correct &= is_number(dialog_data[4]);
            data_is_correct &= is_number(dialog_data[5]);

            if (!data_is_correct) {
                return Status(false, "Bad data: one of int expected fields is not int");
            }

            ref_to_answer_dialog.m_dialog_id = std::stoi(dialog_data[0]);
            ref_to_answer_dialog.m_name = dialog_data[1];
            ref_to_answer_dialog.m_encryption = dialog_data[2];
            ref_to_answer_dialog.m_date_time = std::stoi(dialog_data[3]);
            ref_to_answer_dialog.m_owner_id = std::stoi(dialog_data[4]);
            ref_to_answer_dialog.m_is_group = std::stoi(dialog_data[5]);
            return Status(true, "");
        }


    };

}  // namespace database_interface
#endif  // DIALOG_HPP