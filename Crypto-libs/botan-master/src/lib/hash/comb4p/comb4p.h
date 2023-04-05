/*
* Comb4P hash combiner
* (C) 2010 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_COMB4P_H_
#define BOTAN_COMB4P_H_

#include <botan/hash.h>

namespace Botan {

/**
* Combines two hash functions using a Feistel scheme. Described in
* "On the Security of Hash Function Combiners", Anja Lehmann
*/
class Comb4P final : public HashFunction
   {
   public:
      /**
      * @param h1 the first hash
      * @param h2 the second hash
      */
      Comb4P(std::unique_ptr<HashFunction> h1, std::unique_ptr<HashFunction> h2);

      size_t hash_block_size() const override;

      size_t output_length() const override
         {
         return m_hash1->output_length() + m_hash2->output_length();
         }

      std::unique_ptr<HashFunction> new_object() const override
         {
         return std::make_unique<Comb4P>(m_hash1->new_object(), m_hash2->new_object());
         }

      std::unique_ptr<HashFunction> copy_state() const override;

      std::string name() const override
         {
         return "Comb4P(" + m_hash1->name() + "," + m_hash2->name() + ")";
         }

      void clear() override;
   private:
      Comb4P() = default;

      void add_data(const uint8_t input[], size_t length) override;
      void final_result(uint8_t out[]) override;

      std::unique_ptr<HashFunction> m_hash1, m_hash2;
   };

}

#endif
