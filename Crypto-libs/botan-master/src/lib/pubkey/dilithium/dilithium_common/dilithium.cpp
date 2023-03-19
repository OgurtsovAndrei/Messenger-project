/*
* Crystals Dilithium Digital Signature Algorithms
* Based on the public domain reference implementation by the
* designers (https://github.com/pq-crystals/dilithium)
*
* Further changes
* (C) 2021-2023 Jack Lloyd
* (C) 2021-2022 Manuel Glaser - Rohde & Schwarz Cybersecurity
* (C) 2021-2023 Michael Boric, René Meusel - Rohde & Schwarz Cybersecurity
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <botan/dilithium.h>

#include <botan/ber_dec.h>
#include <botan/der_enc.h>
#include <botan/hash.h>
#include <botan/rng.h>
#include <botan/exceptn.h>

#include <botan/internal/dilithium_polynomials.h>
#include <botan/internal/shake.h>
#include <botan/internal/pk_ops_impl.h>
#include <botan/internal/stl_util.h>
#include <botan/internal/parsing.h>

#include <algorithm>
#include <iterator>
#include <array>
#include <vector>
#include <span>

namespace Botan {
namespace {

/**
 * Helper class to ease unmarshalling of concatenated fixed-length values
 */
class BufferSlicer final
   {
   public:
      BufferSlicer(std::span<const uint8_t> buffer) : m_remaining(buffer)
         {}

      template <typename ContainerT>
      auto take_as(const size_t count)
         {
         const auto result = take(count);
         return ContainerT(result.begin(), result.end());
         }

      auto take_vector(const size_t count) { return take_as<std::vector<uint8_t>>(count); }
      auto take_secure_vector(const size_t count) { return take_as<secure_vector<uint8_t>>(count); }

      std::span<const uint8_t> take(const size_t count)
         {
         BOTAN_STATE_CHECK(remaining() >= count);
         auto result = m_remaining.first(count);
         m_remaining = m_remaining.subspan(count);
         return result;
         }

      void copy_into(std::span<uint8_t> sink)
         {
         const auto data = take(sink.size());
         std::copy(data.begin(), data.end(), sink.begin());
         }

      size_t remaining() const { return m_remaining.size(); }

   private:
      std::span<const uint8_t> m_remaining;
   };

std::pair<Dilithium::PolynomialVector, Dilithium::PolynomialVector>
calculate_t0_and_t1(const DilithiumModeConstants& mode,
                     const std::vector<uint8_t>& rho,
                     Dilithium::PolynomialVector s1,
                     const Dilithium::PolynomialVector& s2)
   {
   /* Generate matrix */
   auto matrix = Dilithium::PolynomialMatrix::generate_matrix(rho, mode);

   /* Matrix-vector multiplication */
   s1.ntt();
   auto t = Dilithium::PolynomialVector::generate_polyvec_matrix_pointwise_montgomery(matrix.get_matrix(), s1, mode);
   t.reduce();
   t.invntt_tomont();

   /* Add error vector s2 */
   t.add_polyvec(s2);

   /* Extract t and write public key */
   t.cadd_q();

   Dilithium::PolynomialVector t0(mode.k());
   Dilithium::PolynomialVector t1(mode.k());
   Dilithium::PolynomialVector::fill_polyvecs_power2round(t1, t0, t);

   return { std::move(t0), std::move(t1) };
   }

DilithiumMode::Mode dilithium_mode_from_string(const std::string& str)
   {
   if(str == "Dilithium-4x4-r3")
      { return DilithiumMode::Dilithium4x4; }
   if(str == "Dilithium-4x4-AES-r3")
      { return DilithiumMode::Dilithium4x4_AES; }
   if(str == "Dilithium-6x5-r3")
      { return DilithiumMode::Dilithium6x5; }
   if(str == "Dilithium-6x5-AES-r3")
      { return DilithiumMode::Dilithium6x5_AES; }
   if(str == "Dilithium-8x7-r3")
      { return DilithiumMode::Dilithium8x7; }
   if(str == "Dilithium-8x7-AES-r3")
      { return DilithiumMode::Dilithium8x7_AES; }

   throw Invalid_Argument(str + " is not a valid Dilithium mode name");
   }

}

