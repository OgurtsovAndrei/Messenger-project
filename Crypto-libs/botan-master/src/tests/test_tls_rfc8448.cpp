/*
* (C) 2021 Jack Lloyd
*     2021, 2022 René Meusel, Hannes Rantzsch - neXenio GmbH
*     2022       René Meusel - Rohde & Schwarz Cybersecurity GmbH
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include "tests.h"
#include <memory>
#include <utility>
// Since RFC 8448 uses a specific set of cipher suites we can only run this
// test if all of them are enabled.
#if defined(BOTAN_HAS_TLS_13) && \
   defined(BOTAN_HAS_AEAD_CHACHA20_POLY1305) && \
   defined(BOTAN_HAS_AEAD_GCM) && \
   defined(BOTAN_HAS_AES) && \
   defined(BOTAN_HAS_CURVE_25519) && \
   defined(BOTAN_HAS_SHA2_32) && \
   defined(BOTAN_HAS_SHA2_64) && \
   defined(BOTAN_HAS_ECDSA)
   #define BOTAN_CAN_RUN_TEST_TLS_RFC8448
#endif

#if defined(BOTAN_CAN_RUN_TEST_TLS_RFC8448)
   #include "test_rng.h"
   #include "test_tls_utils.h"

   #include <botan/credentials_manager.h>
   #include <botan/rsa.h>
   #include <botan/hash.h>
   #include <botan/ecdsa.h>
   #include <botan/tls_alert.h>
   #include <botan/tls_callbacks.h>
   #include <botan/tls_client.h>
   #include <botan/tls_policy.h>
   #include <botan/tls_messages.h>
   #include <botan/internal/tls_reader.h>
   #include <botan/tls_server.h>
   #include <botan/tls_server_info.h>
   #include <botan/tls_session.h>
   #include <botan/tls_session_manager.h>
   #include <botan/tls_version.h>

   #include <botan/assert.h>
   #include <botan/internal/stl_util.h>

   #include <botan/internal/loadstor.h>

   #include <botan/pk_algs.h>
   #include <botan/pkcs8.h>
   #include <botan/data_src.h>
#endif

namespace Botan_Tests {

#if defined(BOTAN_CAN_RUN_TEST_TLS_RFC8448)

namespace {

void add_entropy(Botan_Tests::Fixed_Output_RNG& rng, const std::vector<uint8_t>& bin)
   {
   rng.add_entropy(bin.data(), bin.size());
   }

Botan::X509_Certificate server_certificate()
   {
   // self-signed certificate with an RSA1024 public key valid until:
   //   Jul 30 01:23:59 2026 GMT
   Botan::DataSource_Memory in(Test::read_data_file("tls_13_rfc8448/server_certificate.pem"));
   return Botan::X509_Certificate(in);
   }

Botan::X509_Certificate alternative_server_certificate()
   {
   // self-signed certificate with a P-256 public key valid until:
   //   Jul 30 01:24:00 2026 GMT
   //
   // This certificate is presented by the server in the "Client Authentication"
   // test case. Why the certificate differs in that case remains unclear.
   Botan::DataSource_Memory in(Test::read_data_file("tls_13_rfc8448/server_certificate_client_auth.pem"));
   return Botan::X509_Certificate(in);
   }

Botan::X509_Certificate client_certificate()
   {
   // self-signed certificate with an RSA1024 public key valid until:
   //   Jul 30 01:23:59 2026 GMT
   Botan::DataSource_Memory in(Test::read_data_file("tls_13_rfc8448/client_certificate.pem"));
   return Botan::X509_Certificate(in);
   }

/**
* Simple version of the Padding extension (RFC 7685) to reproduce the
* 2nd Client_Hello in RFC8448 Section 5 (HelloRetryRequest)
*/
class Padding final : public Botan::TLS::Extension
   {
   public:
      static Botan::TLS::Extension_Code static_type()
         { return Botan::TLS::Extension_Code(21); }

      Botan::TLS::Extension_Code type() const override { return static_type(); }

      explicit Padding(const size_t padding_bytes) :
         m_padding_bytes(padding_bytes) {}

      std::vector<uint8_t> serialize(Botan::TLS::Connection_Side) const override
         {
         return std::vector<uint8_t>(m_padding_bytes, 0x00);
         }

      bool empty() const override { return m_padding_bytes == 0; }
   private:
      size_t m_padding_bytes;
   };

using namespace Botan;
using namespace Botan::TLS;

std::chrono::system_clock::time_point from_milliseconds_since_epoch(uint64_t msecs)
   {
   const int64_t  secs_since_epoch  = msecs / 1000;
   const uint32_t additional_millis = msecs % 1000;

   BOTAN_ASSERT_NOMSG(secs_since_epoch <= std::numeric_limits<time_t>::max());
   return std::chrono::system_clock::from_time_t(static_cast<time_t>(secs_since_epoch)) +
          std::chrono::milliseconds(additional_millis);
   }

using Modify_Exts_Fn = std::function<void(Botan::TLS::Extensions&, Botan::TLS::Connection_Side, Botan::TLS::Handshake_Type)>;

/**
 * We cannot actually reproduce the signatures stated in RFC 8448 as their
 * signature scheme is probabilistic and we're lacking the correct RNG
 * input. Hence, signatures are know beforehand and just reproduced by the
 * TLS callback when requested.
 */
struct MockSignature
   {
   std::vector<uint8_t> message_to_sign;
   std::vector<uint8_t> signature_to_produce;
   };

/**
 * Subclass of the Botan::TLS::Callbacks instrumenting all available callbacks.
 * The invocation counts can be checked in the integration tests to make sure
 * all expected callbacks are hit. Furthermore collects the received application
 * data and sent record bytes for further inspection by the test cases.
 */
