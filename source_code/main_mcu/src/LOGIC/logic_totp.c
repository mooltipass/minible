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
/*!  \file     logic_totp.c
*    \brief    General logic for TOTP algorithm based on RFC 6238 (https://tools.ietf.org/html/rfc6238)
*    Created:  31/07/2020
*    Author:   dnns01
*/
#include "logic_totp.h"

/*! \fn     logic_totp_sha1_hmac(void *out, const void *key, const void *message)
*   \brief  Calculate SHA1 HMAC Hash for given message using given key.
*   \param  calculated hash, char[20] expected
*   \param  key for SHA1 HMAC calculation, char[10] expected
*   \param  message to be hashed, 8 bytes expected
*   \return void
*/
static void logic_totp_sha1_hmac(void* out, const void *key, const void *message)
{
    br_hmac_key_context key_context;
    br_hmac_context context;

    br_hmac_key_init(&key_context, &br_sha1_vtable, key, LOGIC_TOTP_SHA1_HMAC_KEY_LEN);
    br_hmac_init(&context, &key_context, 0);
    br_hmac_update(&context, message, LOGIC_TOTP_SHA1_HMAC_MESSAGE_LEN);
    br_hmac_out(&context, out);
}


/*! \fn     logic_totp_get_time()
*   \brief  Retrieves time from RTC and converts calendar back to unix timestamp
*   \return current unix timestamp
*/
static time_t logic_totp_get_time(void)
{
    calendar_t calendar;
    struct tm datetime;

    timer_get_calendar(&calendar);

    datetime.tm_sec = calendar.bit.SECOND;
    datetime.tm_min = calendar.bit.MINUTE;
    datetime.tm_hour = calendar.bit.HOUR;
    datetime.tm_mday = calendar.bit.DAY;
    datetime.tm_mon = calendar.bit.MONTH - 1;
    datetime.tm_year = calendar.bit.YEAR + 2000 - 1900;

    // Assumption: If running in the emulator, gmtime is used. Therefore we call _mkgmtime to get the current timestamp, whereas
    // the real device is running on UTC, therefore mktime is used to get the timestamp.
#ifndef EMULATOR_BUILD
    return mktime(&datetime);
#else
    return _mkgmtime(&datetime);
#endif
}


/*! \fn     logic_totp_base32_decode_key(char* decoded_key, const char* encoded_key)
*   \brief  Decodes the given Base32 encoded key
*   \param  decoded key, char[10] expected
*   \param  Base32 encoded key, char[16] expected
*   \return void
*/
static void logic_totp_base32_decode_key(char* decoded_key, const char* encoded_key)
{
    // Initial value for shift_offset is 8, because there are 8 bits in a byte
    int16_t shift_offset= LOGIC_TOTP_BITS_PER_BYTE;
    // We start at the first element of decoded key
    int16_t decoded_index = 0;

    // Initialize the whole decoded_key array with 0 bytes, to avoid errors in further processing with random values,
    // as we will now shift the 5 bits to their new position in decoded_key array and use logical or to put them in place.
    // A wrong 1 bit at any place would in this case result in a wrong decoding.
    memset(decoded_key, 0x00, LOGIC_TOTP_DECODED_KEY_LEN);

    // For Base32 decoding we loop through the encoded key char by char, and figure out the index of that character in the Base32 alphabet.
    // The least significant 5 bits of that index will then be concatenated to get the decoded key. If the concatenation if not a multiple of 8,
    // the last Byte gets left padded with 0 bits, until the lenght is a multiple of 8.
    for (int i = 0; i < LOGIC_TOTP_ENCODED_KEY_LEN; i++)
    {
        char c = encoded_key[i];
        for (int j = 0; j < LOGIC_TOTP_BASE32_ALPHABET_LEN; j++)
        {
            if (c == LOGIC_TOTP_BASE32_ALPHABET[j])
            {
                // We have now found the index of the character in our Base32 alphabet, which is j. Now we need to concatenate the last 5 bits to the decoded key.
                if (shift_offset - LOGIC_TOTP_BASE32_BITS >= 0)
                {
                    // In this case, there are minimum 5 bits in the current byte at decoded_key[decoded_index] remaining.
                    // So we can shift the index by (shift_offset - logic_totp_base32_bits) places. For the first character of encoded_key this would be for example
                    // (shift_offset - logic_totp_base32_bits) = (8 - 5) = 3, so index is shifted 3 places to the left.
                    // shift_offset is then set to this value for the next character of the encoded_key.
                    decoded_key[decoded_index] |= j << (shift_offset - LOGIC_TOTP_BASE32_BITS);
                    shift_offset -= LOGIC_TOTP_BASE32_BITS;
                }
                else
                {
                    // In this case, we have to split up the 5 bits of the index between the remaining bits of decoded_key[decoded_index]
                    // and the first bits of decoded_key[decoded_index + 1]. Therefore we first have to shift the index by 5 - shift_offset to the right.
                    // That means, we have shift_offset (which is also the number of remaining bits in the current byte) bits from the index remaining to put into the current byte.
                    // Then we need to calculate the new shift_offset, which is basically the old shift_offset + 3 (As already described, the shift_offset is also
                    // the number of remaining bits in the byte. As we have in the first step used this number of bits from the index, there are 5 - shift_offset bits from the
                    // index remaining, that have to be put in the next byte. To get the number places the remaining bits have to be shifted in the next byte, we have to subtract
                    // shift_offset from the number of bits in a byte. If we rearrange the order in that term, we get shift_offset = shift_offset + 8 - 5 = shift_offset + 3)
                    // As an example for the second character, shift_offset is 3. So we take the first 3 of the 5 index bits and put them into current byte. Then we have 2 bits remaining.
                    // Those bits need to be shifted then by 6 places to the left, to be at the beginning of the next byte.
                    decoded_key[decoded_index] |= j >> (LOGIC_TOTP_BASE32_BITS - shift_offset);
                    shift_offset += LOGIC_TOTP_BITS_PER_BYTE - LOGIC_TOTP_BASE32_BITS;
                    decoded_index++;
                    decoded_key[decoded_index] |= j << (shift_offset);
                }
            }
        }
    }
}


