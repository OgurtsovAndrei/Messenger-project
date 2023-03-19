/*
* Message Authentication Code base class
* (C) 1999-2008 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <botan/mac.h>
#include <botan/exceptn.h>
#include <botan/internal/scan_name.h>
#include <botan/mem_ops.h>

#if defined(BOTAN_HAS_CMAC)
  #include <botan/internal/cmac.h>
#endif

#if defined(BOTAN_HAS_GMAC)
  #include <botan/internal/gmac.h>
  #include <botan/block_cipher.h>
#endif

#if defined(BOTAN_HAS_HMAC)
  #include <botan/internal/hmac.h>
  #include <botan/hash.h>
#endif

#if defined(BOTAN_HAS_POLY1305)
  #include <botan/internal/poly1305.h>
#endif

#if defined(BOTAN_HAS_SIPHASH)
  #include <botan/internal/siphash.h>
#endif

#if defined(BOTAN_HAS_ANSI_X919_MAC)
  #include <botan/internal/x919_mac.h>
#endif

#if defined(BOTAN_HAS_BLAKE2BMAC)
  #include <botan/internal/blake2bmac.h>
#endif

namespace Botan {

std::unique_ptr<MessageAuthenticationCode>
MessageAuthenticationCode::create(const std::string& algo_spec,
                                  const std::string& provider)
   {
   const SCAN_Name req(algo_spec);

#if defined(BOTAN_HAS_BLAKE2BMAC)
   if(req.algo_name() == "Blake2b" || req.algo_name() == "BLAKE2b")
      {
      return std::make_unique<BLAKE2bMAC>(req.arg_as_integer(0, 512));
      }
#endif

#if defined(BOTAN_HAS_GMAC)
   if(req.algo_name() == "GMAC" && req.arg_count() == 1)
      {
      if(provider.empty() || provider == "base")
         {
         if(auto bc = BlockCipher::create(req.arg(0)))
            return std::make_unique<GMAC>(std::move(bc));
         }
      }
#endif

#if defined(BOTAN_HAS_HMAC)
   if(req.algo_name() == "HMAC" && req.arg_count() == 1)
      {
      if(provider.empty() || provider == "base")
         {
         if(auto hash = HashFunction::create(req.arg(0)))
            return std::make_unique<HMAC>(std::move(hash));
         }
      }
#endif

#if defined(BOTAN_HAS_POLY1305)
   if(req.algo_name() == "Poly1305" && req.arg_count() == 0)
      {
      if(provider.empty() || provider == "base")
         return std::make_unique<Poly1305>();
      }
#endif

#if defined(BOTAN_HAS_SIPHASH)
   if(req.algo_name() == "SipHash")
      {
      if(provider.empty() || provider == "base")
         {
         return std::make_unique<SipHash>(req.arg_as_integer(0, 2), req.arg_as_integer(1, 4));
         }
      }
#endif

#if defined(BOTAN_HAS_CMAC)
   if((req.algo_name() == "CMAC" || req.algo_name() == "OMAC") && req.arg_count() == 1)
      {
      if(provider.empty() || provider == "base")
         {
         if(auto bc = BlockCipher::create(req.arg(0)))
            return std::make_unique<CMAC>(std::move(bc));
         }
      }
#endif


#if defined(BOTAN_HAS_ANSI_X919_MAC)
   if(req.algo_name() == "X9.19-MAC")
      {
      if(provider.empty() || provider == "base")
         {
         return std::make_unique<ANSI_X919_MAC>();
         }
      }
#endif

   BOTAN_UNUSED(req);
   BOTAN_UNUSED(provider);

   return nullptr;
   }

std::vector<std::string>
MessageAuthenticationCode::providers(const std::string& algo_spec)
   {
   return probe_providers_of<MessageAuthenticationCode>(algo_spec);
   }

//static
std::unique_ptr<MessageAuthenticationCode>
MessageAuthenticationCode::create_or_throw(const std::string& algo,
                                           const std::string& provider)
   {
   if(auto mac = MessageAuthenticationCode::create(algo, provider))
      {
      return mac;
      }
   throw Lookup_Error("MAC", algo, provider);
   }

void MessageAuthenticationCode::start_msg(const uint8_t nonce[], size_t nonce_len)
   {
   BOTAN_UNUSED(nonce);
   if(nonce_len > 0)
      throw Invalid_IV_Length(name(), nonce_len);
   }

/*
* Default (deterministic) MAC verification operation
*/
bool MessageAuthenticationCode::verify_mac(const uint8_t mac[], size_t length)
   {
   secure_vector<uint8_t> our_mac = final();

   if(our_mac.size() != length)
      return false;

   return constant_time_compare(our_mac.data(), mac, length);
   }

}