DilithiumMode::DilithiumMode(const OID& oid)
   : m_mode(dilithium_mode_from_string(oid.to_formatted_string())) {}

DilithiumMode::DilithiumMode(const std::string& str)
   : m_mode(dilithium_mode_from_string(str)) {}

OID DilithiumMode::object_identifier() const { return OID::from_string(to_string()); }

std::string DilithiumMode::to_string() const
   {
   switch(m_mode)
      {
      case DilithiumMode::Dilithium4x4:
         return "Dilithium-4x4-r3";
      case DilithiumMode::Dilithium4x4_AES:
         return "Dilithium-4x4-AES-r3";
      case DilithiumMode::Dilithium6x5:
         return "Dilithium-6x5-r3";
      case DilithiumMode::Dilithium6x5_AES:
         return "Dilithium-6x5-AES-r3";
      case DilithiumMode::Dilithium8x7:
         return "Dilithium-8x7-r3";
      case DilithiumMode::Dilithium8x7_AES:
         return "Dilithium-8x7-AES-r3";
      }

   unreachable();
   }

class Dilithium_PublicKeyInternal : public ASN1_Object
   {
   public:
      Dilithium_PublicKeyInternal(DilithiumModeConstants mode)
         : m_mode(std::move(mode)) {}

      Dilithium_PublicKeyInternal(DilithiumModeConstants mode, const std::vector<uint8_t>& raw_pk)
         : m_mode(std::move(mode))
         {
         BOTAN_ASSERT_NOMSG(raw_pk.size() == m_mode.public_key_bytes());

         BufferSlicer s(raw_pk);
         m_rho = s.take_vector(DilithiumModeConstants::SEEDBYTES);
         m_t1 = Dilithium::PolynomialVector::unpack_t1(s.take(DilithiumModeConstants::POLYT1_PACKEDBYTES * m_mode.k()), m_mode);

         BOTAN_ASSERT_NOMSG(s.remaining() == 0);
         BOTAN_STATE_CHECK(m_t1.m_vec.size() == m_mode.k());
         }

      Dilithium_PublicKeyInternal(DilithiumModeConstants mode, std::vector<uint8_t> rho, const Dilithium::PolynomialVector& s1, const Dilithium::PolynomialVector& s2)
         : m_mode(std::move(mode)), m_rho(std::move(rho)), m_t1([&] { return calculate_t0_and_t1(m_mode, m_rho, s1, s2).second; }())
         {
         BOTAN_ASSERT_NOMSG(!m_rho.empty());
         BOTAN_ASSERT_NOMSG(!m_t1.m_vec.empty());
         }

      Dilithium_PublicKeyInternal(DilithiumModeConstants mode, std::vector<uint8_t> rho, Dilithium::PolynomialVector t1)
         : m_mode(std::move(mode)), m_rho(std::move(rho)), m_t1(std::move(t1))
         {
         BOTAN_ASSERT_NOMSG(!m_rho.empty());
         BOTAN_ASSERT_NOMSG(!m_t1.m_vec.empty());
         }

      Dilithium_PublicKeyInternal(const Dilithium_PublicKeyInternal&) = delete;
      Dilithium_PublicKeyInternal(Dilithium_PublicKeyInternal&&) = delete;
      Dilithium_PublicKeyInternal& operator=(const Dilithium_PublicKeyInternal& other) = delete;
      Dilithium_PublicKeyInternal& operator=(Dilithium_PublicKeyInternal&& other) = delete;

      ~Dilithium_PublicKeyInternal() override = default;

      void encode_into(DER_Encoder& to) const override
         {
         // This encoding is based on draft-uni-qsckeys-dilithium-00 Section 3.6
         // https://datatracker.ietf.org/doc/draft-uni-qsckeys-dilithium/00/
         to.start_sequence()
               .encode(m_rho, ASN1_Type::OctetString)
               .encode(m_t1.polyvec_pack_t1(), ASN1_Type::OctetString)
            .end_cons();
         }

      void decode_from(BER_Decoder& from) override
         {
         std::vector<uint8_t> t1;

         from.start_sequence()
               .decode(m_rho, ASN1_Type::OctetString)
               .decode(t1, ASN1_Type::OctetString)
            .end_cons();

         m_t1 = Dilithium::PolynomialVector::unpack_t1(t1, m_mode);
         }

      std::vector<uint8_t> raw_pk() const
         { return concat_as<std::vector<uint8_t>>(m_rho, m_t1.polyvec_pack_t1()); }

      const Dilithium::PolynomialVector& t1() const { return m_t1; }
      const std::vector<uint8_t>& rho() const { return m_rho; }
      const DilithiumModeConstants& mode() const { return m_mode; }

   private:
      const DilithiumModeConstants m_mode;
      std::vector<uint8_t> m_rho;
      Dilithium::PolynomialVector m_t1;
   };

