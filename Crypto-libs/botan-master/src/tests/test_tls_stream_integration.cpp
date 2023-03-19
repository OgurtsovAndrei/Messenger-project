/*
* TLS ASIO Stream Client-Server Interaction Test
* (C) 2018-2020 Jack Lloyd
*     2018-2020 Hannes Rantzsch, René Meusel
*     2022      René Meusel, Rohde & Schwarz Cybersecurity
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include "tests.h"

#if defined(BOTAN_HAS_TLS) && defined(BOTAN_HAS_TLS_ASIO_STREAM) && defined(BOTAN_TARGET_OS_HAS_THREADS)

// first version to be compatible with Networking TS (N4656) and boost::beast
#include <boost/version.hpp>
#if BOOST_VERSION >= 106600

#include <functional>
#include <thread>
#include <optional>

#include <botan/asio_stream.h>
#include <botan/auto_rng.h>
#include <botan/tls_session_manager_noop.h>

#include <boost/asio.hpp>
#include <boost/exception/exception.hpp>

#include "../cli/tls_helpers.h"  // for Basic_Credentials_Manager

namespace {

namespace net = boost::asio;

using tcp = net::ip::tcp;
using error_code = boost::system::error_code;
using ssl_stream = Botan::TLS::Stream<net::ip::tcp::socket>;
using namespace std::placeholders;
using Result = Botan_Tests::Test::Result;

static const auto k_timeout = std::chrono::seconds(30);
static const auto k_endpoints = std::vector<tcp::endpoint>
   {
   tcp::endpoint{net::ip::make_address("127.0.0.1"), 8082}
   };

enum { max_msg_length = 512 };

static std::string server_cert()
   {
   return Botan_Tests::Test::data_dir() + "/x509/certstor/cert1.crt";
   }
static std::string server_key()
   {
   return Botan_Tests::Test::data_dir() + "/x509/certstor/key01.pem";
   }

class Timeout_Exception : public std::runtime_error
   {
      using std::runtime_error::runtime_error;
   };

class Peer
   {
   public:
      Peer(Botan::TLS::Policy& policy, net::io_context& ioc)
         : m_credentials_manager(true, ""),
           m_ctx(m_credentials_manager, m_rng, m_session_mgr, policy, Botan::TLS::Server_Information()),
           m_timeout_timer(ioc) {}

      Peer(Botan::TLS::Policy& policy, net::io_context& ioc, const std::string& server_cert, const std::string& server_key)
         : m_credentials_manager(server_cert, server_key),
           m_ctx(m_credentials_manager, m_rng, m_session_mgr, policy, Botan::TLS::Server_Information()),
           m_timeout_timer(ioc) {}

      virtual ~Peer()
         {
         cancel_timeout();
         }

      net::mutable_buffer buffer()
         {
         return net::buffer(m_data, max_msg_length);
         }
      net::mutable_buffer buffer(size_t size)
         {
         return net::buffer(m_data, size);
         }

      std::string message() const
         {
         return std::string(m_data);
         }

      // This is a CompletionCondition for net::async_read().
      // Our toy protocol always expects a single \0-terminated string.
      std::size_t received_zero_byte(const boost::system::error_code& error,
                                     std::size_t bytes_transferred)
         {
         if(error)
            {
            return 0;
            }

         if(bytes_transferred > 0 && m_data[bytes_transferred - 1] == '\0')
            {
            return 0;
            }

         return max_msg_length - bytes_transferred;
         }

      void on_timeout(std::function<void(const std::string&)> cb)
         {
         m_on_timeout = std::move(cb);
         }

      void reset_timeout(std::string message)
         {
         m_timeout_timer.expires_after(k_timeout);
         m_timeout_timer.async_wait([=, this](const error_code &ec)
            {
            if(ec != net::error::operation_aborted)  // timer cancelled
               {
               if(m_on_timeout)
                  { m_on_timeout(message); }

               // hard-close the underlying transport to maximize the
               // probability for all participating peers to error out
               // of their pending I/O operations
               if(m_stream)
                  { m_stream->lowest_layer().close(); }

               throw Timeout_Exception("timeout occured: " + message);
               }
            });
         }

      void cancel_timeout()
         {
         m_timeout_timer.cancel();
         }

   protected:
      Botan::AutoSeeded_RNG m_rng;
      Basic_Credentials_Manager m_credentials_manager;
      Botan::TLS::Session_Manager_Noop m_session_mgr;
      Botan::TLS::Context m_ctx;
      std::unique_ptr<ssl_stream> m_stream;
      net::system_timer m_timeout_timer;
      std::function<void(const std::string&)> m_on_timeout;

      char m_data[max_msg_length];
   };

class Result_Wrapper
   {
   public:
      Result_Wrapper(std::string name) : m_result(std::move(name)) {}

      Result& result()
         {
         return m_result;
         }

      void expect_success(const std::string& msg, const error_code& ec)
         {
         error_code success;
         expect_ec(msg, success, ec);
         }

      void expect_ec(const std::string& msg, const error_code& expected, const error_code& ec)
         {
         if(ec != expected)
            {
            m_result.test_failure(msg, "Unexpected error code: " + ec.message());
            }
         else
            {
            m_result.test_success(msg);
            }
         }

      void confirm(const std::string& msg, bool condition)
         {
         m_result.confirm(msg, condition);
         }

      void test_failure(const std::string& msg)
         {
         m_result.test_failure(msg);
         }

   private:
      Result m_result;
   };

class Server : public Peer, public std::enable_shared_from_this<Server>
   {
   public:
      Server(Botan::TLS::Policy& policy, net::io_context& ioc, std::string test_name)
         : Peer(policy, ioc, server_cert(), server_key()),
           m_acceptor(ioc),
           m_result("Server (" + test_name + ")"),
           m_short_read_expected(false),
           m_move_before_accept(false)
         {
         reset_timeout("startup");
         }

      // Control messages
      // The messages below can be used by the test clients in order to configure the server's behavior during a test
      // case.
      //
      // Tell the server that the next read should result in a StreamTruncated error
      std::string expect_short_read_message = "SHORT_READ";
      // Prepare the server for the test case "Shutdown No Response"
      std::string prepare_shutdown_no_response_message = "SHUTDOWN_NOW";

      void listen()
         {
         error_code ec;
         const auto endpoint = k_endpoints.back();

         m_acceptor.open(endpoint.protocol(), ec);
         m_acceptor.set_option(net::socket_base::reuse_address(true), ec);
         m_acceptor.bind(endpoint, ec);
         m_acceptor.listen(net::socket_base::max_listen_connections, ec);

         m_result.expect_success("listen", ec);

         reset_timeout("accept");
         m_acceptor.async_accept(std::bind(&Server::start_session, shared_from_this(), _1, _2));
         }

      void expect_short_read()
         {
         m_short_read_expected = true;
         }

      void move_before_accept()
         {
         m_move_before_accept = true;
         }

      Result_Wrapper result()
         {
         return m_result;
         }

   private:
      void start_session(const error_code& ec, tcp::socket socket)
         {
         // Note: If this fails with 'Operation canceled', it likely means the timer expired and the port is taken.
         m_result.expect_success("accept", ec);

         // Note: If this was a real server, we should create a new session (with its own stream) for each accepted
         // connection. In this test we only have one connection.

         if(m_move_before_accept)
            {
            // regression test for #2635
            ssl_stream s(std::move(socket), m_ctx);
            m_stream = std::unique_ptr<ssl_stream>(new ssl_stream(std::move(s)));
            }
         else
            {
            m_stream = std::unique_ptr<ssl_stream>(new ssl_stream(std::move(socket), m_ctx));
            }

         reset_timeout("handshake");
         m_stream->async_handshake(Botan::TLS::Connection_Side::Server,
                                   std::bind(&Server::handle_handshake, shared_from_this(), _1));
         }

      void shutdown()
         {
         error_code shutdown_ec;
         m_stream->shutdown(shutdown_ec);
         m_result.expect_success("shutdown", shutdown_ec);
         handle_write(error_code{});
         }

      void handle_handshake(const error_code& ec)
         {
         m_result.expect_success("handshake", ec);
         handle_write(error_code{});
         }

      void handle_write(const error_code& ec)
         {
         m_result.expect_success("send_response", ec);
         reset_timeout("read_message");
         net::async_read(*m_stream, buffer(),
                         std::bind(&Server::received_zero_byte, shared_from_this(), _1, _2),
                         std::bind(&Server::handle_read, shared_from_this(), _1, _2));
         }

      void handle_read(const error_code& ec, size_t bytes_transferred=0)
         {
         if(m_short_read_expected)
            {
            m_result.expect_ec("received stream truncated error", Botan::TLS::StreamTruncated, ec);
            cancel_timeout();
            return;
            }

         if(ec)
            {
            if(m_stream->shutdown_received())
               {
               m_result.expect_ec("received EOF after close_notify", net::error::eof, ec);
               reset_timeout("shutdown");
               m_stream->async_shutdown(std::bind(&Server::handle_shutdown, shared_from_this(), _1));
               }
            else
               {
               m_result.test_failure("Unexpected error code: " + ec.message());
               cancel_timeout();
               }
            return;
            }

         m_result.expect_success("read_message", ec);

         if(message() == prepare_shutdown_no_response_message)
            {
            m_short_read_expected = true;
            shutdown();
            return;
            }

         if(message() == expect_short_read_message)
            {
            m_short_read_expected = true;
            }

         reset_timeout("send_response");
         net::async_write(*m_stream, buffer(bytes_transferred),
                          std::bind(&Server::handle_write, shared_from_this(), _1));
         }

      void handle_shutdown(const error_code& ec)
         {
         m_result.expect_success("shutdown", ec);
         cancel_timeout();
         }

   private:
      tcp::acceptor m_acceptor;
      Result_Wrapper m_result;
      bool m_short_read_expected;

      // regression test for #2635
      bool m_move_before_accept;
   };

class Client : public Peer
   {
      static void accept_all(
         const std::vector<Botan::X509_Certificate>&,
         const std::vector<std::optional<Botan::OCSP::Response>>&,
         const std::vector<Botan::Certificate_Store*>&, Botan::Usage_Type,
         const std::string&, const Botan::TLS::Policy&) {}

   public:
      Client(Botan::TLS::Policy& policy, net::io_context& ioc)
         : Peer(policy, ioc)
         {
         m_ctx.set_verify_callback(accept_all);
         m_stream = std::unique_ptr<ssl_stream>(new ssl_stream(ioc, m_ctx));
         }

      ssl_stream& stream()
         {
         return *m_stream;
         }

      void close_socket()
         {
         // Shutdown on TCP level before closing the socket for portable behavior. Otherwise the peer will see a
         // connection_reset error rather than EOF on Windows.
         // See the remark in
         // https://www.boost.org/doc/libs/1_68_0/doc/html/boost_asio/reference/basic_stream_socket/close/overload1.html
         m_stream->lowest_layer().shutdown(tcp::socket::shutdown_both);
         m_stream->lowest_layer().close();
         }

   };

#include <boost/asio/yield.hpp>

class TestBase
   {
   public:
      TestBase(net::io_context& ioc, Botan::TLS::Policy& client_policy, Botan::TLS::Policy& server_policy,
               const std::string& name, const std::string& config_name)
         : m_name(name + " (" + config_name + ")"),
           m_client(std::make_shared<Client>(client_policy, ioc)),
           m_server(std::make_shared<Server>(server_policy, ioc, m_name)),
           m_result(m_name)
         {
         m_client->on_timeout([=, this](const std::string& msg)
            {
            m_result.test_failure("timeout in client during: " + msg);
            });
         m_server->on_timeout([=, this](const std::string& msg)
            {
            m_result.test_failure("timeout in server during: " + msg);
            });

         m_server->listen();
         }

      virtual ~TestBase() = default;

      virtual void finishAsynchronousWork() {}

      void fail(const std::string& msg)
         {
         m_result.test_failure(msg);
         }

      void extend_results(std::vector<Result>& results)
         {
         results.push_back(m_result.result());
         results.push_back(m_server->result().result());
         }

   protected:
      //! retire client and server instances after a job well done
      void teardown()
         {
         m_client->cancel_timeout();
         }

   protected:
      std::string m_name;

      std::shared_ptr<Client> m_client;
      std::shared_ptr<Server> m_server;

      Result_Wrapper m_result;
   };

class Synchronous_Test : public TestBase
   {
   public:
      using TestBase::TestBase;

      void finishAsynchronousWork() override
         {
         m_client_thread.join();
         }

      void run(const error_code&)
         {
         m_client_thread = std::thread([this]
            {
            try
               { this->run_synchronous_client(); }
            catch(const std::exception& ex)
               { m_result.test_failure(std::string("sync client failed with: ") + ex.what()); }
            catch(const boost::exception&)
               { m_result.test_failure("sync client failed with boost exception"); }
            catch(...)
               { m_result.test_failure("sync client failed with unknown error"); }
            });
         }

      virtual void run_synchronous_client() = 0;

   private:
      std::thread m_client_thread;
   };

/* In this test case both parties perform the handshake, exchange a message, and do a full shutdown.
 *
 * The client expects the server to echo the same message it sent. The client then initiates the shutdown. The server is
 * expected to receive a close_notify and complete its shutdown with an error_code Success, the client is expected to
 * receive a close_notify and complete its shutdown with an error_code EOF.
 */