class Test_TLS_13_Callbacks : public Botan::TLS::Callbacks
   {
   public:
      Test_TLS_13_Callbacks(
         Modify_Exts_Fn modify_exts_cb,
         std::vector<MockSignature> mock_signatures,
         uint64_t timestamp) :
         session_activated_called(false),
         m_modify_exts(std::move(modify_exts_cb)),
         m_mock_signatures(std::move(mock_signatures)),
         m_timestamp(from_milliseconds_since_epoch(timestamp))
         {}

      void tls_emit_data(const uint8_t data[], size_t size) override
         {
         count_callback_invocation("tls_emit_data");
         send_buffer.insert(send_buffer.end(), data, data + size);
         }

      void tls_record_received(uint64_t seq_no, const uint8_t data[], size_t size) override
         {
         count_callback_invocation("tls_record_received");
         received_seq_no = seq_no;
         receive_buffer.insert(receive_buffer.end(), data, data + size);
         }

      void tls_alert(Botan::TLS::Alert alert) override
         {
         count_callback_invocation("tls_alert");
         BOTAN_UNUSED(alert);
         // handle a tls alert received from the tls server
         }

      bool tls_peer_closed_connection() override
         {
         count_callback_invocation("tls_peer_closed_connection");
         // we want to handle the closure ourselves
         return false;
         }

      bool tls_session_established(const Botan::TLS::Session_with_Handle&) override
         {
         count_callback_invocation("tls_session_established");
         // the session with the tls client was established
         // return false to prevent the session from being cached, true to
         // cache the session in the configured session manager
         return false;
         }

      void tls_session_activated() override
         {
         count_callback_invocation("tls_session_activated");
         session_activated_called = true;
         }

      bool tls_session_ticket_received(const Session&) override
         {
         count_callback_invocation("tls_session_ticket_received");
         return true; // should always store the session
         }

      void tls_verify_cert_chain(
         const std::vector<Botan::X509_Certificate>& cert_chain,
         const std::vector<std::optional<Botan::OCSP::Response>>&,
         const std::vector<Botan::Certificate_Store*>&,
         Botan::Usage_Type,
         const std::string&,
         const Botan::TLS::Policy&) override
         {
         count_callback_invocation("tls_verify_cert_chain");
         certificate_chain = cert_chain;
         }

      std::chrono::milliseconds tls_verify_cert_chain_ocsp_timeout() const override
         {
         count_callback_invocation("tls_verify_cert_chain");
         return std::chrono::milliseconds(0);
         }

      std::vector<uint8_t> tls_provide_cert_status(const std::vector<X509_Certificate>& chain,
            const Certificate_Status_Request& csr) override
         {
         count_callback_invocation("tls_provide_cert_status");
         return Callbacks::tls_provide_cert_status(chain, csr);
         }

      std::vector<uint8_t> tls_sign_message(
         const Private_Key& key,
         RandomNumberGenerator& rng,
         const std::string& emsa,
         Signature_Format format,
         const std::vector<uint8_t>& msg) override
         {
         BOTAN_UNUSED(key, rng);
         count_callback_invocation("tls_sign_message");

         if(key.algo_name() == "RSA")
            {
            if(format != Signature_Format::Standard)
               {
               throw Test_Error("TLS implementation selected unexpected signature format for RSA");
               }

            if(emsa != "PSSR(SHA-256,MGF1,32)")
               {
               throw Test_Error("TLS implementation selected unexpected padding for RSA: " + emsa);
               }
            }
         else if(key.algo_name() == "ECDSA")
            {
            if(format != Signature_Format::DerSequence)
               {
               throw Test_Error("TLS implementation selected unexpected signature format for ECDSA");
               }

            if(emsa != "EMSA1(SHA-256)")
               {
               throw Test_Error("TLS implementation selected unexpected padding for ECDSA: " + emsa);
               }
            }
         else
            {
            throw Test_Error("TLS implementation trying to sign with unexpected algorithm ("
                              + key.algo_name() + ")");
            }

         for(const auto& mock : m_mock_signatures)
            {
            if(mock.message_to_sign == msg)
               {
               return mock.signature_to_produce;
               }
            }

         throw Test_Error("TLS implementation produced an unexpected message to be signed: " +
                          Botan::hex_encode(msg));
         }

      bool tls_verify_message(
         const Public_Key& key,
         const std::string& emsa,
         Signature_Format format,
         const std::vector<uint8_t>& msg,
         const std::vector<uint8_t>& sig) override
         {
         count_callback_invocation("tls_verify_message");
         return Callbacks::tls_verify_message(key, emsa, format, msg, sig);
         }

      std::pair<secure_vector<uint8_t>, std::vector<uint8_t>> tls_dh_agree(
               const std::vector<uint8_t>& modulus,
               const std::vector<uint8_t>& generator,
               const std::vector<uint8_t>& peer_public_value,
               const Policy& policy,
               RandomNumberGenerator& rng) override
         {
         count_callback_invocation("tls_dh_agree");
         return Callbacks::tls_dh_agree(modulus, generator, peer_public_value, policy, rng);
         }

      std::pair<secure_vector<uint8_t>, std::vector<uint8_t>> tls_ecdh_agree(
               const std::string& curve_name,
               const std::vector<uint8_t>& peer_public_value,
               const Policy& policy,
               RandomNumberGenerator& rng,
               bool compressed) override
         {
         count_callback_invocation("tls_ecdh_agree");
         return Callbacks::tls_ecdh_agree(curve_name, peer_public_value, policy, rng, compressed);
         }

      void tls_inspect_handshake_msg(const Handshake_Message& message) override
         {
         count_callback_invocation("tls_inspect_handshake_msg_" + message.type_string());

         try {
            auto serialized_message = message.serialize();

            serialized_messages
               .try_emplace(message.type_string())
               .first->second
               .emplace_back(std::move(serialized_message));
         } catch (const Not_Implemented&) {
            // TODO: Once the server implementation is finished, this crutch
            //       can likely be removed, as all message types will have a
            //       serialization method with actual business logic. :o)
         }

         return Callbacks::tls_inspect_handshake_msg(message);
         }

      std::string tls_server_choose_app_protocol(const std::vector<std::string>& client_protos) override
         {
         count_callback_invocation("tls_server_choose_app_protocol");
         return Callbacks::tls_server_choose_app_protocol(client_protos);
         }

      void tls_modify_extensions(Botan::TLS::Extensions& exts, Botan::TLS::Connection_Side side, Botan::TLS::Handshake_Type which_message) override
         {
         count_callback_invocation(std::string("tls_modify_extensions_") + handshake_type_to_string(which_message));
         m_modify_exts(exts, side, which_message);
         Callbacks::tls_modify_extensions(exts, side, which_message);
         }

      void tls_examine_extensions(const Botan::TLS::Extensions& extn, Connection_Side which_side, Botan::TLS::Handshake_Type which_message) override
         {
         count_callback_invocation(std::string("tls_examine_extensions_") + handshake_type_to_string(which_message));
         return Callbacks::tls_examine_extensions(extn, which_side, which_message);
         }

      std::string tls_decode_group_param(Group_Params group_param) override
         {
         count_callback_invocation("tls_decode_group_param");
         return Callbacks::tls_decode_group_param(group_param);
         }

      std::string tls_peer_network_identity() override
         {
         count_callback_invocation("tls_peer_network_identity");
         return Callbacks::tls_peer_network_identity();
         }

      std::chrono::system_clock::time_point tls_current_timestamp() override
         {
         count_callback_invocation("tls_current_timestamp");
         return m_timestamp;
         }

      std::vector<uint8_t> pull_send_buffer()
         {
         return std::exchange(send_buffer, std::vector<uint8_t>());
         }

      std::vector<uint8_t> pull_receive_buffer()
         {
         return std::exchange(receive_buffer, std::vector<uint8_t>());
         }

      uint64_t last_received_seq_no() const { return received_seq_no; }

      const std::map<std::string, unsigned int>& callback_invocations() const
         {
         return m_callback_invocations;
         }

      void reset_callback_invocation_counters()
         {
         m_callback_invocations.clear();
         }

   private:
      void count_callback_invocation(const std::string& callback_name) const
         {
         if(!m_callback_invocations.contains(callback_name))
            { m_callback_invocations[callback_name] = 0; }

         m_callback_invocations[callback_name]++;
         }

   public:
      bool session_activated_called;

      std::vector<Botan::X509_Certificate> certificate_chain;
      std::map<std::string, std::vector<std::vector<uint8_t>>> serialized_messages;

   private:
      std::vector<uint8_t>                  send_buffer;
      std::vector<uint8_t>                  receive_buffer;
      uint64_t                              received_seq_no;
      Modify_Exts_Fn                        m_modify_exts;
      std::vector<MockSignature>            m_mock_signatures;
      std::chrono::system_clock::time_point m_timestamp;

      mutable std::map<std::string, unsigned int> m_callback_invocations;
   };

class Test_Credentials : public Botan::Credentials_Manager
   {
   public:
      explicit Test_Credentials(bool use_alternative_server_certificate)
         : m_alternative_server_certificate(use_alternative_server_certificate)
         {
         Botan::DataSource_Memory in(Test::read_data_file("tls_13_rfc8448/server_key.pem"));
         m_server_private_key.reset(Botan::PKCS8::load_key(in).release());

         // RFC 8448 does not actually provide these keys. Hence we generate one on the
         // fly as a stand-in. Instead of actually using it, the signatures generated
         // by this private key must be hard-coded in `Callbacks::sign_message()`; see
         // `MockSignature_Fn` for more details.
         m_bogus_alternative_server_private_key.reset(
            create_private_key("ECDSA", Test::rng(), "secp256r1").release());

         m_client_private_key.reset(create_private_key("RSA", Test::rng(), "1024").release());
         }

      std::vector<Botan::X509_Certificate> cert_chain(
         const std::vector<std::string>& cert_key_types,
         const std::vector<AlgorithmIdentifier>& cert_signature_schemes,
         const std::string& type,
         const std::string& context) override
         {
         BOTAN_UNUSED(cert_key_types, cert_signature_schemes, context);
         return
            {
            (type == "tls-client")
            ? client_certificate()
            : ((m_alternative_server_certificate)
               ? alternative_server_certificate()
               : server_certificate())
            };
         }

      std::shared_ptr<Botan::Private_Key>
      private_key_for(const Botan::X509_Certificate& cert,
                      const std::string& type,
                      const std::string& context) override
         {
         BOTAN_UNUSED(cert, context);

         if(type == "tls-client")
            return m_client_private_key;

         if(m_alternative_server_certificate)
            return m_bogus_alternative_server_private_key;

         return m_server_private_key;
         }

   private:
      bool m_alternative_server_certificate;
      std::shared_ptr<Private_Key> m_client_private_key;
      std::shared_ptr<Private_Key> m_bogus_alternative_server_private_key;
      std::shared_ptr<Private_Key> m_server_private_key;
   };

class RFC8448_Text_Policy : public Botan::TLS::Text_Policy
   {
   public:
      RFC8448_Text_Policy(const Botan::TLS::Text_Policy& other)
         : Text_Policy(other) {}

      std::vector<Botan::TLS::Signature_Scheme> allowed_signature_schemes() const override
         {
         // We extend the allowed signature schemes with algorithms that we don't
         // actually support. The nature of the RFC 8448 test forces us to generate
         // bit-compatible TLS messages. Unfortunately, the test data offers all
         // those algorithms in its Client Hellos.
         return
            {
            Botan::TLS::Signature_Scheme::ECDSA_SHA256,
            Botan::TLS::Signature_Scheme::ECDSA_SHA384,
            Botan::TLS::Signature_Scheme::ECDSA_SHA512,
            Botan::TLS::Signature_Scheme::ECDSA_SHA1,       // not actually supported
            Botan::TLS::Signature_Scheme::RSA_PSS_SHA256,
            Botan::TLS::Signature_Scheme::RSA_PSS_SHA384,
            Botan::TLS::Signature_Scheme::RSA_PSS_SHA512,
            Botan::TLS::Signature_Scheme::RSA_PKCS1_SHA256,
            Botan::TLS::Signature_Scheme::RSA_PKCS1_SHA384,
            Botan::TLS::Signature_Scheme::RSA_PKCS1_SHA512,
            Botan::TLS::Signature_Scheme::RSA_PKCS1_SHA1,   // not actually supported
            Botan::TLS::Signature_Scheme(0x0402),           // DSA_SHA256, not actually supported
            Botan::TLS::Signature_Scheme(0x0502),           // DSA_SHA384, not actually supported
            Botan::TLS::Signature_Scheme(0x0602),           // DSA_SHA512, not actually supported
            Botan::TLS::Signature_Scheme(0x0202),           // DSA_SHA1, not actually supported
            };
         }

      // Overriding the key exchange group selection to favour the server's key
      // exchange group preference. This is required to enforce a Hello Retry Request
      // when testing RFC 8448 5. from the server side.
      Named_Group choose_key_exchange_group(const std::vector<Group_Params>& supported_by_peer,
                                            const std::vector<Group_Params>& offered_by_peer) const override
         {
         BOTAN_UNUSED(offered_by_peer);

         const auto supported_by_us = key_exchange_groups();
         const auto selected_group = std::find_if(supported_by_us.begin(), supported_by_us.end(),
                                                  [&](const auto group)
                                                     {
                                                     return value_exists(supported_by_peer, group);
                                                     });

         return selected_group != supported_by_us.end()
            ? *selected_group
            : Named_Group::NONE;
         }
   };

