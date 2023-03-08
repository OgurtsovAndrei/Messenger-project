#ifndef MESSENGE_HPP
#define MESSENGE_HPP

#include <string>

namespace database_interface {

struct Messenge {
  int m_id;
  int m_date_time;
  std::string m_text;
  std::string m_file_path;
  int m_chat_id;

  Status(int id, int date_time, const std::string &text,
         const std::string &file_path, int chat_id)
      : m_id(id), m_date_time(date_time), m_text(text), m_file_path(file_path),
        m_chat_id(chat_id) {}
};

} // namespace database_interface
#endif // MESSENGE_HPP