class Test_Conversation : public TestBase, public net::coroutine, public std::enable_shared_from_this<Test_Conversation>
   {
   public:
      Test_Conversation(net::io_context& ioc, std::string config_name, Botan::TLS::Policy& client_policy,
                        Botan::TLS::Policy& server_policy, std::string test_name="Test Conversation")
         : TestBase(ioc, client_policy, server_policy, test_name, config_name) {}

      void run(const error_code& ec)
         {
         static auto test_case = &Test_Conversation::run;
         const std::string message("Time is an illusion. Lunchtime doubly so.");

         reenter(*this)
            {
            m_client->reset_timeout("connect");
            yield net::async_connect(m_client->stream().lowest_layer(), k_endpoints,
                                     std::bind(test_case, shared_from_this(), _1));
            m_result.expect_success("connect", ec);

            m_client->reset_timeout("handshake");
            yield m_client->stream().async_handshake(Botan::TLS::Connection_Side::Client,
                  std::bind(test_case, shared_from_this(), _1));
            m_result.expect_success("handshake", ec);

            m_client->reset_timeout("send_message");
            yield net::async_write(m_client->stream(),
                                   net::buffer(message.c_str(), message.size() + 1), // including \0 termination
                                   std::bind(test_case, shared_from_this(), _1));
            m_result.expect_success("send_message", ec);

            m_client->reset_timeout("receive_response");
            yield net::async_read(m_client->stream(),
                                  m_client->buffer(),
                                  std::bind(&Client::received_zero_byte, m_client.get(), _1, _2),
                                  std::bind(test_case, shared_from_this(), _1));
            m_result.expect_success("receive_response", ec);
            m_result.confirm("correct message", m_client->message() == message);

            m_client->reset_timeout("shutdown");
            yield m_client->stream().async_shutdown(std::bind(test_case, shared_from_this(), _1));
            m_result.expect_success("shutdown", ec);

            m_client->reset_timeout("await close_notify");
            yield net::async_read(m_client->stream(), m_client->buffer(),
                                  std::bind(test_case, shared_from_this(), _1));
            m_result.confirm("received close_notify", m_client->stream().shutdown_received());
            m_result.expect_ec("closed with EOF", net::error::eof, ec);

            teardown();
            }
         }
   };