class Dilithium_PrivateKeyInternal : public ASN1_Object
   {
   public:
      Dilithium_PrivateKeyInternal(DilithiumModeConstants mode)
         : m_mode(std::move(mode)) {}

      Dilithium_PrivateKeyInternal(DilithiumModeConstants mode,
                                   std::vector<uint8_t> rho,
                                   secure_vector<uint8_t> tr,
                                   secure_vector<uint8_t> key,
                                   Dilithium::PolynomialVector s1,
                                   Dilithium::PolynomialVector s2,
                                   Dilithium::PolynomialVector t0) :
         m_mode(std::move(mode)),
         m_rho(std::move(rho)),
         m_tr(std::move(tr)),
         m_key(std::move(key)),
         m_t0(std::move(t0)),
         m_s1(std::move(s1)),
         m_s2(std::move(s2)) {}

      Dilithium_PrivateKeyInternal(DilithiumModeConstants mode, const secure_vector<uint8_t>& sk) :
         Dilithium_PrivateKeyInternal(std::move(mode))
         {
         BOTAN_ASSERT_NOMSG(sk.size() == m_mode.private_key_bytes());

         BufferSlicer s(sk);
         m_rho = s.take_vector(DilithiumModeConstants::SEEDBYTES);
         m_key = s.take_secure_vector(DilithiumModeConstants::SEEDBYTES);
         m_tr  = s.take_secure_vector(DilithiumModeConstants::SEEDBYTES);
         m_s1  = Dilithium::PolynomialVector::unpack_eta(s.take(m_mode.l() * m_mode.polyeta_packedbytes()), m_mode.l(), m_mode);
         m_s2  = Dilithium::PolynomialVector::unpack_eta(s.take(m_mode.k() * m_mode.polyeta_packedbytes()), m_mode.k(), m_mode);
         m_t0  = Dilithium::PolynomialVector::unpack_t0(s.take(m_mode.k() * DilithiumModeConstants::POLYT0_PACKEDBYTES), m_mode);
         }

      void encode_into(DER_Encoder& to) const override
         {
         // This encoding is based on draft-uni-qsckeys-dilithium-00 Section 3.3
         // https://datatracker.ietf.org/doc/draft-uni-qsckeys-dilithium/00/
         //
         // Currently, we support the full encoding only. I.e. Sections 3.4 and 3.5
         // are _not_ implemented. Neither for encoding nor for decoding.
         to.start_sequence()
               .encode(size_t(0) /* version info: NIST round 3 */)
               .encode(m_rho, ASN1_Type::BitString)
               .encode(m_key, ASN1_Type::BitString)
               .encode(m_tr, ASN1_Type::BitString)
               .encode(m_s1.polyvec_pack_eta(m_mode), ASN1_Type::BitString)
               .encode(m_s2.polyvec_pack_eta(m_mode), ASN1_Type::BitString)
               .encode(m_t0.polyvec_pack_t0(), ASN1_Type::BitString)
               // we don't encode the public key into the private key
            .end_cons();
         }

      void decode_from(BER_Decoder& from) override
         {
         secure_vector<uint8_t> s1, s2, t0;

         from.start_sequence()
               .decode_and_check(size_t(0) /* Dilithium-r3 */, "Unsupported Dilithium encoding version")
               .decode(m_rho, ASN1_Type::BitString)
               .decode(m_key, ASN1_Type::BitString)
               .decode(m_tr, ASN1_Type::BitString)
               .decode(s1, ASN1_Type::BitString)
               .decode(s2, ASN1_Type::BitString)
               .decode(t0, ASN1_Type::BitString)
               // here might be the optional public key -- ignore for now
            .end_cons();

         if(m_rho.size() != DilithiumModeConstants::SEEDBYTES)
            { throw Decoding_Error("rho has unexpected length"); }
         if(m_key.size() != DilithiumModeConstants::SEEDBYTES)
            { throw Decoding_Error("key has unexpected length"); }
         if(m_tr.size() != DilithiumModeConstants::SEEDBYTES)
            { throw Decoding_Error("tr has unexpected length"); }

         m_s1 = Dilithium::PolynomialVector::unpack_eta(s1, m_mode.l(), m_mode);
         m_s2 = Dilithium::PolynomialVector::unpack_eta(s2, m_mode.k(), m_mode);
         m_t0 = Dilithium::PolynomialVector::unpack_t0(t0, m_mode);
         }

      secure_vector<uint8_t> raw_sk() const
         {
         return concat_as<secure_vector<uint8_t>>(
                   m_rho, m_key, m_tr, m_s1.polyvec_pack_eta(m_mode),
                   m_s2.polyvec_pack_eta(m_mode), m_t0.polyvec_pack_t0());
         }

      const DilithiumModeConstants& mode() const { return m_mode; }
      const std::vector<uint8_t>& rho() const { return m_rho; }
      const secure_vector<uint8_t>& get_key() const { return m_key; }
      const secure_vector<uint8_t>& tr() const { return m_tr; }
      const Dilithium::PolynomialVector& s1() const { return m_s1; }
      const Dilithium::PolynomialVector& s2() const { return m_s2; }
      const Dilithium::PolynomialVector& t0() const { return m_t0; }

   private:
      const DilithiumModeConstants m_mode;
      std::vector<uint8_t> m_rho;
      secure_vector<uint8_t> m_tr, m_key;
      Dilithium::PolynomialVector m_t0, m_s1, m_s2;
   };

