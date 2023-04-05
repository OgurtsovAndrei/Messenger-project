/*
* PKCS #5 PBES2
* (C) 1999-2008,2014,2021 Jack Lloyd
* (C) 2018 Ribose Inc
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <botan/internal/pbes2.h>
#include <botan/cipher_mode.h>
#include <botan/pwdhash.h>
#include <botan/der_enc.h>
#include <botan/ber_dec.h>
#include <botan/internal/parsing.h>
#include <botan/asn1_obj.h>
#include <botan/oids.h>
#include <botan/rng.h>

namespace Botan {

namespace {

bool known_pbes_cipher_mode(const std::string& mode)
   {
   return (mode == "CBC" || mode == "GCM" || mode == "SIV");
   }

secure_vector<uint8_t> derive_key(const std::string& passphrase,
                                  const AlgorithmIdentifier& kdf_algo,
                                  size_t default_key_size)
   {
   if(kdf_algo.oid() == OID::from_string("PKCS5.PBKDF2"))
      {
      secure_vector<uint8_t> salt;
      size_t iterations = 0, key_length = 0;

      AlgorithmIdentifier prf_algo;
      BER_Decoder(kdf_algo.parameters())
         .start_sequence()
         .decode(salt, ASN1_Type::OctetString)
         .decode(iterations)
         .decode_optional(key_length, ASN1_Type::Integer, ASN1_Class::Universal)
         .decode_optional(prf_algo, ASN1_Type::Sequence, ASN1_Class::Constructed,
                          AlgorithmIdentifier("HMAC(SHA-1)",
                                              AlgorithmIdentifier::USE_NULL_PARAM))
         .end_cons();

      if(salt.size() < 8)
         throw Decoding_Error("PBE-PKCS5 v2.0: Encoded salt is too small");

      if(key_length == 0)
         key_length = default_key_size;

      const std::string prf = OIDS::oid2str_or_throw(prf_algo.oid());
      auto pbkdf_fam = PasswordHashFamily::create_or_throw("PBKDF2(" + prf + ")");
      auto pbkdf = pbkdf_fam->from_params(iterations);

      secure_vector<uint8_t> derived_key(key_length);
      pbkdf->derive_key(derived_key.data(), derived_key.size(),
                        passphrase.data(), passphrase.size(),
                        salt.data(), salt.size());
      return derived_key;
      }
   else if(kdf_algo.oid() == OID::from_string("Scrypt"))
      {
      secure_vector<uint8_t> salt;
      size_t N = 0, r = 0, p = 0;
      size_t key_length = 0;

      AlgorithmIdentifier prf_algo;
      BER_Decoder(kdf_algo.parameters())
         .start_sequence()
         .decode(salt, ASN1_Type::OctetString)
         .decode(N)
         .decode(r)
         .decode(p)
         .decode_optional(key_length, ASN1_Type::Integer, ASN1_Class::Universal)
         .end_cons();

      if(key_length == 0)
         key_length = default_key_size;

      secure_vector<uint8_t> derived_key(key_length);

      auto pwdhash_fam = PasswordHashFamily::create_or_throw("Scrypt");
      auto pwdhash = pwdhash_fam->from_params(N, r, p);
      pwdhash->derive_key(derived_key.data(), derived_key.size(),
                          passphrase.data(), passphrase.size(),
                          salt.data(), salt.size());

      return derived_key;
      }
   else
      throw Decoding_Error("PBE-PKCS5 v2.0: Unknown KDF algorithm " +
                           kdf_algo.oid().to_string());
   }

secure_vector<uint8_t> derive_key(const std::string& passphrase,
                                  const std::string& digest,
                                  RandomNumberGenerator& rng,
                                  size_t* msec_in_iterations_out,
                                  size_t iterations_if_msec_null,
                                  size_t key_length,
                                  AlgorithmIdentifier& kdf_algo)
   {
   const secure_vector<uint8_t> salt = rng.random_vec(12);

   if(digest == "Scrypt")
      {
      auto pwhash_fam = PasswordHashFamily::create_or_throw("Scrypt");

      std::unique_ptr<PasswordHash> pwhash;

      if(msec_in_iterations_out)
         {
         const std::chrono::milliseconds msec(*msec_in_iterations_out);
         pwhash = pwhash_fam->tune(key_length, msec);
         }
      else
         {
         pwhash = pwhash_fam->from_iterations(iterations_if_msec_null);
         }

      secure_vector<uint8_t> key(key_length);
      pwhash->derive_key(key.data(), key.size(),
                         passphrase.c_str(), passphrase.size(),
                         salt.data(), salt.size());

      const size_t N = pwhash->memory_param();
      const size_t r = pwhash->iterations();
      const size_t p = pwhash->parallelism();

      if(msec_in_iterations_out)
         *msec_in_iterations_out = 0;

      std::vector<uint8_t> scrypt_params;
      DER_Encoder(scrypt_params)
         .start_sequence()
            .encode(salt, ASN1_Type::OctetString)
            .encode(N)
            .encode(r)
            .encode(p)
            .encode(key_length)
         .end_cons();

      kdf_algo = AlgorithmIdentifier(OID::from_string("Scrypt"), scrypt_params);
      return key;
      }
   else
      {
      const std::string prf = "HMAC(" + digest + ")";
      const std::string pbkdf_name = "PBKDF2(" + prf + ")";

      std::unique_ptr<PasswordHashFamily> pwhash_fam = PasswordHashFamily::create(pbkdf_name);
      if(!pwhash_fam)
         throw Invalid_Argument("Unknown password hash digest " + digest);

      std::unique_ptr<PasswordHash> pwhash;

      if(msec_in_iterations_out)
         {
         const std::chrono::milliseconds msec(*msec_in_iterations_out);
         pwhash = pwhash_fam->tune(key_length, msec);
         }
      else
         {
         pwhash = pwhash_fam->from_iterations(iterations_if_msec_null);
         }

      secure_vector<uint8_t> key(key_length);
      pwhash->derive_key(key.data(), key.size(),
                         passphrase.c_str(), passphrase.size(),
                         salt.data(), salt.size());

      std::vector<uint8_t> pbkdf2_params;

      const size_t iterations = pwhash->iterations();

      if(msec_in_iterations_out)
         *msec_in_iterations_out = iterations;

      DER_Encoder(pbkdf2_params)
         .start_sequence()
            .encode(salt, ASN1_Type::OctetString)
            .encode(iterations)
            .encode(key_length)
            .encode_if(prf != "HMAC(SHA-1)",
                       AlgorithmIdentifier(prf, AlgorithmIdentifier::USE_NULL_PARAM))
         .end_cons();

      kdf_algo = AlgorithmIdentifier("PKCS5.PBKDF2", pbkdf2_params);
      return key;
      }
   }

/*
* PKCS#5 v2.0 PBE Encryption
*/
std::pair<AlgorithmIdentifier, std::vector<uint8_t>>
pbes2_encrypt_shared(const secure_vector<uint8_t>& key_bits,
                     const std::string& passphrase,
                     size_t* msec_in_iterations_out,
                     size_t iterations_if_msec_null,
                     const std::string& cipher,
                     const std::string& prf,
                     RandomNumberGenerator& rng)
   {
   const std::vector<std::string> cipher_spec = split_on(cipher, '/');
   if(cipher_spec.size() != 2)
      throw Encoding_Error("PBE-PKCS5 v2.0: Invalid cipher spec " + cipher);

   if(!known_pbes_cipher_mode(cipher_spec[1]))
      throw Encoding_Error("PBE-PKCS5 v2.0: Don't know param format for " + cipher);

   const OID cipher_oid = OIDS::str2oid_or_empty(cipher);
   if(cipher_oid.empty())
      throw Encoding_Error("PBE-PKCS5 v2.0: No OID assigned for " + cipher);

   std::unique_ptr<Cipher_Mode> enc = Cipher_Mode::create(cipher, Cipher_Dir::Encryption);

   if(!enc)
      throw Decoding_Error("PBE-PKCS5 cannot encrypt no cipher " + cipher);

   const size_t key_length = enc->key_spec().maximum_keylength();

   const secure_vector<uint8_t> iv = rng.random_vec(enc->default_nonce_length());

   AlgorithmIdentifier kdf_algo;

   const secure_vector<uint8_t> derived_key =
      derive_key(passphrase, prf, rng,
                 msec_in_iterations_out, iterations_if_msec_null,
                 key_length, kdf_algo);

   enc->set_key(derived_key);
   enc->start(iv);
   secure_vector<uint8_t> ctext = key_bits;
   enc->finish(ctext);

   std::vector<uint8_t> encoded_iv;
   DER_Encoder(encoded_iv).encode(iv, ASN1_Type::OctetString);

   std::vector<uint8_t> pbes2_params;
   DER_Encoder(pbes2_params)
      .start_sequence()
      .encode(kdf_algo)
      .encode(AlgorithmIdentifier(cipher, encoded_iv))
      .end_cons();

   AlgorithmIdentifier id(OID::from_string("PBE-PKCS5v20"), pbes2_params);

   return std::make_pair(id, unlock(ctext));
   }

}

