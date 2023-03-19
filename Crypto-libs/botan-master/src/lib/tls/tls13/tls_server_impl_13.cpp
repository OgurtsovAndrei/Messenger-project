/*
* TLS Server - implementation for TLS 1.3
* (C) 2022 Jack Lloyd
*     2022 René Meusel - Rohde & Schwarz Cybersecurity
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <botan/internal/tls_server_impl_13.h>
#include <botan/internal/tls_cipher_state.h>
#include <botan/internal/loadstor.h>
#include <botan/internal/stl_util.h>
#include <botan/credentials_manager.h>
#include <botan/rng.h>

namespace Botan::TLS {

Server_Impl_13::Server_Impl_13(Callbacks& callbacks,
                               Session_Manager& session_manager,
                               Credentials_Manager& credentials_manager,
                               const Policy& policy,
                               RandomNumberGenerator& rng)
   : Channel_Impl_13(callbacks, session_manager, credentials_manager,
                     rng, policy, true /* is_server */)
   {
#if defined(BOTAN_HAS_TLS_12)
   if(policy.allow_tls12())
      { expect_downgrade({}, {}); }
#endif

   m_transitions.set_expected_next(Handshake_Type::ClientHello);
   }

std::string Server_Impl_13::application_protocol() const
   {
   if(m_handshake_state.handshake_finished())
      {
      const auto& eee = m_handshake_state.encrypted_extensions().extensions();
      if(const auto alpn = eee.get<Application_Layer_Protocol_Notification>())
         {
         return alpn->single_protocol();
         }
      }

   return "";
   }

std::vector<X509_Certificate> Server_Impl_13::peer_cert_chain() const
   {
   return (m_handshake_state.has_client_certificate_chain())
      ? m_handshake_state.client_certificate().cert_chain()
      // TODO: after implementing session resumption, this may return the
      //       client's certificate chain stored in the previous session.
      : std::vector<X509_Certificate>{};
   }

void Server_Impl_13::process_handshake_msg(Handshake_Message_13 message)
   {
   std::visit([&](auto msg)
      {
      // first verify that the message was expected by the state machine...
      m_transitions.confirm_transition_to(msg.get().type());

      // ... then allow the library user to abort on their discretion
      callbacks().tls_inspect_handshake_msg(msg.get());

      // ... finally handle the message
      handle(msg.get());
      }, m_handshake_state.received(std::move(message)));
   }

void Server_Impl_13::process_post_handshake_msg(Post_Handshake_Message_13 message)
   {
   BOTAN_STATE_CHECK(handshake_finished());

   std::visit([&](auto msg)
      {
      handle(msg);
      }, m_handshake_state.received(std::move(message)));
   }

void Server_Impl_13::process_dummy_change_cipher_spec()
   {
   // RFC 8446 5.
   //    If an implementation detects a change_cipher_spec record received before
   //    the first ClientHello message or after the peer's Finished message, it MUST be
   //    treated as an unexpected record type [("unexpected_message" alert)].
   if(!m_handshake_state.has_client_hello() || m_handshake_state.has_client_finished())
      {
      throw TLS_Exception(Alert::UnexpectedMessage, "Received an unexpected dummy Change Cipher Spec");
      }

   // RFC 8446 5.
   //    An implementation may receive an unencrypted record of type change_cipher_spec [...]
   //    at any time after the first ClientHello message has been sent or received
   //    and before the peer's Finished message has been received [...]
   //    and MUST simply drop it without further processing.
   //
   // ... no further processing.
   }

bool Server_Impl_13::handshake_finished() const
   {
   return m_handshake_state.handshake_finished();
   }


void Server_Impl_13::downgrade()
   {
   BOTAN_ASSERT_NOMSG(expects_downgrade());

   request_downgrade();

   // After this, no further messages are expected here because this instance
   // will be replaced by a Server_Impl_12.
   m_transitions.set_expected_next({});
   }

