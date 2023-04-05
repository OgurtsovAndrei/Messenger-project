#include <botan/auto_rng.h>
#include <botan/certstor.h>
#include <botan/tls_callbacks.h>
#include <botan/tls_client.h>
#include <botan/tls_policy.h>
#include <botan/tls_session_manager_memory.h>
#include <botan/ec_group.h>
#include <botan/oids.h>

/**
 * @brief Callbacks invoked by TLS::Channel.
 *
 * Botan::TLS::Callbacks is an abstract class.
 * For improved readability, only the functions that are mandatory
 * to implement are listed here. See src/lib/tls/tls_callbacks.h.
 */
class Callbacks : public Botan::TLS::Callbacks {
public:
  void tls_emit_data(const uint8_t data[], size_t size) override {
    // send data to tls server, e.g., using BSD sockets or boost asio
  }

  void tls_record_received(uint64_t seq_no, const uint8_t data[], size_t size) override {
    // process full TLS record received by tls server, e.g.,
    // by passing it to the application
  }

  void tls_alert(Botan::TLS::Alert alert) override {
    // handle a tls alert received from the tls server
  }

  bool tls_session_established(const Botan::TLS::Session_with_Handle &session) override {
    // the session with the tls server was established
    // return false to prevent the session from being cached, true to
    // cache the session in the configured session manager
    return false;
  }
  std::string tls_decode_group_param(Botan::TLS::Group_Params group_param) override {
    // handle TLS group identifier decoding and return name as string
    // return empty string to indicate decoding failure

    switch (static_cast<uint16_t>(group_param)) {
    case 0xFE00:
      return "testcurve1102";
    default:
      // decode non-custom groups
      return Botan::TLS::Callbacks::tls_decode_group_param(group_param);
    }
  }
};

/**
 * @brief Credentials storage for the tls client.
 *
 * It returns a list of trusted CA certificates from a local directory.
 * TLS client authentication is disabled. See src/lib/tls/credentials_manager.h.
 */
class Client_Credentials : public Botan::Credentials_Manager {
public:
  std::vector<Botan::Certificate_Store *>
  trusted_certificate_authorities(const std::string &type, const std::string &context) override {
    // return a list of certificates of CAs we trust for tls server certificates,
    // e.g., all the certificates in the local directory "cas"
    return {&m_cert_store};
  }

  std::vector<Botan::X509_Certificate> cert_chain(const std::vector<std::string> &cert_key_types,
                                                  const std::vector<Botan::AlgorithmIdentifier> &cert_signature_schemes,
                                                  const std::string &type,
                                                  const std::string &context) override {
    // when using tls client authentication (optional), return
    // a certificate chain being sent to the tls server,
    // else an empty list
    return {};
  }

  std::shared_ptr<Botan::Private_Key>
  private_key_for(const Botan::X509_Certificate &cert, const std::string &type,
                  const std::string &context) override {
    // when returning a chain in cert_chain(), return the private key
    // associated with the leaf certificate here
    return nullptr;
  }

private:
  Botan::Certificate_Store_In_Memory m_cert_store{"cas"};
};

class Client_Policy : public Botan::TLS::Strict_Policy {
public:
  std::vector<Botan::TLS::Group_Params> key_exchange_groups() const override {
    // modified strict policy to allow our custom curves
    return {static_cast<Botan::TLS::Group_Params>(0xFE00)};
  }
};

int main() {
  // prepare rng
  Botan::AutoSeeded_RNG rng;

  // prepare custom curve

  // prepare curve parameters
  const Botan::BigInt p(
      "0x92309a3e88b94312f36891a2055725bb35ab51af96b3a651d39321b7bbb8c51575a76768c9b6b323");
  const Botan::BigInt a(
      "0x4f30b8e311f6b2dce62078d70b35dacb96aa84b758ab5a8dff0c9f7a2a1ff466c19988aa0acdde69");
  const Botan::BigInt b(
      "0x9045A513CFFF9AE1F1CC84039D852D240344A1D5C9DB203C844089F855C387823EB6FCDDF49C909C");

  const Botan::BigInt x(
      "0x9120f3779a31296cefcb5a5a08831f1a6d438ad5a3f2ce60585ac19c74eebdc65cadb96bb92622c7");
  const Botan::BigInt y(
      "0x836db8251c152dfee071b72c6b06c5387d82f1b5c30c5a5b65ee9429aa2687e8426d5d61276a4ede");
  const Botan::BigInt order(
      "0x248c268fa22e50c4bcda24688155c96ecd6ad46be5c82d7a6be6e7068cb5d1ca72b2e07e8b90d853");

  const Botan::BigInt cofactor(4);

  const Botan::OID oid("1.2.3.1");

  // create EC_Group object to register the curve
  Botan::EC_Group testcurve1102(p, a, b, x, y, order, cofactor, oid);

  if (!testcurve1102.verify_group(rng)) {
    // Warning: if verify_group returns false the curve parameters are insecure
  }

  // register name to specified oid
  Botan::OIDS::add_oid(oid, "testcurve1102");

  // prepare all the parameters
  Callbacks callbacks;
  Botan::TLS::Session_Manager_In_Memory session_mgr(rng);
  Client_Credentials creds;
  Client_Policy policy;

  // open the tls connection
  Botan::TLS::Client client(callbacks, session_mgr, creds, policy, rng,
                            Botan::TLS::Server_Information("botan.randombit.net", 443),
                            Botan::TLS::Protocol_Version::TLS_V12);

  while (!client.is_closed()) {
    // read data received from the tls server, e.g., using BSD sockets or boost asio
    // ...

    // send data to the tls server using client.send()
  }

  return 0;
}