/**
 * In-Memory Session Manager that stores sessions verbatim, without encryption.
 * Therefor it is not dependent on a random number generator and can easily be
 * instrumented for test inspection.
 */
class RFC8448_Session_Manager : public Botan::TLS::Session_Manager
   {
   private:
      decltype(auto) find_by_handle(const Session_Handle& handle)
         {
         return [=](const Session_with_Handle session)
            {
            if(session.handle.id().has_value() && handle.id().has_value() &&
               session.handle.id().value() == handle.id().value())
               { return true; }
            if(session.handle.ticket().has_value() && handle.ticket().has_value() &&
               session.handle.ticket().value() == handle.ticket().value())
               { return true; }
            return false;
            };
         }

   public:
      RFC8448_Session_Manager()
         : Session_Manager(m_null_rng) {}

      const std::vector<Session_with_Handle>& all_sessions() const
         {
         return m_sessions;
         }

      void store(const Session& session, const Session_Handle& handle) override
         {
         m_sessions.push_back({session, handle});
         }

      std::optional<Session_Handle> establish(const Session& session, std::optional<Session_ID> id, bool) override
         {
         m_sessions.emplace_back(Session_with_Handle{session, id.value_or(Session_ID())});
         return id;
         }

      std::optional<Session> retrieve_one(const Session_Handle& handle) override
         {
         auto itr = std::find_if(m_sessions.begin(), m_sessions.end(), find_by_handle(handle));
         if(itr == m_sessions.end())
            return std::nullopt;
         else
            return itr->session;
         }

      std::vector<Session_with_Handle> find_all(const Server_Information& info) override
         {
         std::vector<Session_with_Handle> found_sessions;
         for(const auto& [session, handle] : m_sessions)
            {
            if(session.server_info() == info)
               { found_sessions.emplace_back(Session_with_Handle{session, handle}); }
            }

         return found_sessions;
         }

      size_t remove(const Session_Handle& handle) override
         {
         // TODO: C++20 allows to simply implement the entire method like:
         //
         //   return std::erase_if(m_sessions, find_by_handle(handle));
         //
         // Unfortunately, at the time of this writing Android NDK shipped with
         // a std::erase_if that returns void.
         auto rm_itr = std::remove_if(m_sessions.begin(),
                                      m_sessions.end(),
                                      find_by_handle(handle));

         const auto elements_being_removed = std::distance(rm_itr, m_sessions.end());
         m_sessions.erase(rm_itr);
         return elements_being_removed;
         }

      size_t remove_all() override
         {
         const auto sessions = m_sessions.size();
         m_sessions.clear();
         return sessions;
         }

   private:
      std::vector<Session_with_Handle> m_sessions;
      Botan::Null_RNG m_null_rng;
   };

/**
 * This steers the TLS client handle and is the central entry point for the
 * test cases to interact with the TLS 1.3 implementation.
 *
 * Note: This class is abstract to be subclassed for both client and server tests.
 */
class TLS_Context
   {
   protected:
      TLS_Context(std::unique_ptr<Botan::RandomNumberGenerator> rng_in,
                  RFC8448_Text_Policy policy,
                  Modify_Exts_Fn modify_exts_cb,
                  std::vector<MockSignature> mock_signatures,
                  uint64_t timestamp,
                  std::optional<std::pair<Session, Session_Ticket>> session_and_ticket,
                  bool use_alternative_server_certificate)
         : m_callbacks(std::move(modify_exts_cb), std::move(mock_signatures), timestamp)
         , m_creds(use_alternative_server_certificate)
         , m_rng(std::move(rng_in))
         , m_policy(std::move(policy))
         {
         if(session_and_ticket.has_value())
            {
            m_session_mgr.store(std::get<Session>(session_and_ticket.value()),
                                std::get<Session_Ticket>(session_and_ticket.value()));
            }
         }

   public:
      virtual ~TLS_Context() = default;

      TLS_Context(TLS_Context&) = delete;
      TLS_Context& operator=(const TLS_Context&) = delete;

      TLS_Context(TLS_Context&&) = delete;
      TLS_Context& operator=(TLS_Context&&) = delete;

      std::vector<uint8_t> pull_send_buffer()
         {
         return m_callbacks.pull_send_buffer();
         }

      std::vector<uint8_t> pull_receive_buffer()
         {
         return m_callbacks.pull_receive_buffer();
         }

      uint64_t last_received_seq_no() const { return m_callbacks.last_received_seq_no(); }

      /**
       * Checks that all of the listed callbacks were called at least once, no other
       * callbacks were called in addition to the expected ones. After the checks are
       * done, the callback invocation counters are reset.
       */
      void check_callback_invocations(Test::Result& result, const std::string& context,
                                      const std::vector<std::string>& callback_names)
         {
         const auto& invokes = m_callbacks.callback_invocations();
         for(const auto& cbn : callback_names)
            {
            result.confirm(cbn + " was invoked (Context: " + context + ")", invokes.contains(cbn) && invokes.at(cbn) > 0);
            }

         for(const auto& invoke : invokes)
            {
            if(invoke.second == 0)
               { continue; }
            result.confirm(invoke.first + " was expected (Context: " + context + ")", std::find(callback_names.cbegin(),
                           callback_names.cend(), invoke.first) != callback_names.cend());
            }

         m_callbacks.reset_callback_invocation_counters();
         }

      const std::vector<Session_with_Handle>& stored_sessions() const
         {
         return m_session_mgr.all_sessions();
         }

      const std::vector<Botan::X509_Certificate>& certs_verified() const
         {
         return m_callbacks.certificate_chain;
         }

      decltype(auto) observed_handshake_messages() const
         {
         return m_callbacks.serialized_messages;
         }

      /**
       * Send application data through the secure channel
       */
      virtual void send(const std::vector<uint8_t>& data) = 0;

   protected:
      Test_TLS_13_Callbacks m_callbacks;
      Test_Credentials      m_creds;

      std::unique_ptr<Botan::RandomNumberGenerator> m_rng;
      RFC8448_Session_Manager                       m_session_mgr;
      RFC8448_Text_Policy                           m_policy;
   };

class Client_Context : public TLS_Context
   {
   public:
      Client_Context(std::unique_ptr<Botan::RandomNumberGenerator> rng_in,
                     RFC8448_Text_Policy policy,
                     uint64_t timestamp,
                     Modify_Exts_Fn modify_exts_cb,
                     std::optional<std::pair<Session, Session_Ticket>> session_and_ticket = std::nullopt,
                     std::vector<MockSignature> mock_signatures = {})
         : TLS_Context(std::move(rng_in), std::move(policy), std::move(modify_exts_cb), std::move(mock_signatures), timestamp, std::move(session_and_ticket), false)
         , client(m_callbacks, m_session_mgr, m_creds, m_policy, *m_rng,
                  Botan::TLS::Server_Information("server"),
                  Botan::TLS::Protocol_Version::TLS_V13)
         {}

      void send(const std::vector<uint8_t>& data) override
         {
         client.send(data.data(), data.size());
         }

      Botan::TLS::Client client;
   };

class Server_Context : public TLS_Context
   {
   public:
      Server_Context(std::unique_ptr<Botan::RandomNumberGenerator> rng,
                     RFC8448_Text_Policy policy,
                     uint64_t timestamp,
                     Modify_Exts_Fn modify_exts_cb,
                     std::vector<MockSignature> mock_signatures,
                     bool use_alternative_server_certificate = false)
         : TLS_Context(std::move(rng), std::move(policy),
            std::move(modify_exts_cb),
            std::move(mock_signatures),
            timestamp,
            std::nullopt /* session not yet needed */,
            use_alternative_server_certificate)
         , server(m_callbacks, m_session_mgr, m_creds, m_policy, *m_rng, false /* DTLS NYI */) {}

      void send(const std::vector<uint8_t>& data) override
         {
         server.send(data.data(), data.size());
         }

      Botan::TLS::Server server;
   };

/**
 * Because of the nature of the RFC 8448 test data we need to produce bit-compatible
 * TLS messages. Hence we sort the generated TLS extensions exactly as expected.
 */
void sort_client_extensions(Botan::TLS::Extensions& exts, Botan::TLS::Connection_Side side)
   {
   if(side == Botan::TLS::Connection_Side::Client)
      {
      const std::vector<Botan::TLS::Extension_Code> expected_order =
         {
         Botan::TLS::Extension_Code::ServerNameIndication,
         Botan::TLS::Extension_Code::SafeRenegotiation,
         Botan::TLS::Extension_Code::SupportedGroups,
         Botan::TLS::Extension_Code::SessionTicket,
         Botan::TLS::Extension_Code::KeyShare,
         Botan::TLS::Extension_Code::EarlyData,
         Botan::TLS::Extension_Code::SupportedVersions,
         Botan::TLS::Extension_Code::SignatureAlgorithms,
         Botan::TLS::Extension_Code::Cookie,
         Botan::TLS::Extension_Code::PskKeyExchangeModes,
         Botan::TLS::Extension_Code::RecordSizeLimit,
         Padding::static_type()
         };

      for(const auto ext_type : expected_order)
         {
         auto ext = exts.take(ext_type);
         if(ext != nullptr)
            {
            exts.add(std::move(ext));
            }
         }
      }
   }