void Server_Impl_13::handle_reply_to_client_hello(const Server_Hello_13& server_hello)
   {
   const auto& client_hello = m_handshake_state.client_hello();
   const auto& exts = client_hello.extensions();

   const auto cipher = Ciphersuite::by_id(server_hello.ciphersuite());
   m_transcript_hash.set_algorithm(cipher->prf_algo());

   const auto my_keyshare = server_hello.extensions().get<Key_Share>();
   auto shared_secret = my_keyshare->exchange(*exts.get<Key_Share>(), policy(), callbacks(), rng());
   my_keyshare->erase();

   m_cipher_state = Cipher_State::init_with_server_hello(m_side, std::move(shared_secret), cipher.value(),
                    m_transcript_hash.current());

   auto flight = aggregate_handshake_messages();
   flight
      .add(m_handshake_state.sending(Encrypted_Extensions(client_hello, policy(), callbacks())));

   // RFC 8446 4.3.2
   //    A server which is authenticating with a certificate MAY optionally
   //    request a certificate from the client. This message, if sent, MUST
   //    follow EncryptedExtensions.
   //
   // Note: When implementing PSK, this message must not be sent
   if(auto certificate_request = Certificate_Request_13::maybe_create(client_hello,
                                                                      credentials_manager(),
                                                                      callbacks(),
                                                                      policy()))
      {
      flight.add(m_handshake_state.sending(std::move(certificate_request.value())));

      // RFC 8446 4.4.2
      //    The client MUST send a Certificate message if and only if the server
      //    has requested client authentication via a CertificateRequest message
      //    [...]. If the server requests client authentication but no
      //    suitable certificate is available, the client MUST send a Certificate
      //    message containing no certificates [...].
      m_transitions.set_expected_next(Handshake_Type::Certificate);
      }
   else
      {
      m_transitions.set_expected_next(Handshake_Type::Finished);
      }

   flight
      .add(m_handshake_state.sending(Certificate_13(client_hello, credentials_manager(), callbacks())))
      .add(m_handshake_state.sending(Certificate_Verify_13(
                                        m_handshake_state.server_certificate(),
                                        client_hello.signature_schemes(),
                                        client_hello.sni_hostname(),
                                        m_transcript_hash.current(),
                                        Connection_Side::Server,
                                        credentials_manager(),
                                        policy(),
                                        callbacks(),
                                        rng())))
      .add(m_handshake_state.sending(Finished_13(m_cipher_state.get(), m_transcript_hash.current())));

   if(client_hello.extensions().has<Record_Size_Limit>() &&
         m_handshake_state.encrypted_extensions().extensions().has<Record_Size_Limit>())
      {
      // RFC 8449 4.
      //    When the "record_size_limit" extension is negotiated, an endpoint
      //    MUST NOT generate a protected record with plaintext that is larger
      //    than the RecordSizeLimit value it receives from its peer.
      //    Unprotected messages are not subject to this limit.
      //
      // Hence, the limit is set just before we start sending encrypted records.
      //
      // RFC 8449 4.
      //     The record size limit only applies to records sent toward the
      //     endpoint that advertises the limit.  An endpoint can send records
      //     that are larger than the limit it advertises as its own limit.
      //
      // Hence, the "outgoing" limit is what the client requested and the
      // "incoming" limit is what we will request in the Encrypted Extensions.
      const auto outgoing_limit = client_hello.extensions().get<Record_Size_Limit>();
      const auto incoming_limit = m_handshake_state.encrypted_extensions().extensions().get<Record_Size_Limit>();
      set_record_size_limits(outgoing_limit->limit(), incoming_limit->limit());
      }

   flight.send();

   m_cipher_state->advance_with_server_finished(m_transcript_hash.current());
   }

void Server_Impl_13::handle_reply_to_client_hello(const Hello_Retry_Request& hello_retry_request)
   {
   auto cipher = Ciphersuite::by_id(hello_retry_request.ciphersuite());
   BOTAN_ASSERT_NOMSG(cipher.has_value());  // should work, since we chose that suite

   m_transcript_hash = Transcript_Hash_State::recreate_after_hello_retry_request(cipher->prf_algo(), m_transcript_hash);

   m_transitions.set_expected_next(Handshake_Type::ClientHello);
   }