class Test_Conversation_Sync : public Synchronous_Test
   {
   public:
      Test_Conversation_Sync(net::io_context& ioc, std::string config_name, Botan::TLS::Policy& client_policy,
                             Botan::TLS::Policy& server_policy)
         : Synchronous_Test(ioc, client_policy, server_policy, "Test Conversation Sync", config_name) {}

      void run_synchronous_client() override
         {
         const std::string message("Time is an illusion. Lunchtime doubly so.");
         error_code ec;

         net::connect(m_client->stream().lowest_layer(), k_endpoints, ec);
         m_result.expect_success("connect", ec);

         m_client->stream().handshake(Botan::TLS::Connection_Side::Client, ec);
         m_result.expect_success("handshake", ec);

         net::write(m_client->stream(),
                    net::buffer(message.c_str(), message.size() + 1), // including \0 termination
                    ec);
         m_result.expect_success("send_message", ec);

         net::read(m_client->stream(),
                   m_client->buffer(),
                   std::bind(&Client::received_zero_byte, m_client.get(), _1, _2),
                   ec);
         m_result.expect_success("receive_response", ec);
         m_result.confirm("correct message", m_client->message() == message);

         m_client->stream().shutdown(ec);
         m_result.expect_success("shutdown", ec);

         net::read(m_client->stream(), m_client->buffer(), ec);
         m_result.confirm("received close_notify", m_client->stream().shutdown_received());
         m_result.expect_ec("closed with EOF", net::error::eof, ec);

         teardown();
         }
   };

