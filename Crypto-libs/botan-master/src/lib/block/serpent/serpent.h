/*
* Serpent
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_SERPENT_H_
#define BOTAN_SERPENT_H_

#include <botan/block_cipher.h>

namespace Botan {

/**
* Serpent is the most conservative of the AES finalists
* https://www.cl.cam.ac.uk/~rja14/serpent.html
*/
class Serpent final : public Block_Cipher_Fixed_Params<16, 16, 32, 8>
   {
   public:
      void encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;
      void decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;

      void clear() override;
      std::string provider() const override;
      std::string name() const override { return "Serpent"; }
      std::unique_ptr<BlockCipher> new_object() const override { return std::make_unique<Serpent>(); }

      size_t parallelism() const override { return 4; }
      bool has_keying_material() const override;

   private:

#if defined(BOTAN_HAS_SERPENT_SIMD)
      void simd_encrypt_4(const uint8_t in[64], uint8_t out[64]) const;
      void simd_decrypt_4(const uint8_t in[64], uint8_t out[64]) const;
#endif

#if defined(BOTAN_HAS_SERPENT_AVX2)
      void avx2_encrypt_8(const uint8_t in[128], uint8_t out[128]) const;
      void avx2_decrypt_8(const uint8_t in[128], uint8_t out[128]) const;
#endif

      void key_schedule(const uint8_t key[], size_t length) override;

      secure_vector<uint32_t> m_round_key;
   };

}

#endif