void Server_Impl_13::handle(const Client_Hello_12& ch)
   {
   // The detailed handling of the TLS 1.2 compliant Client Hello is left to
   // the TLS 1.2 server implementation.
   BOTAN_UNUSED(ch);

   // After we sent a Hello Retry Request we must not accept a downgrade.
   if(m_handshake_state.has_hello_retry_request())
      {
      throw TLS_Exception(Alert::UnexpectedMessage,
                          "Received a TLS 1.2 Client Hello after Hello Retry Request");
      }

   // RFC 8446 Appendix D.2
   //    If the "supported_versions" extension is absent and the server only
   //    supports versions greater than ClientHello.legacy_version, the server
   //    MUST abort the handshake with a "protocol_version" alert.
   //
   // If we're not expecting a downgrade, we only support TLS 1.3.
   if(!expects_downgrade())
      {
      throw TLS_Exception(Alert::ProtocolVersion, "Received a legacy Client Hello");
      }

   downgrade();
   }

void Server_Impl_13::handle(const Client_Hello_13& client_hello)
   {
   const auto& exts = client_hello.extensions();

   const bool is_initial_client_hello = !m_handshake_state.has_hello_retry_request();

   if(is_initial_client_hello)
      {
      const auto preferred_version = client_hello.highest_supported_version(policy());
      if(!preferred_version)
         {
         throw TLS_Exception(Alert::ProtocolVersion, "No shared TLS version");
         }

      // RFC 8446 4.2.2
      //   Clients MUST NOT use cookies in their initial ClientHello in subsequent
      //   connections.
      if(exts.has<Cookie>())
         {
         throw TLS_Exception(Alert::IllegalParameter,
                           "Received a Cookie in the initial client hello");
         }
      }

   // TODO: Implement support for PSK. For now, we ignore any such extensions
   //       and always revert to a standard key exchange.
   if(!exts.has<Supported_Groups>())
      {
      throw Not_Implemented("PSK-only handshake NYI");
      }

   // RFC 8446 9.2
   //    If containing a "supported_groups" extension, [Client Hello] MUST
   //    also contain a "key_share" extension, and vice versa.
   //
   // This was validated before in the Client_Hello_13 constructor.
   BOTAN_ASSERT_NOMSG(exts.has<Key_Share>());

   if(!is_initial_client_hello)
      {
      const auto& hrr_exts = m_handshake_state.hello_retry_request().extensions();
      const auto offered_groups = exts.get<Key_Share>()->offered_groups();
      const auto selected_group = hrr_exts.get<Key_Share>()->selected_group();
      if(offered_groups.size() != 1 || offered_groups.at(0) != selected_group)
         {
         throw TLS_Exception(Alert::IllegalParameter,
                             "Client did not comply with the requested key exchange group");
         }
      }

   callbacks().tls_examine_extensions(exts, Connection_Side::Client, client_hello.type());
   const auto sh_or_hrr = m_handshake_state.sending(Server_Hello_13::create(
      client_hello, is_initial_client_hello, rng(), policy(), callbacks()));
   send_handshake_message(sh_or_hrr);

   // RFC 8446 Appendix D.4  (Middlebox Compatibility Mode)
   //    The server sends a dummy change_cipher_spec record immediately after
   //    its first handshake message. This may either be after a ServerHello or
   //    a HelloRetryRequest.
   //
   //    This "compatibility mode" is partially negotiated: the client can opt
   //    to provide a session ID or not, and the server has to echo it. Either
   //    side can send change_cipher_spec at any time during the handshake, as
   //    they must be ignored by the peer, but if the client sends a non-empty
   //    session ID, the server MUST send the change_cipher_spec as described
   //    [above].
   //
   // Technically, the usage of compatibility mode is fully up to the client
   // sending a non-empty session ID. Nevertheless, when the policy requests
   // it we send a CCS regardless. Note that this is perfectly legal and also
   // satisfies some BoGo tests that expect this behaviour.
   if(is_initial_client_hello &&
         (policy().tls_13_middlebox_compatibility_mode() ||
          !client_hello.session_id().empty()))
      {
      send_dummy_change_cipher_spec();
      }

   std::visit([this](auto msg) { handle_reply_to_client_hello(msg); }, sh_or_hrr);
   }

