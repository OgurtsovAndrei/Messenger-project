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
        int m_dialog_id = 0;
        std::string m_name;
        std::string m_encryption;
        int m_date_time = 0;
        int m_owner_id = 0;
        bool m_is_group = false;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(Dialog, m_dialog_id, m_name, m_encryption, m_date_time, m_owner_id, m_is_group);

        static std::vector<User> *m_users;
        static std::list<Dialog> *m_dialogs;

        Dialog() = default;

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
    };

}  // namespace database_interface
#endif  // DIALOG_HPP