class Dilithium_Signature_Operation final : public PK_Ops::Signature
   {
   public:
      Dilithium_Signature_Operation(const Dilithium_PrivateKey& priv_key_dilithium, bool randomized) :
         m_priv_key(priv_key_dilithium),
         m_shake(DilithiumModeConstants::CRHBYTES * 8),
         m_randomized(randomized)
         {
         m_shake.update(m_priv_key.m_private->tr());
         }

      void update(const uint8_t msg[], size_t msg_len) override
         {
         m_shake.update(msg, msg_len);
         }

      secure_vector<uint8_t> sign(RandomNumberGenerator& rng) override
         {
         const auto& mode = m_priv_key.m_private->mode();
         const auto mu = m_shake.final_stdvec();

         // Get set up for the next message (if any)
         m_shake.update(m_priv_key.m_private->tr());

         const auto rhoprime = (m_randomized)
	         ? rng.random_vec(DilithiumModeConstants::CRHBYTES)
	         : mode.CRH(concat(m_priv_key.m_private->get_key(), mu));

         auto s1 = m_priv_key.m_private->s1();
         /* Expand matrix and transform vectors */
         auto matrix = Dilithium::PolynomialMatrix::generate_matrix(m_priv_key.m_private->rho(), mode);
         s1.ntt();

         auto s2 = m_priv_key.m_private->s2();
         s2.ntt();

         auto t0 = m_priv_key.m_private->t0();
         t0.ntt();

         // Note: nonce (as requested by `polyvecl_uniform_gamma1`) is actually just uint16_t
         //       but to avoid an integer overflow, we use uint32_t as the loop variable.
         for(uint32_t nonce = 0; nonce <= std::numeric_limits<uint16_t>::max(); ++nonce)
            {
            /* Sample intermediate vector y */
            Dilithium::PolynomialVector y(mode.l());

            y.polyvecl_uniform_gamma1(rhoprime, static_cast<uint16_t>(nonce), mode);

            auto z = y;
            z.ntt();

            /* Matrix-vector multiplication */
            auto w1 = Dilithium::PolynomialVector::generate_polyvec_matrix_pointwise_montgomery(matrix.get_matrix(), z, mode);

            w1.reduce();
            w1.invntt_tomont();

            /* Decompose w and call the random oracle */
            w1.cadd_q();

            auto w1_w0 = w1.polyvec_decompose(mode);

            auto packed_w1 = std::get<0>(w1_w0).polyvec_pack_w1(mode);

            SHAKE_256 shake256_variable(DilithiumModeConstants::SEEDBYTES * 8);
            shake256_variable.update(mu.data(), DilithiumModeConstants::CRHBYTES);
            shake256_variable.update(packed_w1.data(), packed_w1.size());
            auto sm = shake256_variable.final();

            auto cp = Dilithium::Polynomial::poly_challenge(sm.data(), mode);
            cp.ntt();

            /* Compute z, reject if it reveals secret */
            s1.polyvec_pointwise_poly_montgomery(z, cp);

            z.invntt_tomont();
            z.add_polyvec(y);

            z.reduce();
            if(z.polyvec_chknorm(mode.gamma1() - mode.beta()))
               {
               continue;
               }

            /* Check that subtracting cs2 does not change high bits of w and low bits
            * do not reveal secret information */
            Dilithium::PolynomialVector h(mode.k());
            s2.polyvec_pointwise_poly_montgomery(h, cp);
            h.invntt_tomont();
            std::get<1>(w1_w0) -= h;
            std::get<1>(w1_w0).reduce();

            if(std::get<1>(w1_w0).polyvec_chknorm(mode.gamma2() - mode.beta()))
               {
               continue;
               }

            /* Compute hints for w1 */
            t0.polyvec_pointwise_poly_montgomery(h, cp);
            h.invntt_tomont();
            h.reduce();
            if(h.polyvec_chknorm(mode.gamma2()))
               {
               continue;
               }

            std::get<1>(w1_w0).add_polyvec(h);
            std::get<1>(w1_w0).cadd_q();

            auto n = Dilithium::PolynomialVector::generate_hint_polyvec(h, std::get<1>(w1_w0), std::get<0>(w1_w0), mode);
            if(n > mode.omega())
               {
               continue;
               }

            /* Write signature */
            return pack_sig(sm, z, h);
            }

         throw Internal_Error("dilithium signature loop did not terminate");
         }

      size_t signature_length() const override
         {
         const auto& dilithium_math = m_priv_key.m_private->mode();
         return dilithium_math.crypto_bytes();
         }

      AlgorithmIdentifier algorithm_identifier() const override;

      std::string hash_function() const override { return "SHAKE-256(512)"; }
   private:
      // Bit-pack signature sig = (c, z, h).
      secure_vector<uint8_t> pack_sig(const secure_vector<uint8_t>& c,
                                      const Dilithium::PolynomialVector& z, const Dilithium::PolynomialVector& h)
         {
         BOTAN_ASSERT_NOMSG(c.size() == DilithiumModeConstants::SEEDBYTES);
         size_t position = 0;
         const auto& mode = m_priv_key.m_private->mode();
         secure_vector<uint8_t> sig(mode.crypto_bytes());

         std::copy(c.begin(), c.end(), sig.begin());
         position += DilithiumModeConstants::SEEDBYTES;

         for(size_t i = 0; i < mode.l(); ++i)
            {
            z.m_vec[i].polyz_pack(&sig[position + i*mode.polyz_packedbytes()], mode);
            }
         position += mode.l()*mode.polyz_packedbytes();

         /* Encode h */
         for(size_t i = 0; i < mode.omega() + mode.k(); ++i)
            {
            sig[i + position] = 0;
            }

         size_t k = 0;
         for(size_t i = 0; i < mode.k(); ++i)
            {
            for(size_t j = 0; j < DilithiumModeConstants::N; ++j)
               if(h.m_vec[i].m_coeffs[j] != 0)
                  {
                  sig[position + k] = static_cast<uint8_t>(j);
                  k++;
                  }
            sig[position + mode.omega() + i] = static_cast<uint8_t>(k);
            }
         return sig;
         }

      const Dilithium_PrivateKey& m_priv_key;
      SHAKE_256 m_shake;
      bool m_randomized;
   };