void Server_Impl_13::handle(const Certificate_13& certificate_msg)
   {
   // RFC 8446 4.3.2
   //    certificate_request_context:  [...] This field SHALL be zero length
   //    unless used for the post-handshake authentication exchanges [...].
   if(!handshake_finished() && !certificate_msg.request_context().empty())
      {
      throw TLS_Exception(Alert::DecodeError, "Received a client certificate message with non-empty request context");
      }

   // RFC 8446 4.4.2
   //    Extensions in the Certificate message from the client MUST correspond
   //    to extensions in the CertificateRequest message from the server.
   certificate_msg.validate_extensions(m_handshake_state.certificate_request().extensions().extension_types(), callbacks());

   // RFC 8446 4.4.2.4
   //   If the client does not send any certificates (i.e., it sends an empty
   //   Certificate message), the server MAY at its discretion either continue
   //   the handshake without client authentication or abort the handshake with
   //   a "certificate_required" alert.
   if(certificate_msg.empty())
      {
      if(policy().require_client_certificate_authentication())
         {
         throw TLS_Exception(Alert::CertificateRequired, "Policy requires client send a certificate, but it did not");
         }

      // RFC 8446 4.4.2
      //    A Finished message MUST be sent regardless of whether the
      //    Certificate message is empty.
      m_transitions.set_expected_next(Handshake_Type::Finished);
      }
   else
      {
      // RFC 8446 4.4.2.4
      //    [...], if some aspect of the certificate chain was unacceptable
      //    (e.g., it was not signed by a known, trusted CA), the server MAY at
      //    its discretion either continue the handshake (considering the client
      //    unauthenticated) or abort the handshake.
      //
      // TODO: We could make this dependent on Policy::require_client_auth().
      //       Though, apps may also override Callbacks::tls_verify_cert_chain()
      //       and 'ignore' validation issues to a certain extent.
      certificate_msg.verify(callbacks(),
                             policy(),
                             credentials_manager(),
                             m_handshake_state.client_hello().sni_hostname(),
                             m_handshake_state.client_hello().extensions().has<Certificate_Status_Request>());

      // RFC 8446 4.4.3
      //    Clients MUST send this message whenever authenticating via a
      //    certificate (i.e., when the Certificate message
      //    is non-empty). When sent, this message MUST appear immediately after
      //    the Certificate message [...].
      m_transitions.set_expected_next(Handshake_Type::CertificateVerify);
      }
   }

void Server_Impl_13::handle(const Certificate_Verify_13& certificate_verify_msg)
   {
   // RFC 8446 4.4.3
   //    If sent by a client, the signature algorithm used in the signature
   //    MUST be one of those present in the supported_signature_algorithms
   //    field of the "signature_algorithms" extension in the
   //    CertificateRequest message.
   const auto offered = m_handshake_state.certificate_request().signature_schemes();
   if(!value_exists(offered, certificate_verify_msg.signature_scheme()))
      {
      throw TLS_Exception(Alert::IllegalParameter,
                          "We did not offer the usage of " +
                          certificate_verify_msg.signature_scheme().to_string() +
                          " as a signature scheme");
      }

   BOTAN_ASSERT_NOMSG(m_handshake_state.has_client_certificate_chain() &&
                      !m_handshake_state.client_certificate().empty());
   bool sig_valid = certificate_verify_msg.verify(
                       m_handshake_state.client_certificate().leaf(),
                       callbacks(),
                       m_transcript_hash.previous());

   // RFC 8446 4.4.3
   //   If the verification fails, the receiver MUST terminate the handshake
   //   with a "decrypt_error" alert.
   if(!sig_valid)
      { throw TLS_Exception(Alert::DecryptError, "Client certificate verification failed"); }

   m_transitions.set_expected_next(Handshake_Type::Finished);
   }

void Server_Impl_13::handle(const Finished_13& finished_msg)
   {
   // RFC 8446 4.4.4
   //    Recipients of Finished messages MUST verify that the contents are
   //    correct and if incorrect MUST terminate the connection with a
   //    "decrypt_error" alert.
   if(!finished_msg.verify(m_cipher_state.get(),
                           m_transcript_hash.previous()))
      { throw TLS_Exception(Alert::DecryptError, "Finished message didn't verify"); }

   m_cipher_state->advance_with_client_finished(m_transcript_hash.current());

   // no more handshake messages expected
   m_transitions.set_expected_next({});

   callbacks().tls_session_activated();
   }

}  // namespace Botan::TLS
