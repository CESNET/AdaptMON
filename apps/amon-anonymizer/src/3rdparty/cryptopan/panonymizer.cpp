/*
 *  Copyright (c) 2014, Peter Haag
 *  Copyright (c) 2009, Peter Haag
 *  Copyright (c) 2004-2008, SWITCH - Teleinformatikdienste fuer Lehre und Forschung
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *   * Neither the name of the author nor the names of its contributors may be
 *     used to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  $Author: haag $
 *
 *  $Id: panonymizer.c 39 2009-11-25 08:11:15Z haag $
 *
 *  $LastChangedRevision: 39 $
 *
 */

/* Original disclaimer
 * Atlanta, Georgia 30332.
 * All Rights Reserved
 *
 * The following Software is posted on the Internet by the Georgia
 * Tech Research Corporation (GTRC). It was developed by employees
 * of the Georgia Institute of Technology in the College of Computing.
 * GTRC hereby grants to the user a non-exclusive, royalty-free
 * license to utilize such Software for the User's own purposes
 * pursuant to the following conditions.
 *
 *
 * THE SOFTWARE IS LICENSED ON AN "AS IS" BASIS. GTRC MAKES NO WARRANTY
 * THAT ALL ERRORS CAN BE OR HAVE BEEN ELIMINATED FROM THE SOFTWARE.
 * GTRC SHALL NOT BE RESPONSIBLE FOR LOSSES OF ANY KIND RESULTING FROM
 * THE USE OF THE SOFTWARE AND ITS ACCOMPANYING DOCUMENTATION, AND CAN
 * IN NO WAY PROVIDE COMPENSATION FOR ANY LOSSES SUSTAINED, INCLUDING
 * BUT NOT LIMITED TO ANY OBLIGATION, LIABILITY, RIGHT, CLAIM OR REMEDY
 * FOR TORT, OF FOR ANY ACTUAL OR ALLEGED INFRINGEMENT OF PATENTS, COPYRIGHTS,
 * TRADE SECRETS, OR SIMILAR RIGHTS OF THIRD PARTIES, NOR ANY BUSINESS
 * EXPENSE, MACHINE DOWNTIME, OR DAMAGES CAUSED LICENSEE BY ANY DEFICIENCY,
 * DEFECT OR ERROR IN THE SOFTWARE OR MALFUNCTION THEREOF, NOR ANY
 * INCIDENTAL OR CONSEQUENTIAL DAMAGES, HOWEVER CAUSED. GTRC DISCLAIMS
 * ALL WARRANTIES, BOTH EXPRESS AND IMPLIED RESPECTING THE USE AND
 * OPERATION OF THE SOFTWARE AND ANY ACCOMPANYING DOCUMENTATION,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * PARTICULAR PURPOSE AND ANY IMPLIED WARRANTY ARISING FROM COURSE
 * OF PERFORMANCE, COURSE OF DEALING OR USAGE OF TRADE. GTRC MAKES NO
 * WARRANTY THAT THE SOFTWARE IS ADEQUATELY OR COMPLETELY DESCRIBED
 * IN, OR BEHAVES IN ACCORDANCE WITH ANY OF THE ACCOMPANYING
 * DOCUMENTATION. THE USER OF THE SOFTWARE IS EXPECTED TO MAKE THE FINAL
 * EVALUATION OF THE SOFTWARE'S USEFULNESS IN USER'S OWN ENVIRONMENT.
 *
 *
 * Package: Crypto-PAn 1.0
 * File: panonymizer.cpp
 * Last Update: April 17, 2002
 * Author: Jinliang Fan
 *
 */

// NOLINTBEGIN
/// @cond DO_NOT_DOCUMENT

#include <stdexcept>
#include <string>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"

#include "panonymizer.h"

#include <cctype>
#include <cstdint>
#include <cstring>