AlgorithmIdentifier Dilithium_Signature_Operation::algorithm_identifier() const
   {
   return m_priv_key.algorithm_identifier();
   }

class Dilithium_Verification_Operation final : public PK_Ops::Verification
   {
   public:
      Dilithium_Verification_Operation(const Dilithium_PublicKey& pub_dilithium)
         : m_pub_key(pub_dilithium), m_shake(DilithiumModeConstants::CRHBYTES * 8)
         {
         const auto& mode = m_pub_key.m_public->mode();
         m_shake.update(mode.H(m_pub_key.m_public->raw_pk(), DilithiumModeConstants::SEEDBYTES));
         }

      /*
      * Add more data to the message currently being signed
      * @param msg the message
      * @param msg_len the length of msg in bytes
      */
      void update(const uint8_t msg[], size_t msg_len) override
         {
         m_shake.update(msg, msg_len);
         }

      /*
      * Perform a verification operation
      * @param rng a random number generator
      */
      bool is_valid_signature(const  uint8_t* sig, size_t sig_len) override
         {
         /* Compute CRH(H(rho, t1), msg) */
         const auto mu = m_shake.final_stdvec();

         const auto& mode = m_pub_key.m_public->mode();
         if(sig_len != mode.crypto_bytes())
            {
            return false;
            }

         Dilithium::PolynomialVector z(mode.l());
         Dilithium::PolynomialVector h(mode.k());
         std::vector<uint8_t> signature(sig, sig+sig_len);
         std::array<uint8_t, DilithiumModeConstants::SEEDBYTES> c;
         if(Dilithium::PolynomialVector::unpack_sig(c, z, h, signature, mode))
            {
            return false;
            }

         if(z.polyvec_chknorm(mode.gamma1() - mode.beta()))
            {
            return false;
            }

         /* Matrix-vector multiplication; compute Az - c2^dt1 */
         auto matrix = Dilithium::PolynomialMatrix::generate_matrix(m_pub_key.m_public->rho(), mode);

         auto cp = Dilithium::Polynomial::poly_challenge(c.data(), mode);
         cp.ntt();

         Dilithium::PolynomialVector t1 = m_pub_key.m_public->t1();
         t1.polyvec_shiftl();
         t1.ntt();
         t1.polyvec_pointwise_poly_montgomery(t1, cp);

         z.ntt();

         auto w1 = Dilithium::PolynomialVector::generate_polyvec_matrix_pointwise_montgomery(matrix.get_matrix(), z, mode);
         w1 -= t1;
         w1.reduce();
         w1.invntt_tomont();
         w1.cadd_q();
         w1.polyvec_use_hint(w1, h, mode);
         auto packed_w1 = w1.polyvec_pack_w1(mode);

         /* Call random oracle and verify challenge */
         SHAKE_256 shake256_variable(DilithiumModeConstants::SEEDBYTES * 8);
         shake256_variable.update(mu.data(), mu.size());
         shake256_variable.update(packed_w1.data(), packed_w1.size());
         auto c2 = shake256_variable.final();

         BOTAN_ASSERT_NOMSG(c.size() == c2.size());
         return std::equal(c.begin(), c.end(), c2.begin());
         }

      std::string hash_function() const override { return "SHAKE-256(512)"; }

   private:
      const Dilithium_PublicKey& m_pub_key;
      SHAKE_256 m_shake;
   };