/* In this test case the client shuts down the SSL connection, but does not wait for the server's response before
 * closing the socket. Accordingly, it will not receive the server's close_notify alert. Instead, the async_read
 * operation will be aborted. The server should be able to successfully shutdown nonetheless.
 */
class Test_Eager_Close : public TestBase, public net::coroutine, public std::enable_shared_from_this<Test_Eager_Close>
   {
   public:
      Test_Eager_Close(net::io_context& ioc, std::string config_name, Botan::TLS::Policy& client_policy,
                       Botan::TLS::Policy& server_policy)
         : TestBase(ioc, client_policy, server_policy, "Test Eager Close", config_name) {}

      void run(const error_code& ec)
         {
         static auto test_case = &Test_Eager_Close::run;
         reenter(*this)
            {
            m_client->reset_timeout("connect");
            yield net::async_connect(m_client->stream().lowest_layer(), k_endpoints,
                                     std::bind(test_case, shared_from_this(), _1));
            m_result.expect_success("connect", ec);

            m_client->reset_timeout("handshake");
            yield m_client->stream().async_handshake(Botan::TLS::Connection_Side::Client,
                  std::bind(test_case, shared_from_this(), _1));
            m_result.expect_success("handshake", ec);

            m_client->reset_timeout("shutdown");
            yield m_client->stream().async_shutdown(std::bind(test_case, shared_from_this(), _1));
            m_result.expect_success("shutdown", ec);

            m_client->close_socket();
            m_result.confirm("did not receive close_notify", !m_client->stream().shutdown_received());

            teardown();
            }
         }
   };

