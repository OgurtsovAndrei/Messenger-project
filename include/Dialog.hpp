#ifndef DIALOG_HPP
#define DIALOG_HPP

#include <string>
#include "User.hpp"

namespace database_interface {

struct Dialog {
    int m_dialog_id;
    std::string m_encryption;

    Chat(int dialog_id, const std::string &encryption, )
        : m_dialog_id(dialog_id), m_encryption(encryption) {
    }
};

}  // namespace database_interface
#endif  // DIALOG_HPP