Dilithium_PublicKey::Dilithium_PublicKey(const AlgorithmIdentifier& alg_id, const std::vector<uint8_t>& pk)
   : Dilithium_PublicKey(pk, DilithiumMode(alg_id.oid()), DilithiumKeyEncoding::Raw) {}

Dilithium_PublicKey::Dilithium_PublicKey(const std::vector<uint8_t>& pk, DilithiumMode m, DilithiumKeyEncoding encoding)
   : m_key_encoding(encoding)
   {
   DilithiumModeConstants mode(m);
   switch(m_key_encoding)
      {
      case Botan::DilithiumKeyEncoding::Raw:
         BOTAN_ARG_CHECK(pk.empty() || pk.size() == mode.public_key_bytes(),
                        "dilithium public key does not have the correct byte count");

         m_public = std::make_shared<Dilithium_PublicKeyInternal>(std::move(mode), pk);
         break;

      case Botan::DilithiumKeyEncoding::DER:
         m_public = std::make_shared<Dilithium_PublicKeyInternal>(std::move(mode));
         if(!pk.empty())
            BER_Decoder(pk).decode(*m_public);
         break;
      }
   }

std::string Dilithium_PublicKey::algo_name() const
   {
   return object_identifier().to_formatted_string();
   }

AlgorithmIdentifier Dilithium_PublicKey::algorithm_identifier() const
   {
   return AlgorithmIdentifier(object_identifier(), AlgorithmIdentifier::USE_EMPTY_PARAM);
   }