class Test_Eager_Close_Sync : public Synchronous_Test
   {
   public:
      Test_Eager_Close_Sync(net::io_context& ioc, std::string config_name, Botan::TLS::Policy& client_policy,
                            Botan::TLS::Policy& server_policy)
         : Synchronous_Test(ioc, client_policy, server_policy, "Test Eager Close Sync", config_name) {}

      void run_synchronous_client() override
         {
         error_code ec;

         net::connect(m_client->stream().lowest_layer(), k_endpoints, ec);
         m_result.expect_success("connect", ec);

         m_client->stream().handshake(Botan::TLS::Connection_Side::Client, ec);
         m_result.expect_success("handshake", ec);

         m_client->stream().shutdown(ec);
         m_result.expect_success("shutdown", ec);

         m_client->close_socket();
         m_result.confirm("did not receive close_notify", !m_client->stream().shutdown_received());

         teardown();
         }
   };

/* In this test case the client closes the socket without properly shutting down the connection.
 * The server should see a StreamTruncated error.
 */
class Test_Close_Without_Shutdown
   : public TestBase,
     public net::coroutine,
     public std::enable_shared_from_this<Test_Close_Without_Shutdown>
   {
   public:
      Test_Close_Without_Shutdown(net::io_context& ioc, std::string config_name, Botan::TLS::Policy& client_policy,
                                  Botan::TLS::Policy& server_policy)
         : TestBase(ioc, client_policy, server_policy, "Test Close Without Shutdown", config_name) {}

      void run(const error_code& ec)
         {
         static auto test_case = &Test_Close_Without_Shutdown::run;
         reenter(*this)
            {
            m_client->reset_timeout("connect");
            yield net::async_connect(m_client->stream().lowest_layer(), k_endpoints,
                                     std::bind(test_case, shared_from_this(), _1));
            m_result.expect_success("connect", ec);

            m_client->reset_timeout("handshake");
            yield m_client->stream().async_handshake(Botan::TLS::Connection_Side::Client,
                  std::bind(test_case, shared_from_this(), _1));
            m_result.expect_success("handshake", ec);

            // send the control message to configure the server to expect a short-read
            m_client->reset_timeout("send expect_short_read message");
            yield net::async_write(m_client->stream(),
                                   net::buffer(m_server->expect_short_read_message.c_str(),
                                               m_server->expect_short_read_message.size() + 1), // including \0 termination
                                   std::bind(test_case, shared_from_this(), _1));
            m_result.expect_success("send expect_short_read message", ec);

            // read the confirmation of the control message above
            m_client->reset_timeout("receive_response");
            yield net::async_read(m_client->stream(),
                                  m_client->buffer(),
                                  std::bind(&Client::received_zero_byte, m_client.get(), _1, _2),
                                  std::bind(test_case, shared_from_this(), _1));
            m_result.expect_success("receive_response", ec);

            m_client->close_socket();
            m_result.confirm("did not receive close_notify", !m_client->stream().shutdown_received());

            teardown();
            }
         }
   };

