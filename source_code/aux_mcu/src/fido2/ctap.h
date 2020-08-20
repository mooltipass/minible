// Copyright 2019 SoloKeys Developers
//
// Licensed under the Apache License, Version 2.0, <LICENSE-APACHE or
// http://apache.org/licenses/LICENSE-2.0> or the MIT license <LICENSE-MIT or
// http://opensource.org/licenses/MIT>, at your option. This file may not be
// copied, modified, or distributed except according to those terms.
//
// Modified by MiniBLE developers
// Modifications:
// -Removed code related to PIN
// -Changed message sizes
//
#ifndef _CTAP_H
#define _CTAP_H

#include "cbor.h"

#define CTAP_MAKE_CREDENTIAL        0x01
#define CTAP_GET_ASSERTION          0x02
#define CTAP_CANCEL                 0x03
#define CTAP_GET_INFO               0x04
#define CTAP_CLIENT_PIN             0x06
#define CTAP_RESET                  0x07
#define GET_NEXT_ASSERTION          0x08
#define CTAP_VENDOR_FIRST           0x40
#define CTAP_VENDOR_LAST            0xBF

// AAGUID for minible
#define CTAP_AAGUID                   ((uint8_t*)"\x6d\xb0\x42\xd0\x61\xaf\x40\x4c\xa8\x87\xe7\x2e\x09\xba\x7e\xb4")

#define MC_clientDataHash         0x01
#define MC_rp                     0x02
#define MC_user                   0x03
#define MC_pubKeyCredParams       0x04
#define MC_excludeList            0x05
#define MC_extensions             0x06
#define MC_options                0x07
#define MC_pinAuth                0x08
#define MC_pinProtocol            0x09

#define GA_rpId                   0x01
#define GA_clientDataHash         0x02
#define GA_allowList              0x03
#define GA_extensions             0x04
#define GA_options                0x05
#define GA_pinAuth                0x06
#define GA_pinProtocol            0x07

#define CP_pinProtocol            0x01
#define CP_subCommand             0x02
    #define CP_cmdGetRetries      0x01
    #define CP_cmdGetKeyAgreement 0x02
    #define CP_cmdSetPin          0x03
    #define CP_cmdChangePin       0x04
    #define CP_cmdGetPinToken     0x05
#define CP_keyAgreement           0x03
#define CP_pinAuth                0x04
#define CP_newPinEnc              0x05
#define CP_pinHashEnc             0x06
#define CP_getKeyAgreement        0x07
#define CP_getRetries             0x08

#define EXT_HMAC_SECRET_COSE_KEY    0x01
#define EXT_HMAC_SECRET_SALT_ENC    0x02
#define EXT_HMAC_SECRET_SALT_AUTH   0x03

#define EXT_HMAC_SECRET_REQUESTED   0x01
#define EXT_HMAC_SECRET_PARSED      0x02

#define RESP_versions               0x1
#define RESP_extensions             0x2
#define RESP_aaguid                 0x3
#define RESP_options                0x4
#define RESP_maxMsgSize             0x5
#define RESP_pinProtocols           0x6

#define RESP_fmt                    0x01
#define RESP_authData               0x02
#define RESP_attStmt                0x03

#define RESP_credential             0x01
#define RESP_signature              0x03
#define RESP_publicKeyCredentialUserEntity 0x04
#define RESP_numberOfCredentials    0x05

#define RESP_keyAgreement           0x01
#define RESP_pinToken               0x02
#define RESP_retries                0x03

#define PARAM_clientDataHash        (1 << 0)
#define PARAM_rp                    (1 << 1)
#define PARAM_user                  (1 << 2)
#define PARAM_pubKeyCredParams      (1 << 3)
#define PARAM_excludeList           (1 << 4)
#define PARAM_extensions            (1 << 5)
#define PARAM_options               (1 << 6)
#define PARAM_pinAuth               (1 << 7)
#define PARAM_pinProtocol           (1 << 8)
#define PARAM_rpId                  (1 << 9)
#define PARAM_allowList             (1 << 10)

#define MC_requiredMask             (0x0f)


#define HID_MESSAGE_SIZE            64
#define CLIENT_DATA_HASH_SIZE       32  //sha256 hash
#define DOMAIN_NAME_MAX_SIZE        253
#define RP_NAME_LIMIT               32  // application limit, name parameter isn't needed.
#define USER_HANDLE_MAX_SIZE        64
#define USER_NAME_LIMIT             65  // Must be minimum of 64 bytes but can be more.
#define DISPLAY_NAME_LIMIT          65  // Must be minimum of 64 bytes but can be more.
#define ICON_LIMIT                  128 // Must be minimum of 64 bytes but can be more.
#define CTAP_MAX_MESSAGE_SIZE       1024

#define CREDENTIAL_TAG_SIZE         16

#define PUB_KEY_CRED_PUB_KEY        0x01
#define PUB_KEY_CRED_CTAP1          0x41
#define PUB_KEY_CRED_CUSTOM         0x42
#define PUB_KEY_CRED_UNKNOWN        0x3F

#define CREDENTIAL_IS_SUPPORTED     1
#define CREDENTIAL_NOT_SUPPORTED    0

#define ALLOW_LIST_MAX_SIZE         10

#define NEW_PIN_ENC_MAX_SIZE        256     // includes NULL terminator
#define NEW_PIN_ENC_MIN_SIZE        64
#define NEW_PIN_MAX_SIZE            64
#define NEW_PIN_MIN_SIZE            4

#define CTAP_RESPONSE_BUFFER_SIZE   1024