OID Dilithium_PublicKey::object_identifier() const
   {
   return m_public->mode().oid();
   }

size_t Dilithium_PublicKey::key_length() const
   {
   return m_public->mode().public_key_bytes();
   }

size_t Dilithium_PublicKey::estimated_strength() const
   {
   return m_public->mode().nist_security_strength();
   }
std::vector<uint8_t> Dilithium_PublicKey::public_key_bits() const
   {
   switch(m_key_encoding)
      {
      case DilithiumKeyEncoding::Raw:
         return m_public->raw_pk();

      case DilithiumKeyEncoding::DER:
         std::vector<uint8_t> out;
         DER_Encoder(out).encode(*m_public);
         return out;
      }

   unreachable();
   }

bool Dilithium_PublicKey::check_key(RandomNumberGenerator&, bool) const
   {
   return true; // ???
   }

std::unique_ptr<PK_Ops::Verification> Dilithium_PublicKey::create_verification_op(const std::string& params,
      const std::string& provider) const
   {
   BOTAN_ARG_CHECK(params.empty() || params == "Pure", "Unexpected parameters for verifying with Dilithium");
   if(provider.empty() || provider == "base")
      return std::make_unique<Dilithium_Verification_Operation>(*this);
   throw Provider_Not_Found(algo_name(), provider);
   }