class Test_Close_Without_Shutdown_Sync : public Synchronous_Test
   {
   public:
      Test_Close_Without_Shutdown_Sync(net::io_context& ioc, std::string config_name, Botan::TLS::Policy& client_policy,
                                       Botan::TLS::Policy& server_policy)
         : Synchronous_Test(ioc, client_policy, server_policy, "Test Close Without Shutdown Sync", config_name) {}

      void run_synchronous_client() override
         {
         error_code ec;
         net::connect(m_client->stream().lowest_layer(), k_endpoints, ec);
         m_result.expect_success("connect", ec);

         m_client->stream().handshake(Botan::TLS::Connection_Side::Client, ec);
         m_result.expect_success("handshake", ec);

         m_server->expect_short_read();

         m_client->close_socket();
         m_result.confirm("did not receive close_notify", !m_client->stream().shutdown_received());

         teardown();
         }
   };

/* In this test case the server shuts down the connection but the client doesn't send the corresponding close_notify
 * response. Instead, it closes the socket immediately.
 * The server should see a short-read error.
 */
class Test_No_Shutdown_Response : public TestBase, public net::coroutine,
   public std::enable_shared_from_this<Test_No_Shutdown_Response>
   {
   public:
      Test_No_Shutdown_Response(net::io_context& ioc, std::string config_name, Botan::TLS::Policy& client_policy,
                                Botan::TLS::Policy& server_policy)
         : TestBase(ioc, client_policy, server_policy, "Test No Shutdown Response", config_name) {}

      void run(const error_code& ec)
         {
         static auto test_case = &Test_No_Shutdown_Response::run;
         reenter(*this)
            {
            m_client->reset_timeout("connect");
            yield net::async_connect(m_client->stream().lowest_layer(), k_endpoints,
                                     std::bind(test_case, shared_from_this(), _1));
            m_result.expect_success("connect", ec);

            m_client->reset_timeout("handshake");
            yield m_client->stream().async_handshake(Botan::TLS::Connection_Side::Client,
                  std::bind(test_case, shared_from_this(), _1));
            m_result.expect_success("handshake", ec);

            // send a control message to make the server shut down
            m_client->reset_timeout("send shutdown message");
            yield net::async_write(m_client->stream(),
                                   net::buffer(m_server->prepare_shutdown_no_response_message.c_str(),
                                               m_server->prepare_shutdown_no_response_message.size() + 1), // including \0 termination
                                   std::bind(test_case, shared_from_this(), _1));
            m_result.expect_success("send shutdown message", ec);

            // read the server's close-notify message
            m_client->reset_timeout("read close_notify");
            yield net::async_read(m_client->stream(), m_client->buffer(),
                                  std::bind(test_case, shared_from_this(), _1));
            m_result.expect_ec("read gives EOF", net::error::eof, ec);
            m_result.confirm("received close_notify", m_client->stream().shutdown_received());

            // close the socket rather than shutting down
            m_client->close_socket();

            teardown();
            }
         }
   };

class Test_No_Shutdown_Response_Sync : public Synchronous_Test
   {
   public:
      Test_No_Shutdown_Response_Sync(net::io_context& ioc, std::string config_name, Botan::TLS::Policy& client_policy,
                                     Botan::TLS::Policy& server_policy)
         : Synchronous_Test(ioc, client_policy, server_policy, "Test No Shutdown Response Sync", config_name) {}

      void run_synchronous_client() override
         {
         error_code ec;
         net::connect(m_client->stream().lowest_layer(), k_endpoints, ec);
         m_result.expect_success("connect", ec);

         m_client->stream().handshake(Botan::TLS::Connection_Side::Client, ec);
         m_result.expect_success("handshake", ec);

         net::write(m_client->stream(),
                    net::buffer(m_server->prepare_shutdown_no_response_message.c_str(),
                                m_server->prepare_shutdown_no_response_message.size() + 1), // including \0 termination
                    ec);
         m_result.expect_success("send expect_short_read message", ec);

         net::read(m_client->stream(), m_client->buffer(), ec);
         m_result.expect_ec("read gives EOF", net::error::eof, ec);
         m_result.confirm("received close_notify", m_client->stream().shutdown_received());

         // close the socket rather than shutting down
         m_client->close_socket();

         teardown();
         }
   };

