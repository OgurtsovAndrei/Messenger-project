/*
* (C) 2019,2020,2021 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_ECC_HASH_TO_CURVE_H_
#define BOTAN_ECC_HASH_TO_CURVE_H_

#include <botan/types.h>
#include <botan/ec_point.h>
#include <string>

namespace Botan {

class EC_Group;

/**
* expand_message_xmd
*/
void BOTAN_TEST_API expand_message_xmd(const std::string& hash_fn,
                                       uint8_t output[],
                                       size_t output_len,
                                       const uint8_t input[],
                                       size_t input_len,
                                       const uint8_t domain_sep[],
                                       size_t domain_sep_len);


/**
* Hash an input onto an elliptic curve point using the
* methods from draft-irtf-cfrg-hash-to-curve
*
* This method requires that the ECC group have (a*b) != 0
* which excludes certain groups including secp256k1
*/
EC_Point hash_to_curve_sswu(const EC_Group& group,
                            const std::string& hash_fn,
                            const uint8_t input[],
                            size_t input_len,
                            const uint8_t domain_sep[],
                            size_t domain_sep_len,
                            bool random_oracle);

}

#endif
