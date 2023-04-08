#ifndef STATUS_HPP
#define STATUS_HPP

#include <string>

namespace database_interface {

struct Status {
    bool m_correct;
    std::string m_message;

    explicit Status(bool correct, const std::string &message = "")
        : m_correct(correct), m_message(message) {
    }
};

}  // namespace database_interface
#endif  // STATUS_HPP