/*
* TLS Server - implementation for TLS 1.3
* (C) 2022 Jack Lloyd
*     2022 René Meusel - Rohde & Schwarz Cybersecurity
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_SERVER_IMPL_13_H_
#define BOTAN_TLS_SERVER_IMPL_13_H_

#include <botan/internal/tls_channel_impl_13.h>
#include <botan/internal/tls_handshake_state_13.h>
#include <botan/internal/tls_handshake_transitions.h>

namespace Botan::TLS {

/**
* SSL/TLS Server 1.3 implementation
*/
class Server_Impl_13 : public Channel_Impl_13
   {
   public:
      explicit Server_Impl_13(Callbacks& callbacks,
                              Session_Manager& session_manager,
                              Credentials_Manager& credentials_manager,
                              const Policy& policy,
                              RandomNumberGenerator& rng);

      std::string application_protocol() const override;
      std::vector<X509_Certificate> peer_cert_chain() const override;

   private:
      void process_handshake_msg(Handshake_Message_13 msg) override;
      void process_post_handshake_msg(Post_Handshake_Message_13 msg) override;
      void process_dummy_change_cipher_spec() override;

      using Channel_Impl_13::handle;
      void handle(const Client_Hello_12& client_hello_msg);
      void handle(const Client_Hello_13& client_hello_msg);
      void handle(const Certificate_13& certificate_msg);
      void handle(const Certificate_Verify_13& certificate_verify_msg);
      void handle(const Finished_13& finished_msg);

      void handle_reply_to_client_hello(const Server_Hello_13& server_hello);
      void handle_reply_to_client_hello(const Hello_Retry_Request& hello_retry_request);

      bool handshake_finished() const override;

      void downgrade();

   private:
      Server_Handshake_State_13 m_handshake_state;
      Handshake_Transitions m_transitions;
   };

}

#endif
