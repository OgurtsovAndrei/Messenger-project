#ifndef STATUS_HPP
#define STATUS_HPP

#include <string>
#include <utility>

struct Status {
private:
    bool m_correct;
    std::string m_message;

public:
    explicit Status() : m_correct(false), m_message("This status is only initialized and not determined") {}

    explicit Status(bool correct, std::string message = "")
            : m_correct(correct), m_message(std::move(message)) {}

    explicit operator bool() const {
        return m_correct;
    }

    [[nodiscard]] bool correct() const {
        return m_correct;
    }

    [[nodiscard]] std::string message() const {
        return m_message;
    }
};

#endif  // STATUS_HPP