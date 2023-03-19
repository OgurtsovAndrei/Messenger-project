#include <iostream>

#include <botan/asio_stream.h>
#include <botan/auto_rng.h>
#include <botan/certstor_system.h>
#include <botan/tls_session_manager_noop.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/bind.hpp>

namespace http = boost::beast::http;
namespace _ = boost::asio::placeholders;

// very basic credentials manager
class Credentials_Manager : public Botan::Credentials_Manager {
public:
  Credentials_Manager() {}

  std::vector<Botan::Certificate_Store *>
  trusted_certificate_authorities(const std::string &, const std::string &) override {
    return {&m_cert_store};
  }

private:
  Botan::System_Certificate_Store m_cert_store;
};

// a simple https client based on TLS::Stream
class client {
public:
  client(boost::asio::io_context &io_context,
         boost::asio::ip::tcp::resolver::iterator endpoint_iterator,
         http::request<http::string_body> req)
      : m_request(req),
        m_ctx(m_credentials_mgr, m_rng, m_session_mgr, m_policy, Botan::TLS::Server_Information()),
        m_stream(io_context, m_ctx) {
    boost::asio::async_connect(m_stream.lowest_layer(), endpoint_iterator,
                               boost::bind(&client::handle_connect, this, _::error));
  }

  void handle_connect(const boost::system::error_code &error) {
    if (error) {
      std::cout << "Connect failed: " << error.message() << "\n";
      return;
    }
    m_stream.async_handshake(Botan::TLS::Connection_Side::Client,
                             boost::bind(&client::handle_handshake, this, _::error));
  }

  void handle_handshake(const boost::system::error_code &error) {
    if (error) {
      std::cout << "Handshake failed: " << error.message() << "\n";
      return;
    }
    http::async_write(m_stream, m_request,
                      boost::bind(&client::handle_write, this, _::error, _::bytes_transferred));
  }

  void handle_write(const boost::system::error_code &error, size_t) {
    if (error) {
      std::cout << "Write failed: " << error.message() << "\n";
      return;
    }
    http::async_read(m_stream, m_reply, m_response,
                     boost::bind(&client::handle_read, this, _::error, _::bytes_transferred));
  }

  void handle_read(const boost::system::error_code &error, size_t) {
    if (!error) {
      std::cout << "Reply: ";
      std::cout << m_response.body() << "\n";
    } else {
      std::cout << "Read failed: " << error.message() << "\n";
    }
  }

private:
  http::request<http::dynamic_body> m_request;
  http::response<http::string_body> m_response;
  boost::beast::flat_buffer m_reply;

  Botan::TLS::Session_Manager_Noop m_session_mgr;
  Botan::AutoSeeded_RNG m_rng;
  Credentials_Manager m_credentials_mgr;
  Botan::TLS::Policy m_policy;

  Botan::TLS::Context m_ctx;
  Botan::TLS::Stream<boost::asio::ip::tcp::socket> m_stream;
};

int main() {
  boost::asio::io_context io_context;

  boost::asio::ip::tcp::resolver resolver(io_context);
  boost::asio::ip::tcp::resolver::query query("botan.randombit.net", "443");
  boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);

  http::request<http::string_body> req;
  req.version(11);
  req.method(http::verb::get);
  req.target("/news.html");
  req.set(http::field::host, "botan.randombit.net");

  client c(io_context, iterator, req);

  io_context.run();

  return 0;
}