#define PIN_LOCKOUT_ATTEMPTS        8       // Number of attempts total
#define PIN_BOOT_ATTEMPTS           3       // number of attempts per boot

#define CTAP2_UP_DELAY_MS           5000

enum ErrorCode
{
    SUCCESS = 0,
    OPERATION_DENIED = 1,
    USER_NOT_PRESENT = 2,
    STORAGE_EXHAUSTED = 3,
    NO_CREDENTIALS = 4,
};

typedef struct
{
    uint8_t id[USER_HANDLE_MAX_SIZE];
    uint8_t id_size;
    uint8_t name[USER_NAME_LIMIT];
    uint8_t displayName[DISPLAY_NAME_LIMIT];
} CTAP_userEntity;

typedef struct {
    uint8_t tag[CREDENTIAL_TAG_SIZE];
} CredentialId;

struct Credential {
    CredentialId id;
    CTAP_userEntity user;
};
typedef struct Credential CTAP_residentKey;

typedef struct
{
    uint8_t type;
    CredentialId id;
} CTAP_credentialDescriptor;

typedef struct
{
    uint8_t aaguid[16];
    uint8_t credLenH;
    uint8_t credLenL;
    CredentialId id;
} CTAP_attestHeader;

typedef struct
{
    uint8_t rpIdHash[32];
    uint8_t flags;
    uint32_t signCount;
} __attribute__((packed)) CTAP_authDataHeader;

typedef struct
{
    CTAP_authDataHeader head;
    CTAP_attestHeader attest;
} CTAP_authData;

typedef struct
{
    uint8_t data[CTAP_RESPONSE_BUFFER_SIZE];
    uint16_t data_size;
    uint16_t length;
} CTAP_RESPONSE;

struct rpId
{
    uint8_t id[DOMAIN_NAME_MAX_SIZE + 1];     // extra for NULL termination
    size_t size;
    uint8_t name[RP_NAME_LIMIT];
};

typedef struct
{
    struct{
        uint8_t x[32];
        uint8_t y[32];
    } pubkey;

    int kty;
    int crv;
} COSE_key;

typedef struct
{
    CredentialId id;
    CTAP_userEntity user;
    uint8_t publicKeyCredentialType;
    int32_t COSEAlgorithmIdentifier;
    uint8_t rk;
} CTAP_credInfo;

typedef struct
{
    uint32_t paramsParsed;
    uint8_t clientDataHash[CLIENT_DATA_HASH_SIZE];
    struct rpId rp;
    uint8_t pinAuthPresent;
    uint8_t pinAuthEmpty;
} CTAP_requestCommon;

typedef struct
{
    CTAP_requestCommon common;

    CTAP_credInfo credInfo;

    CborValue excludeList;
    size_t excludeListSize;

    uint8_t uv;
    uint8_t up;
    uint8_t uvPresent;
    uint8_t upPresent;
} CTAP_makeCredential;

typedef struct
{
    CTAP_requestCommon common;
    uint8_t clientDataHashPresent;

    int credLen;

    uint8_t rk;
    uint8_t uv;
    uint8_t up;

    CTAP_credentialDescriptor creds[ALLOW_LIST_MAX_SIZE];
    uint8_t allowListPresent;
    uint8_t upPresent;
    uint8_t uvPresent;
} CTAP_getAssertion;

void ctap_response_init(CTAP_RESPONSE * resp);

uint8_t ctap_request(uint8_t * pkt_raw, int length, CTAP_RESPONSE * resp);

// Encodes R,S signature to 2 der sequence of two integers.  Sigder must be at least 72 bytes.
// @return length of der signature
int ctap_encode_der_sig(uint8_t const * const in_sigbuf, uint8_t * const out_sigder);

// Run ctap related power-up procedures (init pinToken, generate shared secret)
void ctap_init(void);

// Resets state between different accesses of different applications
void ctap_reset_state(void);

uint8_t ctap_add_pin_if_verified(uint8_t * pinTokenEnc, uint8_t * platform_pubkey, uint8_t * pinHashEnc);
uint8_t ctap_update_pin_if_verified(uint8_t * pinEnc, int len, uint8_t * platform_pubkey, uint8_t * pinAuth, uint8_t * pinHashEnc);
int ctap_authenticate_credential(struct rpId * rp, CTAP_credentialDescriptor * desc);
uint8_t ctap_make_credential(CborEncoder * encoder, uint8_t * request, int length);
uint8_t ctap_get_assertion(CborEncoder * encoder, uint8_t * request, int length);
uint8_t ctap_add_attest_statement(CborEncoder * map, uint8_t * sigder, int len);
uint8_t ctap_add_user_entity(CborEncoder * map, CTAP_userEntity * user);
void ctap_update_pin(uint8_t * pin, int len);
uint8_t ctap_get_info(CborEncoder * encoder);
uint8_t ctap_decrement_pin_attempts(void);
int8_t ctap_leftover_pin_attempts(void);
void ctap_reset_pin_attempts(void);
uint8_t ctap_is_pin_set(void);
uint8_t ctap_pin_matches(uint8_t * pin, int len);
void ctap_reset(void);
int8_t ctap_device_locked(void);
int8_t ctap_device_boot_locked(void);

// Key storage API

// Return length of key at index.  0 if not exist.
uint16_t ctap_key_len(uint8_t index);

// See error codes in storage.h
int8_t ctap_store_key(uint8_t index, uint8_t * key, uint16_t len);
int8_t ctap_load_key(uint8_t index, uint8_t * key);

#endif
