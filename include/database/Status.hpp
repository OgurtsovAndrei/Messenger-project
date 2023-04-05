#ifndef STATUS_HPP
#define STATUS_HPP

#include <string>

namespace database_interface {

struct Status {
private:
    bool m_correct;
    std::string m_message;

public:
    Status(bool correct, const std::string &message = "")
        : m_correct(correct), m_message(message) {
    }

    bool correct(){
        return m_correct;
    }

    std::string message(){
        return m_message;
    }
};

}  // namespace database_interface
#endif  // STATUS_HPP