class Test_Conversation_With_Move : public Test_Conversation
   {
   public:
      Test_Conversation_With_Move(net::io_context& ioc, std::string config_name, Botan::TLS::Policy& client_policy,
                                  Botan::TLS::Policy& server_policy)
         : Test_Conversation(ioc, std::move(config_name), client_policy, server_policy, "Test Conversation With Move")
         {
         m_server->move_before_accept();
         }
   };

#include <boost/asio/unyield.hpp>

struct SystemConfiguration
   {
   std::string name;

   Botan::TLS::Text_Policy client_policy;
   Botan::TLS::Text_Policy server_policy;

   SystemConfiguration(std::string n, Botan::TLS::Text_Policy cp, Botan::TLS::Text_Policy sp)
      : name(std::move(n))
      , client_policy(std::move(cp))
      , server_policy(std::move(sp))
      {}

   template<typename TestT>
   void run(std::vector<Result>& results)
      {
      net::io_context ioc;

      auto t = std::make_shared<TestT>(ioc, name, server_policy, client_policy);

      t->run(error_code{});

      while(true)
         {
         try
            {
            ioc.run();
            break;
            }
         catch(const Timeout_Exception&)
            { /* timeout is reported via Test::Result object */ }
         catch(const boost::exception&)
            { t->fail("boost exception"); }
         catch(const std::exception& ex)
            { t->fail(ex.what()); }
         }

      t->finishAsynchronousWork();
      t->extend_results(results);
      }
   };

std::vector<SystemConfiguration> get_configurations()
   {
   return
      {
      SystemConfiguration(
         "TLS 1.2 only",
         Botan::TLS::Text_Policy("allow_tls12=true\nallow_tls13=false"),
         Botan::TLS::Text_Policy("allow_tls12=true\nallow_tls13=false")),
#if defined(BOTAN_HAS_TLS_13)
      SystemConfiguration(
         "TLS 1.3 only",
         Botan::TLS::Text_Policy("allow_tls12=false\nallow_tls13=true"),
         Botan::TLS::Text_Policy("allow_tls12=false\nallow_tls13=true")),
      SystemConfiguration(
         "TLS 1.x server, TLS 1.2 client",
         Botan::TLS::Text_Policy("allow_tls12=true\nallow_tls13=false"),
         Botan::TLS::Text_Policy("allow_tls12=true\nallow_tls13=true")),
      SystemConfiguration(
         "TLS 1.2 server, TLS 1.x client",
         Botan::TLS::Text_Policy("allow_tls12=true\nallow_tls13=true"),
         Botan::TLS::Text_Policy("allow_tls12=true\nallow_tls13=false")),
#endif
      };
   }

}  // namespace

namespace Botan_Tests {

class Tls_Stream_Integration_Tests final : public Test
   {
   public:
      std::vector<Test::Result> run() override
         {
         std::vector<Test::Result> results;

         auto configs = get_configurations();
         for(auto& config : configs)
            {
            config.run<Test_Conversation>(results);
            config.run<Test_Eager_Close>(results);
            config.run<Test_Close_Without_Shutdown>(results);
            config.run<Test_No_Shutdown_Response>(results);
            config.run<Test_Conversation_Sync>(results);
            config.run<Test_Eager_Close_Sync>(results);
            config.run<Test_Close_Without_Shutdown_Sync>(results);
            config.run<Test_No_Shutdown_Response_Sync>(results);
            config.run<Test_Conversation_With_Move>(results);
            }

         return results;
         }
   };

BOTAN_REGISTER_TEST("tls", "tls_stream_integration", Tls_Stream_Integration_Tests);

}  // namespace Botan_Tests

#endif // BOOST_VERSION
#endif // BOTAN_HAS_TLS && BOTAN_HAS_BOOST_ASIO