Dilithium_PrivateKey::Dilithium_PrivateKey(RandomNumberGenerator& rng, DilithiumMode m)
   {
   DilithiumModeConstants mode(m);

   secure_vector<uint8_t> seedbuf = rng.random_vec(DilithiumModeConstants::SEEDBYTES);

   auto seed = mode.H(seedbuf, 2 * DilithiumModeConstants::SEEDBYTES + DilithiumModeConstants::CRHBYTES);

   // seed is a concatination of rho || rhoprime || key
   std::vector<uint8_t> rho(seed.begin(), seed.begin() + DilithiumModeConstants::SEEDBYTES);
   secure_vector<uint8_t> rhoprime(seed.begin() + DilithiumModeConstants::SEEDBYTES,
                                   seed.begin() + DilithiumModeConstants::SEEDBYTES + DilithiumModeConstants::CRHBYTES);
   secure_vector<uint8_t> key(seed.begin() + DilithiumModeConstants::SEEDBYTES + DilithiumModeConstants::CRHBYTES,
                              seed.end());

   BOTAN_ASSERT_NOMSG(rho.size() == DilithiumModeConstants::SEEDBYTES);
   BOTAN_ASSERT_NOMSG(rhoprime.size() == DilithiumModeConstants::CRHBYTES);
   BOTAN_ASSERT_NOMSG(key.size() == DilithiumModeConstants::SEEDBYTES);

   /* Generate matrix */
   auto matrix = Dilithium::PolynomialMatrix::generate_matrix(rho, mode);

   /* Sample short vectors s1 and s2 */
   Dilithium::PolynomialVector s1(mode.l());
   Dilithium::PolynomialVector::fill_polyvec_uniform_eta(s1, rhoprime, 0, mode);

   Dilithium::PolynomialVector s2(mode.k());
   Dilithium::PolynomialVector::fill_polyvec_uniform_eta(s2, rhoprime, mode.l(), mode);

   auto [ t0, t1 ] = calculate_t0_and_t1(mode, rho, s1, s2);

   m_public = std::make_shared<Dilithium_PublicKeyInternal>(mode, rho, std::move(t1));

   /* Compute H(rho, t1) == H(pk) and write secret key */
   auto tr = mode.H(m_public->raw_pk(), DilithiumModeConstants::SEEDBYTES);

   m_private = std::make_shared<Dilithium_PrivateKeyInternal>(std::move(mode), std::move(rho), std::move(tr), std::move(key),
                                                              std::move(s1), std::move(s2), std::move(t0));
   }

Dilithium_PrivateKey::Dilithium_PrivateKey(const AlgorithmIdentifier& alg_id, const secure_vector<uint8_t>& sk)
   : Dilithium_PrivateKey(sk, DilithiumMode(alg_id.oid()), DilithiumKeyEncoding::Raw) {}

Dilithium_PrivateKey::Dilithium_PrivateKey(const secure_vector<uint8_t>& sk,
                                           DilithiumMode m, DilithiumKeyEncoding encoding)
   {
   DilithiumModeConstants mode(m);

   switch(encoding)
      {
      case Botan::DilithiumKeyEncoding::Raw:
         BOTAN_ARG_CHECK(sk.size() == mode.private_key_bytes(),
                         "dilithium private key does not have the correct byte count");
         m_private = std::make_shared<Dilithium_PrivateKeyInternal>(std::move(mode), sk);
         break;
      case Botan::DilithiumKeyEncoding::DER:
         m_private = std::make_shared<Dilithium_PrivateKeyInternal>(std::move(mode));
         BER_Decoder(sk).decode(*m_private);
         break;
      }

   m_public = std::make_shared<Dilithium_PublicKeyInternal>(m_private->mode(), m_private->rho(), m_private->s1(), m_private->s2());
   }

secure_vector<uint8_t> Dilithium_PrivateKey::private_key_bits() const
   {
   switch(m_key_encoding)
      {
      case DilithiumKeyEncoding::Raw:
         return m_private->raw_sk();

      case DilithiumKeyEncoding::DER:
         secure_vector<uint8_t> out;
         DER_Encoder(out).encode(*m_private);
         return out;
      }

   unreachable();
   }

std::unique_ptr<PK_Ops::Signature>
Dilithium_PrivateKey::create_signature_op(RandomNumberGenerator& rng,
      const std::string& params,
      const std::string& provider) const
   {
   BOTAN_UNUSED(rng);
   const bool randomized = (params != "Deterministic");
   if(provider.empty() || provider == "base")
      return std::make_unique<Dilithium_Signature_Operation>(*this, randomized);
   throw Provider_Not_Found(algo_name(), provider);
   }

std::unique_ptr<Public_Key> Dilithium_PrivateKey::public_key() const
   {
   auto public_key = std::make_unique<Dilithium_PublicKey>(*this);
   public_key->set_binary_encoding(binary_encoding());
   return public_key;
   }
}