/**
 * Because of the nature of the RFC 8448 test data we need to produce bit-compatible
 * TLS messages. Hence we sort the generated TLS extensions exactly as expected.
 */
void sort_server_extensions(Botan::TLS::Extensions& exts, Botan::TLS::Connection_Side side, Botan::TLS::Handshake_Type /*type*/)
   {
   if(side == Botan::TLS::Connection_Side::Server)
      {
      const std::vector<Botan::TLS::Extension_Code> expected_order =
         {
         Botan::TLS::Extension_Code::SupportedGroups,
         Botan::TLS::Extension_Code::KeyShare,
         Botan::TLS::Extension_Code::Cookie,
         Botan::TLS::Extension_Code::SupportedVersions,
         Botan::TLS::Extension_Code::SignatureAlgorithms,
         Botan::TLS::Extension_Code::RecordSizeLimit,
         Botan::TLS::Extension_Code::ServerNameIndication
         };

      for(const auto ext_type : expected_order)
         {
         auto ext = exts.take(ext_type);
         if(ext != nullptr)
            {
            exts.add(std::move(ext));
            }
         }
      }
   }

void add_renegotiation_extension(Botan::TLS::Extensions& exts)
   {
   // Renegotiation is not possible in TLS 1.3. Nevertheless, RFC 8448 requires
   // to add this to the Client Hello for reasons.
   exts.add(new Renegotiation_Extension());
   }

void add_early_data_indication(Botan::TLS::Extensions& exts)
   {
   exts.add(new Botan::TLS::EarlyDataIndication());
   }

std::vector<uint8_t> strip_message_header(const std::vector<uint8_t>& msg)
   {
   BOTAN_ASSERT_NOMSG(msg.size() >= 4);
   return {msg.begin() + 4, msg.end()};
   }

std::vector<MockSignature> make_mock_signatures(const VarMap& vars)
   {
   std::vector<MockSignature> result;

   auto mock = [&](const std::string& msg, const std::string& sig)
      {
      if(vars.has_key(msg) && vars.has_key(sig))
         result.push_back({vars.get_opt_bin(msg), vars.get_opt_bin(sig)});
      };

   mock("Server_MessageToSign", "Server_MessageSignature");
   mock("Client_MessageToSign", "Client_MessageSignature");

   return result;
   }

}  // namespace

/**
 * Traffic transcripts and supporting data for the TLS RFC 8448 and TLS policy
 * configuration is kept in data files (accessible via `Test:::data_file()`).
 *
 * tls_13_rfc8448/transcripts.vec
 *   The record transcripts and RNG outputs as defined/required in RFC 8448 in
 *   Botan's Text_Based_Test vector format. Data from each RFC 8448 section is
 *   placed in a sub-section of the *.vec file. Each of those sections needs a
 *   specific test case implementation that is dispatched in `run_one_test()`.
 *
 * tls_13_rfc8448/client_certificate.pem
 *   The client certificate provided in RFC 8448 used to perform client auth.
 *   Note that RFC 8448 _does not_ provide the associated private key but only
 *   the resulting signature in the client's CertificateVerify message.
 *
 * tls_13_rfc8448/server_certificate.pem
 * tls_13_rfc8448/server_key.pem
 *   The server certificate and its associated private key.
 *
 * tls_13_rfc8448/server_certificate_client_auth.pem
 *   The server certificate used in the Client Authentication test case.
 *
 * tls-policy/rfc8448_*.txt
 *   Each RFC 8448 section test required a slightly adapted Botan TLS policy
 *   to enable/disable certain features under test.
 *
 * While the test cases are split into Client-side and Server-side tests, the
 * transcript data is reused. See the concrete implementations of the abstract
 * Test_TLS_RFC8448 test class.
 */
class Test_TLS_RFC8448 : public Text_Based_Test
   {
   protected:
      virtual std::vector<Test::Result> simple_1_rtt(const VarMap& vars) = 0;
      virtual std::vector<Test::Result> resumed_handshake_with_0_rtt(const VarMap& vars) = 0;
      virtual std::vector<Test::Result> hello_retry_request(const VarMap& vars) = 0;
      virtual std::vector<Test::Result> client_authentication(const VarMap& vars) = 0;
      virtual std::vector<Test::Result> middlebox_compatibility(const VarMap& vars) = 0;

      virtual std::string side() const = 0;

   public:
      Test_TLS_RFC8448()
         : Text_Based_Test("tls_13_rfc8448/transcripts.vec",
                           // mandatory data fields
                           "Client_RNG_Pool,"
                           "Server_RNG_Pool,"
                           "CurrentTimestamp,"
                           "Record_ClientHello_1,"
                           "Record_ServerHello,"
                           "Record_ServerHandshakeMessages,"
                           "Record_ClientFinished,"
                           "Record_Client_CloseNotify,"
                           "Record_Server_CloseNotify",
                           // optional data fields
                           "Message_ServerHello,"
                           "Message_EncryptedExtensions,"
                           "Message_CertificateRequest,"
                           "Message_Server_Certificate,"
                           "Message_Server_CertificateVerify,"
                           "Message_Server_Finished,"
                           "Record_HelloRetryRequest,"
                           "Record_ClientHello_2,"
                           "Record_NewSessionTicket,"
                           "Client_AppData,"
                           "Record_Client_AppData,"
                           "Server_AppData,"
                           "Record_Server_AppData,"
                           "Client_EarlyAppData,"
                           "Record_Client_EarlyAppData,"
                           "SessionTicket,"
                           "Client_SessionData,"
                           "Server_MessageToSign,"
                           "Server_MessageSignature,"
                           "Client_MessageToSign,"
                           "Client_MessageSignature,"
                           "HelloRetryRequest_Cookie") {}

      Test::Result run_one_test(const std::string& header,
                                const VarMap& vars) override
         {
         if(header == "Simple_1RTT_Handshake")
            return Test::Result("Simple 1-RTT (" + side() + " side)", simple_1_rtt(vars));
         else if(header == "Resumed_0RTT_Handshake")
            return Test::Result("Resumption with 0-RTT data (" + side() + " side)", resumed_handshake_with_0_rtt(vars));
         else if(header == "HelloRetryRequest_Handshake")
            return Test::Result("Handshake involving Hello Retry Request (" + side() + " side)", hello_retry_request(vars));
         else if(header == "Client_Authentication_Handshake")
            return Test::Result("Client Authentication (" + side() + " side)", client_authentication(vars));
         else if(header == "Middlebox_Compatibility_Mode")
            return Test::Result("Middlebox Compatibility Mode (" + side() + " side)", middlebox_compatibility(vars));
         else
            return Test::Result::Failure("test dispatcher", "unknown sub-test: " + header);
         }
   };

