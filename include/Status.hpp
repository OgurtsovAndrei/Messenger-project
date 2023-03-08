#ifndef STATUS_HPP
#define STATUS_HPP

#include <string>

namespace database_interface {

struct Satus {
  bool m_correct;
  std::string m_message;
  Status(bool correct, const std::string &message = "")
      : m_correct(res), m_message(s) {}
};

} // namespace database_interface
#endif // STATUS_HPP