namespace cryptopan {

// Init
int Panonymizer::init(const uint8_t* key)
{
    // initialize the 128-bit secret key.
    memcpy(m_key, key, 16);

    // initialize the Rijndael cipher.
    int ret = m_rijndael.init(ECB, Encrypt, key, Key16Bytes, NULL);
    if (ret != 0) {
        return ret;
    }

    // initialize the 128-bit secret pad. The pad is encrypted before being used for padding.
    m_rijndael.blockEncrypt(key + 16, 128, m_pad);

    return 0;
}

void Panonymizer::parseCryptoPAnKey(char* s, char* key)
{
    int i;
    int j;
    char numstr[3];
    uint32_t len = strlen(s);

    if (len < 32 || len > 66) {
        throw std::runtime_error(
            "CryptoPAnKey error: size: " + std::to_string(len)
            + ", need either a plain 32 char string, or a 32 byte hex key starting with 0x..");
    }

    if (strlen(s) == 32) {
        // Key is a string
        strncpy(key, s, 32);
        return;
    }

    s[1] = tolower(s[1]);
    numstr[2] = 0;
    if (strlen(s) == 66 && s[0] == '0' && s[1] == 'x') {
        j = 2;
        for (i = 0; i < 32; i++) {
            if (!isxdigit((int) s[j]) || !isxdigit((int) s[j + 1]))
                return;
            numstr[0] = s[j++];
            numstr[1] = s[j++];
            key[i] = strtol(numstr, NULL, 16);
        }
        return;
    }

    // It's an invalid key
    throw std::runtime_error(
        "CryptoPAnKey error: size: " + std::to_string(len)
        + ", need either a plain 32 char string, or a 32 byte hex key starting with 0x..");
} // End of ParseCryptoPAnKey

// Anonymization funtion
uint32_t Panonymizer::anonymize(uint32_t origAddr)
{
    uint8_t rinOutput[16];
    uint8_t rinInput[16];

    uint32_t result = 0;
    uint32_t first4bytesPad;
    uint32_t first4bytesInput;
    int pos;

    memcpy(rinInput, m_pad, 16);
    first4bytesPad = (((uint32_t) m_pad[0]) << 24) + (((uint32_t) m_pad[1]) << 16)
        + (((uint32_t) m_pad[2]) << 8) + (uint32_t) m_pad[3];

    // For each prefixes with length from 0 to 31, generate a bit using the Rijndael cipher,
    // which is used as a pseudorandom function here. The bits generated in every rounds
    // are combineed into a pseudorandom one-time-pad.
    for (pos = 0; pos <= 31; pos++) {
        // Padding: The most significant pos bits are taken from orig_addr. The other 128-pos
        // bits are taken from m_pad. The variables first4bytes_pad and first4bytes_input are used
        // to handle the annoying byte order problem.
        if (pos == 0) {
            first4bytesInput = first4bytesPad;
        } else {
            first4bytesInput
                = ((origAddr >> (32 - pos)) << (32 - pos)) | ((first4bytesPad << pos) >> pos);
        }
        rinInput[0] = (uint8_t) (first4bytesInput >> 24);
        rinInput[1] = (uint8_t) ((first4bytesInput << 8) >> 24);
        rinInput[2] = (uint8_t) ((first4bytesInput << 16) >> 24);
        rinInput[3] = (uint8_t) ((first4bytesInput << 24) >> 24);

        // Encryption: The Rijndael cipher is used as pseudorandom function. During each
        // round, only the first bit of rin_output is used.
        m_rijndael.blockEncrypt(rinInput, 128, rinOutput);

        // Combination: the bits are combined into a pseudorandom one-time-pad
        result |= (rinOutput[0] >> 7) << (31 - pos);
    }
    // XOR the orginal address with the pseudorandom one-time-pad
    return result ^ origAddr;
}

/* little endian CPU's are boring! - but give it a try
 * orig_addr is a ptr to memory, return by inet_pton for IPv6
 * anon_addr return the result in the same order
 */
void Panonymizer::anonymizeV6(const uint64_t origAddr[2], uint64_t* anonAddr)
{
    uint8_t rinOutput[16];
    uint8_t* origBytes;
    uint8_t* result;
    uint8_t rinInput[16];

    int pos;
    int i;
    int bitNum;
    int leftByte;

    anonAddr[0] = anonAddr[1] = 0;
    result = (uint8_t*) anonAddr;
    origBytes = (uint8_t*) origAddr;

    // For each prefixes with length from 0 to 127, generate a bit using the Rijndael cipher,
    // which is used as a pseudorandom function here. The bits generated in every rounds
    // are combineed into a pseudorandom one-time-pad.
    for (pos = 0; pos <= 127; pos++) {
        bitNum = pos & 0x7;
        leftByte = (pos >> 3);

        for (i = 0; i < leftByte; i++) {
            rinInput[i] = origBytes[i];
        }
        rinInput[leftByte] = origBytes[leftByte] >> (7 - bitNum) << (7 - bitNum)
            | (m_pad[leftByte] << bitNum) >> bitNum;
        for (i = leftByte + 1; i < 16; i++) {
            rinInput[i] = m_pad[i];
        }

        // Encryption: The Rijndael cipher is used as pseudorandom function. During each
        // round, only the first bit of rin_output is used.
        m_rijndael.blockEncrypt(rinInput, 128, rinOutput);

        // Combination: the bits are combined into a pseudorandom one-time-pad
        result[leftByte] |= (rinOutput[0] >> 7) << bitNum;
    }
    // XOR the orginal address with the pseudorandom one-time-pad
    anonAddr[0] ^= origAddr[0];
    anonAddr[1] ^= origAddr[1];
}

} // namespace cryptopan

#pragma GCC diagnostic pop
/// @endcond
// NOLINTEND
