/*
* KDF1 from ISO 18033-2
* (C) 2016 Philipp Weber
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_KDF1_18033_H_
#define BOTAN_KDF1_18033_H_

#include <botan/kdf.h>
#include <botan/hash.h>

namespace Botan {

/**
* KDF1, from ISO 18033-2
*/
class KDF1_18033 final : public KDF
   {
   public:
      std::string name() const override { return "KDF1-18033(" + m_hash->name() + ")"; }

      std::unique_ptr<KDF> new_object() const override { return std::make_unique<KDF1_18033>(m_hash->new_object()); }

      void kdf(uint8_t key[], size_t key_len,
               const uint8_t secret[], size_t secret_len,
               const uint8_t salt[], size_t salt_len,
               const uint8_t label[], size_t label_len) const override;

      /**
      * @param hash function to use
      */
      explicit KDF1_18033(std::unique_ptr<HashFunction> hash) :
         m_hash(std::move(hash))
         {}
   private:
      std::unique_ptr<HashFunction> m_hash;
   };

}

#endif