class Test_TLS_RFC8448_Client : public Test_TLS_RFC8448
   {
   private:
      std::string side() const override { return "Client"; }

      std::vector<Test::Result> simple_1_rtt(const VarMap& vars) override
         {
         auto rng = std::make_unique<Botan_Tests::Fixed_Output_RNG>("");

         // 32 - for client hello random
         // 32 - for KeyShare (eph. x25519 key pair)
         add_entropy(*rng, vars.get_req_bin("Client_RNG_Pool"));

         auto add_extensions_and_sort = [](Botan::TLS::Extensions& exts, Botan::TLS::Connection_Side side, Botan::TLS::Handshake_Type which_message)
            {
            if(which_message == Handshake_Type::ClientHello)
               {
               // For some reason, presumably checking compatibility, the RFC 8448 Client
               // Hello includes a (TLS 1.2) Session_Ticket extension. We don't normally add
               // this obsoleted extension in a TLS 1.3 client.
               exts.add(new Botan::TLS::Session_Ticket_Extension());

               add_renegotiation_extension(exts);
               sort_client_extensions(exts, side);
               }
            };

         std::unique_ptr<Client_Context> ctx;

         return {
            Botan_Tests::CHECK("Client Hello", [&](Test::Result& result)
               {
               ctx = std::make_unique<Client_Context>(std::move(rng), read_tls_policy("rfc8448_1rtt"), vars.get_req_u64("CurrentTimestamp"), add_extensions_and_sort);

               result.confirm("client not closed", !ctx->client.is_closed());
               ctx->check_callback_invocations(result, "client hello prepared",
                  {
                  "tls_emit_data",
                  "tls_inspect_handshake_msg_client_hello",
                  "tls_modify_extensions_client_hello"
                  });

               result.test_eq("TLS client hello", ctx->pull_send_buffer(), vars.get_req_bin("Record_ClientHello_1"));
               }),

            Botan_Tests::CHECK("Server Hello", [&](Test::Result& result)
               {
               result.require("ctx is available", ctx != nullptr);
               const auto server_hello = vars.get_req_bin("Record_ServerHello");
               // splitting the input data to test partial reads
               const std::vector<uint8_t> server_hello_a(server_hello.begin(), server_hello.begin() + 20);
               const std::vector<uint8_t> server_hello_b(server_hello.begin() + 20, server_hello.end());

               ctx->client.received_data(server_hello_a);
               ctx->check_callback_invocations(result, "server hello partially received", { });

               ctx->client.received_data(server_hello_b);
               ctx->check_callback_invocations(result, "server hello received",
                  {
                  "tls_inspect_handshake_msg_server_hello",
                  "tls_examine_extensions_server_hello"
                  });

               result.confirm("client is not yet active", !ctx->client.is_active());
               }),

            Botan_Tests::CHECK("Server HS messages .. Client Finished", [&](Test::Result& result)
               {
               result.require("ctx is available", ctx != nullptr);
               ctx->client.received_data(vars.get_req_bin("Record_ServerHandshakeMessages"));

               ctx->check_callback_invocations(result, "encrypted handshake messages received",
                  {
                  "tls_inspect_handshake_msg_encrypted_extensions",
                  "tls_inspect_handshake_msg_certificate",
                  "tls_inspect_handshake_msg_certificate_verify",
                  "tls_inspect_handshake_msg_finished",
                  "tls_examine_extensions_encrypted_extensions",
                  "tls_examine_extensions_certificate",
                  "tls_emit_data",
                  "tls_session_activated",
                  "tls_verify_cert_chain",
                  "tls_verify_message"
                  });
               result.require("certificate exists", !ctx->certs_verified().empty());
               result.require("correct certificate", ctx->certs_verified().front() == server_certificate());
               result.require("client is active", ctx->client.is_active());

               result.test_eq("correct handshake finished", ctx->pull_send_buffer(),
                              vars.get_req_bin("Record_ClientFinished"));
               }),

            Botan_Tests::CHECK("Post-Handshake: NewSessionTicket", [&](Test::Result& result)
               {
               result.require("ctx is available", ctx != nullptr);
               result.require("no sessions so far", ctx->stored_sessions().empty());
               ctx->client.received_data(vars.get_req_bin("Record_NewSessionTicket"));

               ctx->check_callback_invocations(result, "new session ticket received",
                  {
                  "tls_examine_extensions_new_session_ticket",
                  "tls_session_ticket_received",
                  "tls_current_timestamp"
                  });
               if(result.test_eq("session was stored", ctx->stored_sessions().size(), 1))
                  {
                  const auto& [stored_session, stored_handle] = ctx->stored_sessions().front();
                  result.require("session handle contains a ticket", stored_handle.ticket().has_value());
                  result.test_is_eq("session was serialized as expected", Botan::unlock(stored_session.DER_encode()), vars.get_req_bin("Client_SessionData"));
                  }
               }),

            Botan_Tests::CHECK("Send Application Data", [&](Test::Result& result)
               {
               result.require("ctx is available", ctx != nullptr);
               ctx->send(vars.get_req_bin("Client_AppData"));

               ctx->check_callback_invocations(result, "application data sent", { "tls_emit_data" });

               result.test_eq("correct client application data", ctx->pull_send_buffer(),
                              vars.get_req_bin("Record_Client_AppData"));
               }),

            Botan_Tests::CHECK("Receive Application Data", [&](Test::Result& result)
               {
               result.require("ctx is available", ctx != nullptr);
               ctx->client.received_data(vars.get_req_bin("Record_Server_AppData"));

               ctx->check_callback_invocations(result, "application data sent", { "tls_record_received" });

               const auto rcvd = ctx->pull_receive_buffer();
               result.test_eq("decrypted application traffic", rcvd, vars.get_req_bin("Server_AppData"));
               result.test_is_eq("sequence number", ctx->last_received_seq_no(), uint64_t(1));
               }),

            Botan_Tests::CHECK("Close Connection", [&](Test::Result& result)
               {
               result.require("ctx is available", ctx != nullptr);
               ctx->client.close();

               result.test_eq("close payload", ctx->pull_send_buffer(), vars.get_req_bin("Record_Client_CloseNotify"));
               ctx->check_callback_invocations(result, "CLOSE_NOTIFY sent", { "tls_emit_data" });

               ctx->client.received_data(vars.get_req_bin("Record_Server_CloseNotify"));
               ctx->check_callback_invocations(result, "CLOSE_NOTIFY received", { "tls_alert", "tls_peer_closed_connection" });

               result.confirm("connection is closed", ctx->client.is_closed());
               }),
            };
         }

      std::vector<Test::Result> resumed_handshake_with_0_rtt(const VarMap& vars) override
         {
         auto rng = std::make_unique<Botan_Tests::Fixed_Output_RNG>("");

         // 32 - for client hello random
         // 32 - for KeyShare (eph. x25519 key pair)
         add_entropy(*rng, vars.get_req_bin("Client_RNG_Pool"));

         auto add_extensions_and_sort = [](Botan::TLS::Extensions& exts, Botan::TLS::Connection_Side side, Botan::TLS::Handshake_Type which_message)
            {
            if(which_message == Handshake_Type::ClientHello)
               {
               exts.add(new Padding(87));

               add_renegotiation_extension(exts);

               // TODO: Implement early data support and remove this 'hack'.
               //
               // Currently, the production implementation will never add this
               // extension even if the resumed session would allow early data.
               add_early_data_indication(exts);
               sort_client_extensions(exts, side);
               }
            };

         std::unique_ptr<Client_Context> ctx;

         return {
            Botan_Tests::CHECK("Client Hello", [&](Test::Result& result)
               {
               ctx = std::make_unique<Client_Context>(std::move(rng),
                                                      read_tls_policy("rfc8448_1rtt"),
                                                      vars.get_req_u64("CurrentTimestamp"),
                                                      add_extensions_and_sort,
                                                      std::pair{
                                                      Session(vars.get_req_bin("Client_SessionData")),
                                                      Session_Ticket(vars.get_req_bin("SessionTicket"))
                                                      });

               result.confirm("client not closed", !ctx->client.is_closed());
               ctx->check_callback_invocations(result, "client hello prepared",
                  {
                  "tls_emit_data",
                  "tls_inspect_handshake_msg_client_hello",
                  "tls_modify_extensions_client_hello",
                  "tls_current_timestamp"
                  });

               result.test_eq("TLS client hello", ctx->pull_send_buffer(), vars.get_req_bin("Record_ClientHello_1"));
               })

            // TODO: The rest of this test vector requires 0-RTT which is not
            //       yet implemented. For now we can only test the client's
            //       ability to offer a session resumption via PSK.
            };
         }

      std::vector<Test::Result> hello_retry_request(const VarMap& vars) override
         {
         auto add_extensions_and_sort = [flights = 0](Botan::TLS::Extensions& exts, Botan::TLS::Connection_Side side, Botan::TLS::Handshake_Type which_message) mutable
            {
            if(which_message == Handshake_Type::ClientHello)
               {
               ++flights;

               if(flights == 1)
                  {
                  add_renegotiation_extension(exts);
                  }

               // For some reason RFC8448 decided to require this (fairly obscure) extension
               // in the second flight of the Client_Hello.
               if(flights == 2)
                  {
                  exts.add(new Padding(175));
                  }

               sort_client_extensions(exts, side);
               }
            };

         // Fallback RNG is required to for blinding in ECDH with P-256
         auto& fallback_rng = Test::rng();
         auto rng = std::make_unique<Botan_Tests::Fixed_Output_RNG>(fallback_rng);

         // 32 - client hello random
         // 32 - eph. x25519 key pair
         // 32 - eph. P-256 key pair
         add_entropy(*rng, vars.get_req_bin("Client_RNG_Pool"));

         std::unique_ptr<Client_Context> ctx;

         return {
            Botan_Tests::CHECK("Client Hello", [&](Test::Result& result)
               {
               ctx = std::make_unique<Client_Context>(std::move(rng), read_tls_policy("rfc8448_hrr_client"), vars.get_req_u64("CurrentTimestamp"), add_extensions_and_sort);
               result.confirm("client not closed", !ctx->client.is_closed());

               ctx->check_callback_invocations(result, "client hello prepared",
                  {
                  "tls_emit_data",
                  "tls_inspect_handshake_msg_client_hello",
                  "tls_modify_extensions_client_hello",
                  });

               result.test_eq("TLS client hello (1)", ctx->pull_send_buffer(), vars.get_req_bin("Record_ClientHello_1"));
               }),

            Botan_Tests::CHECK("Hello Retry Request .. second Client Hello", [&](Test::Result& result)
               {
               result.require("ctx is available", ctx != nullptr);
               ctx->client.received_data(vars.get_req_bin("Record_HelloRetryRequest"));

               ctx->check_callback_invocations(result, "hello retry request received",
                  {
                  "tls_emit_data",
                  "tls_inspect_handshake_msg_hello_retry_request",
                  "tls_examine_extensions_hello_retry_request",
                  "tls_inspect_handshake_msg_client_hello",
                  "tls_modify_extensions_client_hello",
                  "tls_decode_group_param"
                  });

               result.test_eq("TLS client hello (2)", ctx->pull_send_buffer(), vars.get_req_bin("Record_ClientHello_2"));
               }),

            Botan_Tests::CHECK("Server Hello", [&](Test::Result& result)
               {
               result.require("ctx is available", ctx != nullptr);
               ctx->client.received_data(vars.get_req_bin("Record_ServerHello"));

               ctx->check_callback_invocations(result, "server hello received",
                  {
                  "tls_inspect_handshake_msg_server_hello",
                  "tls_examine_extensions_server_hello",
                  "tls_decode_group_param"
                  });
               }),

            Botan_Tests::CHECK("Server HS Messages .. Client Finished", [&](Test::Result& result)
               {
               result.require("ctx is available", ctx != nullptr);
               ctx->client.received_data(vars.get_req_bin("Record_ServerHandshakeMessages"));

               ctx->check_callback_invocations(result, "encrypted handshake messages received",
                  {
                  "tls_inspect_handshake_msg_encrypted_extensions",
                  "tls_inspect_handshake_msg_certificate",
                  "tls_inspect_handshake_msg_certificate_verify",
                  "tls_inspect_handshake_msg_finished",
                  "tls_examine_extensions_encrypted_extensions",
                  "tls_examine_extensions_certificate",
                  "tls_emit_data",
                  "tls_session_activated",
                  "tls_verify_cert_chain",
                  "tls_verify_message"
                  });

               result.test_eq("client finished", ctx->pull_send_buffer(), vars.get_req_bin("Record_ClientFinished"));
               }),

            Botan_Tests::CHECK("Close Connection", [&](Test::Result& result)
               {
               result.require("ctx is available", ctx != nullptr);
               ctx->client.close();
               ctx->check_callback_invocations(result, "encrypted handshake messages received", { "tls_emit_data" });
               result.test_eq("client close notify", ctx->pull_send_buffer(), vars.get_req_bin("Record_Client_CloseNotify"));

               ctx->client.received_data(vars.get_req_bin("Record_Server_CloseNotify"));
               ctx->check_callback_invocations(result, "encrypted handshake messages received", { "tls_alert", "tls_peer_closed_connection" });

               result.confirm("connection is closed", ctx->client.is_closed());
               }),
            };
         }

      std::vector<Test::Result> client_authentication(const VarMap& vars) override
         {
         auto rng = std::make_unique<Botan_Tests::Fixed_Output_RNG>("");

         // 32 - for client hello random
         // 32 - for eph. x25519 key pair
         add_entropy(*rng, vars.get_req_bin("Client_RNG_Pool"));

         auto add_extensions_and_sort = [&](Botan::TLS::Extensions& exts, Botan::TLS::Connection_Side side, Botan::TLS::Handshake_Type which_message)
            {
            if(which_message == Handshake_Type::ClientHello)
               {
               add_renegotiation_extension(exts);
               sort_client_extensions(exts, side);
               }
            };

         std::unique_ptr<Client_Context> ctx;

         return {
            Botan_Tests::CHECK("Client Hello", [&](Test::Result& result)
               {

               ctx = std::make_unique<Client_Context>(std::move(rng),
                                                      read_tls_policy("rfc8448_1rtt"),
                                                      vars.get_req_u64("CurrentTimestamp"),
                                                      add_extensions_and_sort,
                                                      std::nullopt,
                                                      make_mock_signatures(vars));

               ctx->check_callback_invocations(result, "initial callbacks", {
                  "tls_emit_data",
                  "tls_inspect_handshake_msg_client_hello",
                  "tls_modify_extensions_client_hello",
                  });

               result.test_eq("Client Hello", ctx->pull_send_buffer(), vars.get_req_bin("Record_ClientHello_1"));
               }),

            Botan_Tests::CHECK("Server Hello", [&](auto& result)
               {
               ctx->client.received_data(vars.get_req_bin("Record_ServerHello"));

               ctx->check_callback_invocations(result, "callbacks after server hello", {
                  "tls_examine_extensions_server_hello",
                  "tls_inspect_handshake_msg_server_hello",
                  });
               }),

            Botan_Tests::CHECK("other handshake messages and client auth", [&](Test::Result& result)
               {
               ctx->client.received_data(vars.get_req_bin("Record_ServerHandshakeMessages"));

               ctx->check_callback_invocations(result, "signing callbacks invoked", {
                  "tls_sign_message",
                  "tls_emit_data",
                  "tls_examine_extensions_encrypted_extensions",
                  "tls_examine_extensions_certificate",
                  "tls_examine_extensions_certificate_request",
                  "tls_modify_extensions_certificate",
                  "tls_inspect_handshake_msg_certificate",
                  "tls_inspect_handshake_msg_certificate_request",
                  "tls_inspect_handshake_msg_certificate_verify",
                  "tls_inspect_handshake_msg_encrypted_extensions",
                  "tls_inspect_handshake_msg_finished",
                  "tls_session_activated",
                  "tls_verify_cert_chain",
                  "tls_verify_message",
                  });

               // ClientFinished contains the entire coalesced client authentication flight
               // Messages: Certificate, CertificateVerify, Finished
               result.test_eq("Client Auth and Finished", ctx->pull_send_buffer(), vars.get_req_bin("Record_ClientFinished"));
               }),

            Botan_Tests::CHECK("Close Connection", [&](Test::Result& result)
               {
               ctx->client.close();
               result.test_eq("Client close_notify", ctx->pull_send_buffer(), vars.get_req_bin("Record_Client_CloseNotify"));

               ctx->check_callback_invocations(result, "after sending close notify", {
                  "tls_emit_data",
                  });

               ctx->client.received_data(vars.get_req_bin("Record_Server_CloseNotify"));
               result.confirm("connection closed", ctx->client.is_closed());

               ctx->check_callback_invocations(result, "after receiving close notify", {
                  "tls_alert",
                  "tls_peer_closed_connection"
                  });
               }),
         };
      }

      std::vector<Test::Result> middlebox_compatibility(const VarMap& vars) override
         {
         auto rng = std::make_unique<Botan_Tests::Fixed_Output_RNG>("");

         // 32 - client hello random
         // 32 - legacy session ID
         // 32 - eph. x25519 key pair
         add_entropy(*rng, vars.get_req_bin("Client_RNG_Pool"));

         auto add_extensions_and_sort = [&](Botan::TLS::Extensions& exts, Botan::TLS::Connection_Side side, Botan::TLS::Handshake_Type which_message)
            {
            if(which_message == Handshake_Type::ClientHello)
               {
               add_renegotiation_extension(exts);
               sort_client_extensions(exts, side);
               }
            };

         std::unique_ptr<Client_Context> ctx;

         return {
            Botan_Tests::CHECK("Client Hello", [&](Test::Result& result)
               {
               ctx = std::make_unique<Client_Context>(std::move(rng), read_tls_policy("rfc8448_compat_client"), vars.get_req_u64("CurrentTimestamp"), add_extensions_and_sort);

               result.test_eq("Client Hello", ctx->pull_send_buffer(), vars.get_req_bin("Record_ClientHello_1"));

               ctx->check_callback_invocations(result, "client hello prepared",
                  {
                  "tls_emit_data",
                  "tls_inspect_handshake_msg_client_hello",
                  "tls_modify_extensions_client_hello",
                  });
               }),

            Botan_Tests::CHECK("Server Hello + other handshake messages", [&](Test::Result& result)
               {
               result.require("ctx is available", ctx != nullptr);
               ctx->client.received_data(Botan::concat(vars.get_req_bin("Record_ServerHello"),
                                                      // ServerHandshakeMessages contains the expected ChangeCipherSpec record
                                                      vars.get_req_bin("Record_ServerHandshakeMessages")));

               ctx->check_callback_invocations(result, "callbacks after server's first flight", {
                  "tls_inspect_handshake_msg_server_hello",
                  "tls_inspect_handshake_msg_encrypted_extensions",
                  "tls_inspect_handshake_msg_certificate",
                  "tls_inspect_handshake_msg_certificate_verify",
                  "tls_inspect_handshake_msg_finished",
                  "tls_examine_extensions_server_hello",
                  "tls_examine_extensions_encrypted_extensions",
                  "tls_examine_extensions_certificate",
                  "tls_emit_data",
                  "tls_session_activated",
                  "tls_verify_cert_chain",
                  "tls_verify_message"
                  });

               result.test_eq("CCS + Client Finished", ctx->pull_send_buffer(),
                              // ClientFinished contains the expected ChangeCipherSpec record
                              vars.get_req_bin("Record_ClientFinished"));

               result.confirm("client is ready to send application traffic", ctx->client.is_active());
               }),

            Botan_Tests::CHECK("Close connection", [&](Test::Result& result)
               {
               result.require("ctx is available", ctx != nullptr);
               ctx->client.close();

               result.test_eq("Client close_notify", ctx->pull_send_buffer(), vars.get_req_bin("Record_Client_CloseNotify"));

               result.require("client cannot send application traffic anymore", !ctx->client.is_active());
               result.require("client is not fully closed yet", !ctx->client.is_closed());

               ctx->client.received_data(vars.get_req_bin("Record_Server_CloseNotify"));

               result.confirm("client connection was terminated", ctx->client.is_closed());
               }),
            };
         }
   };