/*! \fn     logic_totp(char* totp_token, const char* key)
*   \brief  Calculate TOTP Token using given key
*   \param  calculated totp_token, char[6] expected
*   \param  Base32 encoded key that is used to calculate totp_token, char[16] expectd
*   \return void
*/
void logic_totp(char* totp_token, const char* key)
{
    char hash[20];
    char decoded_key[10];
    int truncated_hash = 0;
    int offset = 0;
    int shift_offset = LOGIC_TOTP_BITS_PER_BYTE * 3;
    int token_idx = LOGIC_TOTP_TOKEN_LEN - 1;
    time_t timestamp = logic_totp_get_time();
    // step is the message the hash gets calculated for. TOTP is based on the current unix timestamp. As a TOTP token is valid for 30 seconds, we divide the current timestamp by
    // 30 before calculating the SHA1 HMAC hash
    time_t step = utils_switch_endiannes(timestamp/30);

    logic_totp_base32_decode_key(decoded_key, key);
    logic_totp_sha1_hmac(hash, decoded_key, &step);

    // We use the last nibble of the calculated hash to get the offset. This is the offset in the hash, from where we take 4 bytes as truncated hash.
    offset = hash[19] & 0x0F;

    for (int i = offset; i < offset + LOGIC_TOTP_TRUNCATED_HASH_LEN; i++)
    {
        // Get the truncated hash as int, as we those 4 bytes as a whole number.
        truncated_hash |= (hash[i] << shift_offset) & (0x000000FF << shift_offset);
        shift_offset -= LOGIC_TOTP_BITS_PER_BYTE;
    }

    // Initialize the totp_token with 0 characters. This is used for leading zeros, if the truncated hash is less than 100000.
    memset(totp_token, '0', LOGIC_TOTP_TOKEN_LEN);

    // Usually you would now first calculate truncated_hash mod 1000000 to have only 6 places remaining. We don't need that, as we take a maximum of 6 places from truncated_hash,
    // which is guaranteed by using token_idy >= 0 as termination condition for the while loop. Here we convert truncated_hash from an int to a string representing the number.
    while ((truncated_hash > 0) | (token_idx >= 0))
    {
        totp_token[token_idx] = truncated_hash % 10 + '0';
        truncated_hash /= 10;
        token_idx--;
    }
}