std::pair<AlgorithmIdentifier, std::vector<uint8_t>>
pbes2_encrypt(const secure_vector<uint8_t>& key_bits,
              const std::string& passphrase,
              std::chrono::milliseconds msec,
              const std::string& cipher,
              const std::string& digest,
              RandomNumberGenerator& rng)
   {
   size_t msec_in_iterations_out = static_cast<size_t>(msec.count());
   return pbes2_encrypt_shared(key_bits, passphrase, &msec_in_iterations_out, 0, cipher, digest, rng);
   // return value msec_in_iterations_out discarded
   }

std::pair<AlgorithmIdentifier, std::vector<uint8_t>>
pbes2_encrypt_msec(const secure_vector<uint8_t>& key_bits,
                   const std::string& passphrase,
                   std::chrono::milliseconds msec,
                   size_t* out_iterations_if_nonnull,
                   const std::string& cipher,
                   const std::string& digest,
                   RandomNumberGenerator& rng)
   {
   size_t msec_in_iterations_out = static_cast<size_t>(msec.count());

   auto ret = pbes2_encrypt_shared(key_bits, passphrase, &msec_in_iterations_out, 0, cipher, digest, rng);

   if(out_iterations_if_nonnull)
      *out_iterations_if_nonnull = msec_in_iterations_out;

   return ret;
   }

