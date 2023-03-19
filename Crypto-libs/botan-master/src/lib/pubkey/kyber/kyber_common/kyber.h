/*
 * Crystals Kyber key encapsulation mechanism
 * Based on the public domain reference implementation by the
 * designers (https://github.com/pq-crystals/kyber)
 *
 * Further changes
 * (C) 2021-2022 Jack Lloyd
 * (C) 2021-2022 Manuel Glaser and Michael Boric, Rohde & Schwarz Cybersecurity
 * (C) 2021-2022 René Meusel and Hannes Rantzsch, neXenio GmbH
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 */

#ifndef BOTAN_KYBER_COMMON_H_
#define BOTAN_KYBER_COMMON_H_

#include <botan/asn1_obj.h>
#include <botan/der_enc.h>
#include <botan/exceptn.h>
#include <botan/pk_keys.h>

#if !defined(BOTAN_HAS_KYBER_90S) && !defined(BOTAN_HAS_KYBER)
   static_assert(false, "botan module 'kyber_common' is useful only when enabling modules 'kyber', 'kyber_90s' or both");
#endif

namespace Botan {

class BOTAN_PUBLIC_API(3, 0) KyberMode
   {
   public:
      enum Mode
         {
         Kyber512,
         Kyber512_90s,
         Kyber768,
         Kyber768_90s,
         Kyber1024,
         Kyber1024_90s
         };

      KyberMode(Mode mode);
      explicit KyberMode(const OID& oid);
      explicit KyberMode(const std::string& str);

      OID object_identifier() const;
      std::string to_string() const;

      Mode mode() const { return m_mode; }
      bool is_90s() const { return m_mode == Kyber512_90s || m_mode == Kyber768_90s || m_mode == Kyber1024_90s; }
      bool is_modern() const { return !is_90s(); }

      bool operator==(const KyberMode& other) const { return m_mode == other.m_mode; }
      bool operator!=(const KyberMode& other) const { return !(*this == other); }

   private:
      Mode m_mode;
   };

enum class KyberKeyEncoding
   {
   Raw, // as implemented in the reference implementation
   Full // as described in draft-uni-qsckeys-00 Section 4.3 (private key), 4.5 (public key)
   };

class Kyber_PublicKeyInternal;
class Kyber_PrivateKeyInternal;

class BOTAN_PUBLIC_API(3, 0) Kyber_PublicKey : public virtual Public_Key
   {
   public:
      Kyber_PublicKey(const std::vector<uint8_t>& pub_key,
                      KyberMode mode,
                      KyberKeyEncoding encoding);

      Kyber_PublicKey(const AlgorithmIdentifier& alg_id,
                      const std::vector<uint8_t>& key_bits);

      Kyber_PublicKey(const Kyber_PublicKey& other);

      Kyber_PublicKey& operator=(const Kyber_PublicKey& other) = default;

      virtual ~Kyber_PublicKey() = default;

      std::string algo_name() const override;

      AlgorithmIdentifier algorithm_identifier() const override;

      OID object_identifier() const override;

      size_t key_length() const override;

      size_t estimated_strength() const override;

      std::vector<uint8_t> public_key_bits() const override;

      bool check_key(RandomNumberGenerator&, bool) const override;

      std::unique_ptr<PK_Ops::KEM_Encryption> create_kem_encryption_op(RandomNumberGenerator& rng,
            const std::string& params,
            const std::string& provider) const override;

      KyberMode mode() const;

      void set_binary_encoding(KyberKeyEncoding encoding)
         {
         m_key_encoding = encoding;
         }
      KyberKeyEncoding binary_encoding() const
         {
         return m_key_encoding;
         }

   protected:
      Kyber_PublicKey() {}

      void initialize_from_encoding(const std::vector<uint8_t>& pub_key,
                                    KyberMode m,
                                    KyberKeyEncoding encoding);

      std::vector<uint8_t> public_key_bits_raw() const;
      std::vector<uint8_t> public_key_bits_der() const;

   protected:
      friend class Kyber_KEM_Encryptor;
      friend class Kyber_KEM_Decryptor;

      std::shared_ptr<Kyber_PublicKeyInternal> m_public;
      KyberKeyEncoding m_key_encoding = KyberKeyEncoding::Full;
   };

class BOTAN_PUBLIC_API(3, 0) Kyber_PrivateKey final : public virtual Kyber_PublicKey, public virtual Private_Key
   {
   public:
      Kyber_PrivateKey(RandomNumberGenerator& rng, KyberMode mode);

      Kyber_PrivateKey(const secure_vector<uint8_t>& sk,
                       KyberMode mode,
                       KyberKeyEncoding encoding);

      Kyber_PrivateKey(const AlgorithmIdentifier& alg_id,
                       const secure_vector<uint8_t>& key_bits);

      std::unique_ptr<Public_Key> public_key() const override;

      secure_vector<uint8_t> private_key_bits() const override;

      std::unique_ptr<PK_Ops::KEM_Decryption> create_kem_decryption_op(RandomNumberGenerator& rng,
            const std::string& params,
            const std::string& provider) const override;

   private:
      secure_vector<uint8_t> private_key_bits_raw() const;
      secure_vector<uint8_t> private_key_bits_der() const;

   private:
      friend class Kyber_KEM_Decryptor;

      std::shared_ptr<Kyber_PrivateKeyInternal> m_private;
   };

} // namespace Botan

#endif
