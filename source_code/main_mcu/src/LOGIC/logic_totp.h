/* 
 * This file is part of the Mooltipass Project (https://github.com/mooltipass).
 * Copyright (c) 2019 Stephan Mathieu
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
/*!  \file     logic_totp.h
*    \brief    General logic for TOTP algorithm based on RFC 6238 (https://tools.ietf.org/html/rfc6238)
*    Created:  31/07/2020
*    Author:   dnns01
*/


#ifndef LOGIC_TOTP_H_
#define LOGIC_TOTP_H_

#include "bearssl_hash.h"
#include "bearssl_hmac.h"
#include "driver_timer.h"
#include "defines.h"
#include "utils.h"
#include <time.h>

/** SHA1 HMAC key length in byte */
#define LOGIC_TOTP_SHA1_HMAC_KEY_LEN 10
/** SHA1 HMAC message length in byte */
#define LOGIC_TOTP_SHA1_HMAC_MESSAGE_LEN 8
/** Alphabet that is used in Base32 alrogithm to encode bytes as string */
#define LOGIC_TOTP_BASE32_ALPHABET "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"
/** Base32 alphabet length in bytes */
#define LOGIC_TOTP_BASE32_ALPHABET_LEN 32
/** Amount of bits that are encoded as a character from the alphabet in Base 32 (2^5 = 32, which is the alphabet length) */
#define LOGIC_TOTP_BASE32_BITS 5
/** Length of encoded input key in bytes, that should be decoded */
#define LOGIC_TOTP_ENCODED_KEY_LEN 16
/** Length of decoded input key in bytes */
#define LOGIC_TOTP_DECODED_KEY_LEN 10
/** Number of bits that are in one byte */
#define LOGIC_TOTP_BITS_PER_BYTE 8
/** Based on RFC 6238 this is the number of bytes that are used from the calculated SHA1 HMAC hash to get TOTP token */
#define LOGIC_TOTP_TRUNCATED_HASH_LEN 4

/* Prototypes */
void logic_totp(char* totp_token, const char* key);

#endif /* LOGIC_TOTP_H_ */