std::pair<AlgorithmIdentifier, std::vector<uint8_t>>
pbes2_encrypt_iter(const secure_vector<uint8_t>& key_bits,
                   const std::string& passphrase,
                   size_t pbkdf_iter,
                   const std::string& cipher,
                   const std::string& digest,
                   RandomNumberGenerator& rng)
   {
   return pbes2_encrypt_shared(key_bits, passphrase, nullptr, pbkdf_iter, cipher, digest, rng);
   }

secure_vector<uint8_t>
pbes2_decrypt(const secure_vector<uint8_t>& key_bits,
              const std::string& passphrase,
              const std::vector<uint8_t>& params)
   {
   AlgorithmIdentifier kdf_algo, enc_algo;

   BER_Decoder(params)
      .start_sequence()
         .decode(kdf_algo)
         .decode(enc_algo)
      .end_cons();

   const std::string cipher = OIDS::oid2str_or_throw(enc_algo.oid());
   const std::vector<std::string> cipher_spec = split_on(cipher, '/');
   if(cipher_spec.size() != 2)
      throw Decoding_Error("PBE-PKCS5 v2.0: Invalid cipher spec " + cipher);
   if(!known_pbes_cipher_mode(cipher_spec[1]))
      throw Decoding_Error("PBE-PKCS5 v2.0: Don't know param format for " + cipher);

   secure_vector<uint8_t> iv;
   BER_Decoder(enc_algo.parameters()).decode(iv, ASN1_Type::OctetString).verify_end();

   std::unique_ptr<Cipher_Mode> dec = Cipher_Mode::create(cipher, Cipher_Dir::Decryption);
   if(!dec)
      throw Decoding_Error("PBE-PKCS5 cannot decrypt no cipher " + cipher);

   dec->set_key(derive_key(passphrase, kdf_algo, dec->key_spec().maximum_keylength()));

   dec->start(iv);

   secure_vector<uint8_t> buf = key_bits;
   dec->finish(buf);

   return buf;
   }

}