class Test_TLS_RFC8448_Server : public Test_TLS_RFC8448
   {
   private:
      std::string side() const override { return "Server"; }

      std::vector<Test::Result> simple_1_rtt(const VarMap& vars) override
         {
         auto rng = std::make_unique<Botan_Tests::Fixed_Output_RNG>("");

         // 32 - for server hello random
         // 32 - for KeyShare (eph. x25519 key pair)  --  I guess?
         //  4 - for ticket_age_add (in New Session Ticket)
         add_entropy(*rng, vars.get_req_bin("Server_RNG_Pool"));

         std::unique_ptr<Server_Context> ctx;

         return {
            Botan_Tests::CHECK("Send Client Hello", [&](Test::Result& result)
               {
               ctx = std::make_unique<Server_Context>(std::move(rng), read_tls_policy("rfc8448_1rtt"), vars.get_req_u64("CurrentTimestamp"), sort_server_extensions, make_mock_signatures(vars));
               result.confirm("server not closed", !ctx->server.is_closed());

               ctx->server.received_data(vars.get_req_bin("Record_ClientHello_1"));

               ctx->check_callback_invocations(result, "client hello received", {
                  "tls_emit_data",
                  "tls_examine_extensions_client_hello",
                  "tls_modify_extensions_server_hello",
                  "tls_modify_extensions_encrypted_extensions",
                  "tls_modify_extensions_certificate",
                  "tls_sign_message",
                  "tls_inspect_handshake_msg_client_hello",
                  "tls_inspect_handshake_msg_server_hello",
                  "tls_inspect_handshake_msg_encrypted_extensions",
                  "tls_inspect_handshake_msg_certificate",
                  "tls_inspect_handshake_msg_certificate_verify",
                  "tls_inspect_handshake_msg_finished"
                  });
               }),

            Botan_Tests::CHECK("Verify generated messages in server's first flight", [&](Test::Result& result)
               {
               const auto& msgs = ctx->observed_handshake_messages();

               result.test_eq("Server Hello", msgs.at("server_hello")[0], strip_message_header(vars.get_opt_bin("Message_ServerHello")));
               result.test_eq("Encrypted Extensions", msgs.at("encrypted_extensions")[0], strip_message_header(vars.get_opt_bin("Message_EncryptedExtensions")));
               result.test_eq("Certificate", msgs.at("certificate")[0], strip_message_header(vars.get_opt_bin("Message_Server_Certificate")));
               result.test_eq("CertificateVerify", msgs.at("certificate_verify")[0], strip_message_header(vars.get_opt_bin("Message_Server_CertificateVerify")));

               result.test_eq("Server's entire first flight", ctx->pull_send_buffer(), concat(vars.get_req_bin("Record_ServerHello"),
                                                                                              vars.get_req_bin("Record_ServerHandshakeMessages")));

               result.confirm("Server can now send application data", ctx->server.is_active());
               }),

            Botan_Tests::CHECK("Send Client Finished", [&](Test::Result& result)
               {
               ctx->server.received_data(vars.get_req_bin("Record_ClientFinished"));

               ctx->check_callback_invocations(result, "client finished received", {
                  "tls_inspect_handshake_msg_finished",
                  "tls_session_activated"
                  });
               }),

            Botan_Tests::CHECK("Send Session Ticket", [&](Test::Result&)
               {
               // ctx->server.send_new_session_ticket(???);
               })
            };
         }

      // TODO: once session resumption is implemented this test trace should be added
      std::vector<Test::Result> resumed_handshake_with_0_rtt(const VarMap& /*vars*/) override { return {}; }

      std::vector<Test::Result> hello_retry_request(const VarMap& vars) override
         {
         // Fallback RNG is required to for blinding in ECDH with P-256
         auto& fallback_rng = Test::rng();
         auto rng = std::make_unique<Botan_Tests::Fixed_Output_RNG>(fallback_rng);

         // 32 - for server hello random
         // 32 - for KeyShare (eph. P-256 key pair)
         add_entropy(*rng, vars.get_req_bin("Server_RNG_Pool"));

         std::unique_ptr<Server_Context> ctx;

         return
            {
            Botan_Tests::CHECK("Receive Client Hello", [&](Test::Result& result)
               {
               auto add_cookie_and_sort = [&](Botan::TLS::Extensions& exts, Botan::TLS::Connection_Side side, Botan::TLS::Handshake_Type type)
                  {
                  if(type == Handshake_Type::HelloRetryRequest)
                     {
                     // This cookie needs to be mocked into the HRR since RFC 8448 contains it.
                     exts.add(new Cookie(vars.get_opt_bin("HelloRetryRequest_Cookie")));
                     }
                  sort_server_extensions(exts, side, type);
                  };

               ctx = std::make_unique<Server_Context>(std::move(rng), read_tls_policy("rfc8448_hrr_server"), vars.get_req_u64("CurrentTimestamp"), add_cookie_and_sort, make_mock_signatures(vars));
               result.confirm("server not closed", !ctx->server.is_closed());

               ctx->server.received_data(vars.get_req_bin("Record_ClientHello_1"));

               ctx->check_callback_invocations(result, "client hello received", {
                  "tls_emit_data",
                  "tls_examine_extensions_client_hello",
                  "tls_modify_extensions_hello_retry_request",
                  "tls_inspect_handshake_msg_client_hello",
                  "tls_inspect_handshake_msg_hello_retry_request"
                  });
               }),

            Botan_Tests::CHECK("Verify generated Hello Retry Request message", [&](Test::Result& result)
               {
               BOTAN_ASSERT_NONNULL(ctx);
               result.test_eq("Server's Hello Retry Request record", ctx->pull_send_buffer(), vars.get_req_bin("Record_HelloRetryRequest"));
               result.confirm("TLS handshake not yet finished", !ctx->server.is_active());
               }),

            Botan_Tests::CHECK("Receive updated Client Hello message", [&](Test::Result& result)
               {
               BOTAN_ASSERT_NONNULL(ctx);
               ctx->server.received_data(vars.get_req_bin("Record_ClientHello_2"));

               ctx->check_callback_invocations(result, "updated client hello received", {
                  "tls_emit_data",
                  "tls_examine_extensions_client_hello",
                  "tls_modify_extensions_server_hello",
                  "tls_modify_extensions_encrypted_extensions",
                  "tls_modify_extensions_certificate",
                  "tls_sign_message",
                  "tls_decode_group_param",
                  "tls_inspect_handshake_msg_client_hello",
                  "tls_inspect_handshake_msg_server_hello",
                  "tls_inspect_handshake_msg_encrypted_extensions",
                  "tls_inspect_handshake_msg_certificate",
                  "tls_inspect_handshake_msg_certificate_verify",
                  "tls_inspect_handshake_msg_finished"
                  });
               }),

            Botan_Tests::CHECK("Verify generated messages in server's second flight", [&](Test::Result& result)
               {
               BOTAN_ASSERT_NONNULL(ctx);
               const auto& msgs = ctx->observed_handshake_messages();

               result.test_eq("Server Hello", msgs.at("server_hello")[0], strip_message_header(vars.get_opt_bin("Message_ServerHello")));
               result.test_eq("Encrypted Extensions", msgs.at("encrypted_extensions")[0], strip_message_header(vars.get_opt_bin("Message_EncryptedExtensions")));
               result.test_eq("Certificate", msgs.at("certificate")[0], strip_message_header(vars.get_opt_bin("Message_Server_Certificate")));
               result.test_eq("CertificateVerify", msgs.at("certificate_verify")[0], strip_message_header(vars.get_opt_bin("Message_Server_CertificateVerify")));
               result.test_eq("Finished", msgs.at("finished")[0], strip_message_header(vars.get_opt_bin("Message_Server_Finished")));

               result.test_eq("Server's entire second flight", ctx->pull_send_buffer(), concat(vars.get_req_bin("Record_ServerHello"),
                                                                                               vars.get_req_bin("Record_ServerHandshakeMessages")));
               result.confirm("Server could now send application data", ctx->server.is_active());
               }),

            Botan_Tests::CHECK("Receive Client Finished", [&](Test::Result& result)
               {
               BOTAN_ASSERT_NONNULL(ctx);
               ctx->server.received_data(vars.get_req_bin("Record_ClientFinished"));

               ctx->check_callback_invocations(result, "client finished received", {
                  "tls_inspect_handshake_msg_finished",
                  "tls_session_activated"
                  });

               result.confirm("TLS handshake finished", ctx->server.is_active());
               }),

            Botan_Tests::CHECK("Receive Client close_notify", [&](Test::Result& result)
               {
               BOTAN_ASSERT_NONNULL(ctx);
               ctx->server.received_data(vars.get_req_bin("Record_Client_CloseNotify"));

               ctx->check_callback_invocations(result, "client finished received", {
                  "tls_alert",
                  "tls_peer_closed_connection"
                  });

               result.confirm("connection is not yet closed", !ctx->server.is_closed());
               result.confirm("connection is still active", ctx->server.is_active());
               }),

            Botan_Tests::CHECK("Expect Server close_notify", [&](Test::Result& result)
               {
               BOTAN_ASSERT_NONNULL(ctx);
               ctx->server.close();

               result.confirm("connection is now inactive", !ctx->server.is_active());
               result.confirm("connection is now closed", ctx->server.is_closed());
               result.test_eq("Server's close notify", ctx->pull_send_buffer(), vars.get_req_bin("Record_Server_CloseNotify"));
               }),

            };
         }

      std::vector<Test::Result> client_authentication(const VarMap& vars) override
         {
         auto rng = std::make_unique<Botan_Tests::Fixed_Output_RNG>("");

         // 32 - for server hello random
         // 32 - for KeyShare (eph. x25519 pair)
         add_entropy(*rng, vars.get_req_bin("Server_RNG_Pool"));

         std::unique_ptr<Server_Context> ctx;

         return
            {
            Botan_Tests::CHECK("Receive Client Hello", [&](Test::Result& result)
               {
               ctx = std::make_unique<Server_Context>(std::move(rng), read_tls_policy("rfc8448_client_auth_server"), vars.get_req_u64("CurrentTimestamp"), sort_server_extensions, make_mock_signatures(vars), true /* use alternative certificate */);
               result.confirm("server not closed", !ctx->server.is_closed());

               ctx->server.received_data(vars.get_req_bin("Record_ClientHello_1"));

               ctx->check_callback_invocations(result, "client hello received", {
                  "tls_emit_data",
                  "tls_examine_extensions_client_hello",
                  "tls_modify_extensions_server_hello",
                  "tls_modify_extensions_encrypted_extensions",
                  "tls_modify_extensions_certificate",
                  "tls_sign_message",
                  "tls_inspect_handshake_msg_client_hello",
                  "tls_inspect_handshake_msg_server_hello",
                  "tls_inspect_handshake_msg_encrypted_extensions",
                  "tls_inspect_handshake_msg_certificate_request",
                  "tls_inspect_handshake_msg_certificate",
                  "tls_inspect_handshake_msg_certificate_verify",
                  "tls_inspect_handshake_msg_finished"
                  });
               }),

            Botan_Tests::CHECK("Verify server's generated handshake messages", [&](Test::Result& result)
               {
               BOTAN_ASSERT_NONNULL(ctx);
               const auto& msgs = ctx->observed_handshake_messages();

               result.test_eq("Server Hello", msgs.at("server_hello")[0], strip_message_header(vars.get_opt_bin("Message_ServerHello")));
               result.test_eq("Encrypted Extensions", msgs.at("encrypted_extensions")[0], strip_message_header(vars.get_opt_bin("Message_EncryptedExtensions")));
               result.test_eq("Certificate Request", msgs.at("certificate_request")[0], strip_message_header(vars.get_opt_bin("Message_CertificateRequest")));
               result.test_eq("Certificate", msgs.at("certificate")[0], strip_message_header(vars.get_opt_bin("Message_Server_Certificate")));
               result.test_eq("CertificateVerify", msgs.at("certificate_verify")[0], strip_message_header(vars.get_opt_bin("Message_Server_CertificateVerify")));
               result.test_eq("Finished", msgs.at("finished")[0], strip_message_header(vars.get_opt_bin("Message_Server_Finished")));

               result.test_eq("Server's entire first flight", ctx->pull_send_buffer(), concat(vars.get_req_bin("Record_ServerHello"),
                                                                                              vars.get_req_bin("Record_ServerHandshakeMessages")));

               result.confirm("Not yet aware of client's cert chain", ctx->server.peer_cert_chain().empty());
               result.confirm("Server could now send application data", ctx->server.is_active());
               }),

            Botan_Tests::CHECK("Receive Client's second flight", [&](Test::Result& result)
               {
               BOTAN_ASSERT_NONNULL(ctx);
               // This encrypted message contains the following messages:
               // * client's Certificate message
               // * client's Certificate_Verify message
               // * client's Finished message
               ctx->server.received_data(vars.get_req_bin("Record_ClientFinished"));

               ctx->check_callback_invocations(result, "client finished received", {
                  "tls_inspect_handshake_msg_certificate",
                  "tls_inspect_handshake_msg_certificate_verify",
                  "tls_inspect_handshake_msg_finished",
                  "tls_examine_extensions_certificate",
                  "tls_verify_cert_chain",
                  "tls_verify_message",
                  "tls_session_activated"
                  });

               const auto cert_chain = ctx->server.peer_cert_chain();
               result.confirm("Received client's cert chain", !cert_chain.empty() && cert_chain.front() == client_certificate());

               result.confirm("TLS handshake finished", ctx->server.is_active());
               }),

            Botan_Tests::CHECK("Receive Client close_notify", [&](Test::Result& result)
               {
               BOTAN_ASSERT_NONNULL(ctx);
               ctx->server.received_data(vars.get_req_bin("Record_Client_CloseNotify"));

               ctx->check_callback_invocations(result, "client finished received", {
                  "tls_alert",
                  "tls_peer_closed_connection"
                  });

               result.confirm("connection is not yet closed", !ctx->server.is_closed());
               result.confirm("connection is still active", ctx->server.is_active());
               }),

            Botan_Tests::CHECK("Expect Server close_notify", [&](Test::Result& result)
               {
               BOTAN_ASSERT_NONNULL(ctx);
               ctx->server.close();

               result.confirm("connection is now inactive", !ctx->server.is_active());
               result.confirm("connection is now closed", ctx->server.is_closed());
               result.test_eq("Server's close notify", ctx->pull_send_buffer(), vars.get_req_bin("Record_Server_CloseNotify"));
               }),

            };
         }

      std::vector<Test::Result> middlebox_compatibility(const VarMap& vars) override
         {
         auto rng = std::make_unique<Botan_Tests::Fixed_Output_RNG>("");

         // 32 - for server hello random
         // 32 - for KeyShare (eph. x25519 pair)
         add_entropy(*rng, vars.get_req_bin("Server_RNG_Pool"));

         std::unique_ptr<Server_Context> ctx;

         return
            {
            Botan_Tests::CHECK("Receive Client Hello", [&](Test::Result& result)
               {
               ctx = std::make_unique<Server_Context>(std::move(rng), read_tls_policy("rfc8448_compat_server"), vars.get_req_u64("CurrentTimestamp"), sort_server_extensions, make_mock_signatures(vars));
               result.confirm("server not closed", !ctx->server.is_closed());

               ctx->server.received_data(vars.get_req_bin("Record_ClientHello_1"));

               ctx->check_callback_invocations(result, "client hello received", {
                  "tls_emit_data",
                  "tls_examine_extensions_client_hello",
                  "tls_modify_extensions_server_hello",
                  "tls_modify_extensions_encrypted_extensions",
                  "tls_modify_extensions_certificate",
                  "tls_sign_message",
                  "tls_inspect_handshake_msg_client_hello",
                  "tls_inspect_handshake_msg_server_hello",
                  "tls_inspect_handshake_msg_encrypted_extensions",
                  "tls_inspect_handshake_msg_certificate",
                  "tls_inspect_handshake_msg_certificate_verify",
                  "tls_inspect_handshake_msg_finished"
                  });
               }),

            Botan_Tests::CHECK("Verify server's generated handshake messages", [&](Test::Result& result)
               {
               BOTAN_ASSERT_NONNULL(ctx);
               const auto& msgs = ctx->observed_handshake_messages();

               result.test_eq("Server Hello", msgs.at("server_hello")[0], strip_message_header(vars.get_opt_bin("Message_ServerHello")));
               result.test_eq("Encrypted Extensions", msgs.at("encrypted_extensions")[0], strip_message_header(vars.get_opt_bin("Message_EncryptedExtensions")));
               result.test_eq("Certificate", msgs.at("certificate")[0], strip_message_header(vars.get_opt_bin("Message_Server_Certificate")));
               result.test_eq("CertificateVerify", msgs.at("certificate_verify")[0], strip_message_header(vars.get_opt_bin("Message_Server_CertificateVerify")));
               result.test_eq("Finished", msgs.at("finished")[0], strip_message_header(vars.get_opt_bin("Message_Server_Finished")));

               // Those records contain the required Change Cipher Spec message the server must produce for compatibility mode compliance
               result.test_eq("Server's entire first flight", ctx->pull_send_buffer(), concat(vars.get_req_bin("Record_ServerHello"),
                                                                                              vars.get_req_bin("Record_ServerHandshakeMessages")));

               result.confirm("Server could now send application data", ctx->server.is_active());
               }),

            Botan_Tests::CHECK("Receive Client Finished", [&](Test::Result& result)
               {
               BOTAN_ASSERT_NONNULL(ctx);
               ctx->server.received_data(vars.get_req_bin("Record_ClientFinished"));

               ctx->check_callback_invocations(result, "client finished received", {
                  "tls_inspect_handshake_msg_finished",
                  "tls_session_activated"
                  });

               result.confirm("TLS handshake finished", ctx->server.is_active());
               }),

            Botan_Tests::CHECK("Receive Client close_notify", [&](Test::Result& result)
               {
               BOTAN_ASSERT_NONNULL(ctx);
               ctx->server.received_data(vars.get_req_bin("Record_Client_CloseNotify"));

               ctx->check_callback_invocations(result, "client finished received", {
                  "tls_alert",
                  "tls_peer_closed_connection"
                  });

               result.confirm("connection is not yet closed", !ctx->server.is_closed());
               result.confirm("connection is still active", ctx->server.is_active());
               }),

            Botan_Tests::CHECK("Expect Server close_notify", [&](Test::Result& result)
               {
               BOTAN_ASSERT_NONNULL(ctx);
               ctx->server.close();

               result.confirm("connection is now inactive", !ctx->server.is_active());
               result.confirm("connection is now closed", ctx->server.is_closed());
               result.test_eq("Server's close notify", ctx->pull_send_buffer(), vars.get_req_bin("Record_Server_CloseNotify"));
               }),

            };
         }
   };

BOTAN_REGISTER_TEST("tls", "tls_rfc8448_client", Test_TLS_RFC8448_Client);
BOTAN_REGISTER_TEST("tls", "tls_rfc8448_server", Test_TLS_RFC8448_Server);

#endif

}
