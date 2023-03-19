/**
* (C) 2023 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <botan/argon2.h>
#include <immintrin.h>

namespace Botan {

namespace {

class SIMD_4x64 final
   {
   public:
      SIMD_4x64& operator=(const SIMD_4x64& other) = default;
      SIMD_4x64(const SIMD_4x64& other) = default;

      SIMD_4x64& operator=(SIMD_4x64&& other) = default;
      SIMD_4x64(SIMD_4x64&& other) = default;

      ~SIMD_4x64() = default;

      BOTAN_FUNC_ISA("avx2")
      SIMD_4x64() // zero initialized
         {
         m_simd = _mm256_setzero_si256();
         }

      // Load two halves at different addresses
      static BOTAN_FUNC_ISA("avx2")
      SIMD_4x64 load_le2(const void* inl, const void* inh)
         {
         return SIMD_4x64(_mm256_loadu2_m128i(
                             reinterpret_cast<const __m128i*>(inl),
                             reinterpret_cast<const __m128i*>(inh)));
         }

      static BOTAN_FUNC_ISA("avx2")
      SIMD_4x64 load_le(const void* in)
         {
         return SIMD_4x64(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(in)));
         }

      void store_le(uint64_t out[4]) const
         {
         this->store_le(reinterpret_cast<uint8_t*>(out));
         }

      BOTAN_FUNC_ISA("avx2")
      void store_le(uint8_t out[]) const
         {
         _mm256_storeu_si256(reinterpret_cast<__m256i*>(out), m_simd);
         }

      BOTAN_FUNC_ISA("avx2")
      void store_le2(void* outh, void* outl)
         {
         _mm256_storeu2_m128i(
            reinterpret_cast<__m128i*>(outh),
            reinterpret_cast<__m128i*>(outl),
            m_simd);
         }

      SIMD_4x64 operator+(const SIMD_4x64& other) const
         {
         SIMD_4x64 retval(*this);
         retval += other;
         return retval;
         }

      SIMD_4x64 operator^(const SIMD_4x64& other) const
         {
         SIMD_4x64 retval(*this);
         retval ^= other;
         return retval;
         }

      BOTAN_FUNC_ISA("avx2")
      void operator+=(const SIMD_4x64& other)
         {
         m_simd = _mm256_add_epi64(m_simd, other.m_simd);
         }

      BOTAN_FUNC_ISA("avx2")
      void operator^=(const SIMD_4x64& other)
         {
         m_simd = _mm256_xor_si256(m_simd, other.m_simd);
         }

      template<size_t ROT>
      BOTAN_FUNC_ISA("avx2")
      SIMD_4x64 rotr() const
         {
         static_assert(ROT > 0 && ROT < 64, "Invalid rotation constant");

         if constexpr(ROT == 16)
            {
            auto tab = _mm256_setr_epi8(
               2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9,
               2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9);
            return SIMD_4x64(_mm256_shuffle_epi8(m_simd, tab));
            }
         else if constexpr(ROT == 24)
            {
            auto tab = _mm256_setr_epi8(
               3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10,
               3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10);
            return SIMD_4x64(_mm256_shuffle_epi8(m_simd, tab));
         }
         else if constexpr(ROT == 32)
            {
            auto tab = _mm256_setr_epi8(
               4, 5, 6, 7, 0, 1, 2, 3, 12, 13, 14, 15, 8, 9, 10, 11,
               4, 5, 6, 7, 0, 1, 2, 3, 12, 13, 14, 15, 8, 9, 10, 11);
            return SIMD_4x64(_mm256_shuffle_epi8(m_simd, tab));
            }
         else
            {
            return SIMD_4x64(_mm256_or_si256(_mm256_srli_epi64(m_simd, static_cast<int>(ROT)),
                                             _mm256_slli_epi64(m_simd, static_cast<int>(64-ROT))));
            }
         }

      template<size_t ROT>
      SIMD_4x64 rotl() const
         {
         return this->rotr<64-ROT>();
         }

      // Argon2 specific operation
      static BOTAN_FUNC_ISA("avx2")
      SIMD_4x64 mul2_32(SIMD_4x64 x, SIMD_4x64 y)
         {
         const __m256i m = _mm256_mul_epu32(x.m_simd, y.m_simd);
         return SIMD_4x64(_mm256_add_epi64(m, m));
         }

      template<uint8_t CTRL>
      static BOTAN_FUNC_ISA("avx2") SIMD_4x64 permute_4x64(SIMD_4x64 x)
         {
         return SIMD_4x64(_mm256_permute4x64_epi64(x.m_simd, CTRL));
         }

      // Argon2 specific
      static void twist(
         SIMD_4x64& B,
         SIMD_4x64& C,
         SIMD_4x64& D)
         {
         B = SIMD_4x64::permute_4x64<0b00'11'10'01>(B);
         C = SIMD_4x64::permute_4x64<0b01'00'11'10>(C);
         D = SIMD_4x64::permute_4x64<0b10'01'00'11>(D);
         }

      // Argon2 specific
      static void untwist(
         SIMD_4x64& B,
         SIMD_4x64& C,
         SIMD_4x64& D)
         {
         B = SIMD_4x64::permute_4x64<0b10'01'00'11>(B);
         C = SIMD_4x64::permute_4x64<0b01'00'11'10>(C);
         D = SIMD_4x64::permute_4x64<0b00'11'10'01>(D);
         }

      explicit BOTAN_FUNC_ISA("avx2") SIMD_4x64(__m256i x) : m_simd(x) {}
   private:
      __m256i m_simd;
   };

BOTAN_FORCE_INLINE void blamka_G(
   SIMD_4x64& A,
   SIMD_4x64& B,
   SIMD_4x64& C,
   SIMD_4x64& D)
   {
   A += B + SIMD_4x64::mul2_32(A, B);
   D ^= A;
   D = D.rotr<32>();

   C += D + SIMD_4x64::mul2_32(C, D);
   B ^= C;
   B = B.rotr<24>();

   A += B + SIMD_4x64::mul2_32(A, B);
   D ^= A;
   D = D.rotr<16>();

   C += D + SIMD_4x64::mul2_32(C, D);
   B ^= C;
   B = B.rotr<63>();
   }

BOTAN_FORCE_INLINE void blamka_R(
   SIMD_4x64& A,
   SIMD_4x64& B,
   SIMD_4x64& C,
   SIMD_4x64& D)
   {
   blamka_G(A, B, C, D);

   SIMD_4x64::twist(B, C, D);
   blamka_G(A, B, C, D);
   SIMD_4x64::untwist(B, C, D);
   }

}

BOTAN_FUNC_ISA("avx2")
void Argon2::blamka_avx2(uint64_t N[128], uint64_t T[128])
   {
   for(size_t i = 0; i != 8; ++i)
      {
      SIMD_4x64 A = SIMD_4x64::load_le(&N[16*i + 4*0]);
      SIMD_4x64 B = SIMD_4x64::load_le(&N[16*i + 4*1]);
      SIMD_4x64 C = SIMD_4x64::load_le(&N[16*i + 4*2]);
      SIMD_4x64 D = SIMD_4x64::load_le(&N[16*i + 4*3]);

      blamka_R(A, B, C, D);

      A.store_le(&T[16*i + 4*0]);
      B.store_le(&T[16*i + 4*1]);
      C.store_le(&T[16*i + 4*2]);
      D.store_le(&T[16*i + 4*3]);
      }

   for(size_t i = 0; i != 8; ++i)
      {
      SIMD_4x64 A = SIMD_4x64::load_le2(&T[2*i + 32*0], &T[2*i + 32*0 + 16]);
      SIMD_4x64 B = SIMD_4x64::load_le2(&T[2*i + 32*1], &T[2*i + 32*1 + 16]);
      SIMD_4x64 C = SIMD_4x64::load_le2(&T[2*i + 32*2], &T[2*i + 32*2 + 16]);
      SIMD_4x64 D = SIMD_4x64::load_le2(&T[2*i + 32*3], &T[2*i + 32*3 + 16]);

      blamka_R(A, B, C, D);

      A.store_le2(&T[2*i + 32*0], &T[2*i + 32*0 + 16]);
      B.store_le2(&T[2*i + 32*1], &T[2*i + 32*1 + 16]);
      C.store_le2(&T[2*i + 32*2], &T[2*i + 32*2 + 16]);
      D.store_le2(&T[2*i + 32*3], &T[2*i + 32*3 + 16]);
      }

   for(size_t i = 0; i != 128 / 8; ++i)
      {
      SIMD_4x64 n0 = SIMD_4x64::load_le(&N[8*i]);
      SIMD_4x64 n1 = SIMD_4x64::load_le(&N[8*i+4]);
      SIMD_4x64 t0 = SIMD_4x64::load_le(&T[8*i]);
      SIMD_4x64 t1 = SIMD_4x64::load_le(&T[8*i+4]);

      n0 ^= t0;
      n1 ^= t1;
      n0.store_le(&N[8*i]);
      n1.store_le(&N[8*i+4]);
      }
   }

}
