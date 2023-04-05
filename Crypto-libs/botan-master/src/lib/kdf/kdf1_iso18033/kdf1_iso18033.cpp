/*
* KDF1 from ISO 18033-2
* (C) 2016 Philipp Weber
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <botan/internal/kdf1_iso18033.h>
#include <botan/exceptn.h>

namespace Botan {

void KDF1_18033::kdf(uint8_t key[], size_t key_len,
                     const uint8_t secret[], size_t secret_len,
                     const uint8_t salt[], size_t salt_len,
                     const uint8_t label[], size_t label_len) const
   {
   if(key_len == 0)
      return;

   const size_t blocks_required = key_len / m_hash->output_length();

   if(blocks_required >= 0xFFFFFFFE)
      throw Invalid_Argument("KDF1-18033 maximum output length exceeeded");

   uint32_t counter = 0;
   secure_vector<uint8_t> h;

   size_t offset = 0;
   while(offset != key_len)
      {
      m_hash->update(secret, secret_len);
      m_hash->update_be(counter++);
      m_hash->update(label, label_len);
      m_hash->update(salt, salt_len);
      m_hash->final(h);

      const size_t added = std::min(h.size(), key_len - offset);
      copy_mem(&key[offset], h.data(), added);
      offset += added;
      }
   }

}
