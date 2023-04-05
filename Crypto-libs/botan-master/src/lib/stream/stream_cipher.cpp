/*
* Stream Ciphers
* (C) 2015,2016 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <botan/stream_cipher.h>
#include <botan/internal/scan_name.h>
#include <botan/exceptn.h>

#if defined(BOTAN_HAS_CHACHA)
  #include <botan/internal/chacha.h>
#endif

#if defined(BOTAN_HAS_SALSA20)
  #include <botan/internal/salsa20.h>
#endif

#if defined(BOTAN_HAS_SHAKE_CIPHER)
  #include <botan/internal/shake_cipher.h>
#endif

#if defined(BOTAN_HAS_CTR_BE)
  #include <botan/internal/ctr.h>
#endif

#if defined(BOTAN_HAS_OFB)
  #include <botan/internal/ofb.h>
#endif

#if defined(BOTAN_HAS_RC4)
  #include <botan/internal/rc4.h>
#endif

namespace Botan {

std::unique_ptr<StreamCipher> StreamCipher::create(const std::string& algo_spec,
                                                   const std::string& provider)
   {
   const SCAN_Name req(algo_spec);

#if defined(BOTAN_HAS_CTR_BE)
   if((req.algo_name() == "CTR-BE" || req.algo_name() == "CTR") && req.arg_count_between(1,2))
      {
      if(provider.empty() || provider == "base")
         {
         auto cipher = BlockCipher::create(req.arg(0));
         if(cipher)
            {
            size_t ctr_size = req.arg_as_integer(1, cipher->block_size());
            return std::make_unique<CTR_BE>(std::move(cipher), ctr_size);
            }
         }
      }
#endif

#if defined(BOTAN_HAS_CHACHA)
   if(req.algo_name() == "ChaCha")
      {
      if(provider.empty() || provider == "base")
         return std::make_unique<ChaCha>(req.arg_as_integer(0, 20));
      }

   if(req.algo_name() == "ChaCha20")
      {
      if(provider.empty() || provider == "base")
         return std::make_unique<ChaCha>(20);
      }
#endif

#if defined(BOTAN_HAS_SALSA20)
   if(req.algo_name() == "Salsa20")
      {
      if(provider.empty() || provider == "base")
         return std::make_unique<Salsa20>();
      }
#endif

#if defined(BOTAN_HAS_SHAKE_CIPHER)
   if(req.algo_name() == "SHAKE-128" || req.algo_name() == "SHAKE-128-XOF")
      {
      if(provider.empty() || provider == "base")
         return std::make_unique<SHAKE_128_Cipher>();
      }

   if(req.algo_name() == "SHAKE-256" || req.algo_name() == "SHAKE-256-XOF")
      {
      if(provider.empty() || provider == "base")
         return std::make_unique<SHAKE_256_Cipher>();
      }
#endif

#if defined(BOTAN_HAS_OFB)
   if(req.algo_name() == "OFB" && req.arg_count() == 1)
      {
      if(provider.empty() || provider == "base")
         {
         if(auto cipher = BlockCipher::create(req.arg(0)))
            return std::make_unique<OFB>(std::move(cipher));
         }
      }
#endif

#if defined(BOTAN_HAS_RC4)

   if(req.algo_name() == "RC4" ||
      req.algo_name() == "ARC4" ||
      req.algo_name() == "MARK-4")
      {
      const size_t skip = (req.algo_name() == "MARK-4") ? 256 : req.arg_as_integer(0, 0);

      if(provider.empty() || provider == "base")
         {
         return std::make_unique<RC4>(skip);
         }
      }

#endif

   BOTAN_UNUSED(req);
   BOTAN_UNUSED(provider);

   return nullptr;
   }

//static
std::unique_ptr<StreamCipher>
StreamCipher::create_or_throw(const std::string& algo,
                             const std::string& provider)
   {
   if(auto sc = StreamCipher::create(algo, provider))
      {
      return sc;
      }
   throw Lookup_Error("Stream cipher", algo, provider);
   }

std::vector<std::string> StreamCipher::providers(const std::string& algo_spec)
   {
   return probe_providers_of<StreamCipher>(algo_spec);
   }

}
