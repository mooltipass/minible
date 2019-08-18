/*!  \file     fido2.c
*    \brief    Crypto functions related to FIDO2
*    Created:  15/10/2019
*    Author:   0x0ptr@pm.me
*
*    Code inspired/reused from Solo project.
*/
#include <stdint.h>
#include <string.h>
#include "platform_defines.h"
#include "comms_aux_mcu_defines.h"
#include "comms_aux_mcu.h"
#include "cose_key.h"
#include "cbor.h"
#include "crypto.h"
#include "fido2.h"
#include "rng.h"
#include "main.h"

enum ErrorCode
{
    SUCCESS = 0,
    OPERATION_DENIED = 1,
    USER_NOT_PRESENT = 2,
    STORAGE_EXHAUSTED = 3,
    CRED_NOT_FOUND = 4,
};

#define UP_BIT (1 << 0) //User Present
#define UV_BIT (1 << 2) //User Verified
#define AT_BIT (1 << 6) //Attested Credential Data Included
#define ED_BIT (1 << 7) //Extension Data Included


#define MINIBLE_AAGUID ((uint8_t*)"\x6d\xb0\x42\xd0\x61\xaf\x40\x4c\xa8\x87\xe7\x2e\x09\xba\x7e\xb4")

typedef struct auth_data_header_s
{
    uint8_t rpID_hash[RPID_HASH_LEN];
    uint8_t flags;
    uint32_t sign_count;
} __attribute__((packed)) auth_data_header_t;

typedef struct attest_header_s
{
    uint8_t aaguid[AAGUID_LEN];
    uint8_t cred_len_h;
    uint8_t cred_len_l;
} __attribute__((packed)) attest_header_t;

typedef struct attested_data_s
{
    auth_data_header_t auth_data_header;
    attest_header_t attest_header;
    credential_ID_t cred_ID;
    uint8_t enc_pub_key[ENC_PUB_KEY_LEN];
    uint32_t enc_PK_len;
} __attribute__((packed)) attested_data_t;

typedef struct credential_DB_rec_s
{
    uint8_t rpID[RPID_LEN];                  //252
    uint8_t tag[TAG_LEN];                   //16
    uint8_t user_ID[USER_ID_LEN];            //65
    uint8_t user_name[USER_NAME_LEN];        //65
    uint8_t display_name[DISPLAY_NAME_LEN];  //65
    uint8_t spare;                          //1 (padding)
    uint8_t priv_key[PRIV_KEY_LEN];         //32
    uint32_t count;                         //4
} credential_DB_rec_t;

static bool fido2_room_for_more_creds(uint8_t const *rpID);
static void fido2_store_credential_in_db(credential_DB_rec_t const *cred_rec);
static void fido2_update_credential_in_db(credential_DB_rec_t const *cred_rec);
static credential_DB_rec_t *fido2_get_credential(uint8_t const *rpID, uint8_t const *tag);
static credential_DB_rec_t *fido2_get_credential_by_index(uint8_t const *rpID, uint32_t index);

/*! \fn     fido2_update_credential_count(uint32_t count)
*   \brief  Update the credential count. This is to protect against replay attack.
*           Server will check that count is not going backwards or re-used.
*   \param  Current count to be updated
*   \return new count
*/
static uint32_t fido2_update_credential_count(uint32_t count)
{
    //TBD
    count++;
    return count;
}

/*! \fn     fido2_prompt_user(char const *prompt, bool wait_for_ack, uint32_t timeout_sec)
*   \brief  Prompt the user and optionally wait for ack from user.
*   \param  prompt to display
*   \param  boolean whether to wait for ACK
*   \param  the amount of time to display message/wait for ack before timeout
*   \return 0 if accepted or other value if denied/failed
*/
static uint32_t fido2_prompt_user(char const *prompt, bool wait_for_ack, uint32_t timeout_sec)
{
    uint32_t result = SUCCESS;
    //TBD
    //Display request to create cred with name
    //Wait for ack
    return result;
}

/*! \fn     fido2_make_auth_tag(credential_ID_t *cred_ID)
*   \brief  Make a tag uniquely identifying credential
*   \param  credential id
*   \return void
*/
static void fido2_make_auth_tag(credential_ID_t *cred_ID)
{
    rng_fill_array(cred_ID->tag, TAG_LEN);
}

/*! \fn     fido2_calc_attestation_signature(uint8_t const *data, int datalen, uint8_t const * client_data_hash, uint8_t * sigbuf)
*   \brief  Calculate the attestation signature
*   \param  data to calculate signature over
*   \param  length of data
*   \param  hash of client data
*   \param  out buffer with signature
*   \return void
*/
static void fido2_calc_attestation_signature(uint8_t const *data, int datalen, uint8_t const * client_data_hash, uint8_t * sigbuf)
{
    #define HASH_LEN 32
    uint8_t hashbuf[HASH_LEN];

    crypto_sha256_init();
    crypto_sha256_update(data, datalen);
    crypto_sha256_update(client_data_hash, CLIENT_DATA_HASH_LEN);
    crypto_sha256_final(hashbuf);

    crypto_ecc256_sign(hashbuf, HASH_LEN, sigbuf);
}

/*! \fn     fido2_cbor_encode_public_key(uint8_t *buf, uint32_t bufLen, ecc256_pub_key const *pub_key)
*   \brief  CBOR encoder for the public key. All cbor encoding and decoding belongs to aux_mcu.
*           Unfortunately, to be able to create attestation signature we have to encode the public key
*           in cbor format so we have to do some cbor encoding on main_mcu also.
*           Essentiallyt the same as Solo function ctap_add_coseKey() but
*           removed error checking and pass in buffer instead of the cbor encoder object
*           in addition to hardcoding algtype.
*   \param  buffer for encoded output
*   \param  buffer length
*   \param  public key
*   \return encoded public key buffer size
*/
static uint32_t fido2_cbor_encode_public_key(uint8_t *buf, uint32_t bufLen, ecc256_pub_key const *pub_key)
{
    CborEncoder cose_key;
    CborEncoder map;

    cbor_encoder_init(&cose_key, buf, bufLen, 0);

    cbor_encoder_create_map(&cose_key, &map, 5);

    cbor_encode_int(&map, COSE_KEY_LABEL_KTY);
    cbor_encode_int(&map, COSE_KEY_KTY_EC2);

    cbor_encode_int(&map, COSE_KEY_LABEL_ALG);
    cbor_encode_int(&map, COSE_ALG_ES256);

    cbor_encode_int(&map, COSE_KEY_LABEL_CRV);
    cbor_encode_int(&map, COSE_KEY_CRV_P256);

    cbor_encode_int(&map, COSE_KEY_LABEL_X);
    cbor_encode_byte_string(&map, pub_key->x, 32);
    cbor_encode_int(&map, COSE_KEY_LABEL_Y);
    cbor_encode_byte_string(&map, pub_key->y, 32);
    cbor_encoder_close_container(&cose_key, &map);

    return cbor_encoder_get_buffer_size(&cose_key, buf);
}

/*! \fn     fido2_process_exclude_list_item(FIDO2_auth_cred_req_message_t const *request, FIDO2_auth_cred_rsp_message_t *response)
*   \brief  Process Exclude list check from aux_mcu.
*           Checks if tag already exists. Returns 1 if credential exists or 0
*           otherwise. If tag exists prompt the user and wait for user ack.
*   \param  incoming messsage request
*   \param  outgoing response
*   \return void
*/
void fido2_process_exclude_list_item(FIDO2_auth_cred_req_message_t const *request, FIDO2_auth_cred_rsp_message_t *response)
{
    credential_DB_rec_t *cred_rec = fido2_get_credential(request->rpID, request->cred_ID.tag);
    bool found= (cred_rec != NULL) ? 1 : 0;

    memset(response, 0, sizeof(*response));

    if (found)
    {
        fido2_prompt_user("Credential exists already and in exclude list", true, 10);
        memcpy(&response->cred_ID, &request->cred_ID, sizeof(response->cred_ID));
        memcpy(&response->user_ID, cred_rec->user_ID, sizeof(response->user_ID));
        response->result = 1;
    }
}

/*! \fn     fido2_set_attest_sign_count(uint32_t cred_sign_count, uint32_t *attested_sign_count)
*   \brief  Set the attestation sign count in big endian format
*   \param  input sign count
*   \param  sign count in BE format
*   \return void
*/
static void fido2_set_attest_sign_count(uint32_t cred_sign_count, uint32_t *attested_sign_count)
{
    /* FIDO2 requires sign count to be in Big-Endian format */
    *attested_sign_count = cpu_to_be32(cred_sign_count);
}

/*! \fn     fido2_make_auth_data_new_cred(FIDO2_make_auth_data_req_message_t const *request, FIDO2_make_auth_data_rsp_message_t *response)
*   \brief  Make authentication data for a new credential
*   \param  incoming request message
*   \param  outgoing response message
*   \return error code
*/
static uint32_t fido2_make_auth_data_new_cred(FIDO2_make_auth_data_req_message_t const *request, FIDO2_make_auth_data_rsp_message_t *response)
{
    credential_DB_rec_t credential_rec;
    ecc256_pub_key pub_key;
    attested_data_t attested_data;

    if (!fido2_room_for_more_creds(request->rpID))
    {
        response->error_code = STORAGE_EXHAUSTED;
        return response->error_code;
    }
    memset(&attested_data, 0, sizeof(attested_data));

    crypto_sha256_init();
    crypto_sha256_update(request->rpID, strnlen(request->rpID, RPID_LEN));
    crypto_sha256_final(attested_data.auth_data_header.rpID_hash);

    bool confirmation = fido2_prompt_user("Create credential?", TRUE, 10);

    if (confirmation == SUCCESS)
    {
        attested_data.auth_data_header.flags |= UP_BIT; //User present
        attested_data.auth_data_header.flags |= UV_BIT; //User verified
        attested_data.auth_data_header.flags |= AT_BIT; //Attested credential data attached
        response->error_code = SUCCESS;
    }
    else
    {
        attested_data.auth_data_header.flags &= ~UP_BIT;
        attested_data.auth_data_header.flags &= ~UV_BIT;
        attested_data.auth_data_header.flags &= ~AT_BIT;
        response->error_code = (uint8_t) OPERATION_DENIED;
        return response->error_code;
    }

    //Make tag to uniquely identify this credential
    fido2_make_auth_tag(&attested_data.cred_ID);

    //Create key pair
    crypto_ecc256_generate_private_key(credential_rec.priv_key);
    crypto_ecc256_derive_public_key(credential_rec.priv_key, &pub_key);

    //Fill out credential record to store in DB
    //count will always be 1 when new credential created. Update on every get_assertion/get_next_assertion
    credential_rec.count = 0;
    credential_rec.count = fido2_update_credential_count(credential_rec.count);

    memcpy(credential_rec.tag, attested_data.cred_ID.tag, TAG_LEN);
    memcpy(credential_rec.user_ID, request->user_ID, USER_ID_LEN);
    credential_rec.user_ID[USER_ID_LEN-1] = '\0';
    memcpy(credential_rec.user_name, request->user_name, USER_NAME_LEN);
    credential_rec.user_name[USER_NAME_LEN-1] = '\0';
    memcpy(credential_rec.display_name, request->display_name, DISPLAY_NAME_LEN);
    credential_rec.display_name[DISPLAY_NAME_LEN-1] = '\0';
    credential_rec.spare = 0;
    //priv_key is already set above during crypto_ecc256_generate_private_key()

    memcpy(credential_rec.rpID, request->rpID, RPID_LEN);
    fido2_store_credential_in_db(&credential_rec);

    //Create attestation signature
    fido2_set_attest_sign_count(credential_rec.count, &attested_data.auth_data_header.sign_count);

    memcpy(attested_data.attest_header.aaguid, MINIBLE_AAGUID, AAGUID_LEN);
    attested_data.attest_header.cred_len_l = CRED_ID_LEN & 0x00FF;
    attested_data.attest_header.cred_len_h = (CRED_ID_LEN & 0xFF00) >> 8;

    attested_data.enc_PK_len = fido2_cbor_encode_public_key(attested_data.enc_pub_key, sizeof(attested_data.enc_pub_key), &pub_key);
    crypto_ecc256_load_key(credential_rec.priv_key);
    fido2_calc_attestation_signature((uint8_t const *) &attested_data, sizeof(attested_data) - ENC_PUB_KEY_LEN + attested_data.enc_PK_len - sizeof(attested_data.enc_PK_len), request->client_data_hash, response->attest_sig);

    //Fill out response object
    memcpy(response->tag, attested_data.cred_ID.tag, TAG_LEN);
    memcpy(response->rpID_hash, attested_data.auth_data_header.rpID_hash, RPID_HASH_LEN);
    response->count_BE = attested_data.auth_data_header.sign_count;
    memcpy(response->pub_key_x, pub_key.x, PUB_KEY_X_LEN);
    memcpy(response->pub_key_y, pub_key.y, PUB_KEY_Y_LEN);
    //Attest signature already filled out above when calculating attestation signature
    memcpy(response->aaguid, attested_data.attest_header.aaguid, AAGUID_LEN);
    response->cred_ID_len = CRED_ID_LEN;
    response->flags = attested_data.auth_data_header.flags;
    //error code already set

    //Clear private key to limit chance of leaking key
    memset(credential_rec.priv_key, 0, PRIV_KEY_LEN);
    return response->error_code;
}

/*! \fn     fido2_make_auth_data_existing_cred(FIDO2_make_auth_data_req_message_t const *request, FIDO2_make_auth_data_rsp_message_t *response)
*   \brief  Make authentication data for an existing credential
*   \param  incoming request message
*   \param  outgoing response message
*   \return error code
*/
static uint32_t fido2_make_auth_data_existing_cred(FIDO2_make_auth_data_req_message_t const *request, FIDO2_make_auth_data_rsp_message_t *response)
{
    auth_data_header_t auth_data_header;
    credential_DB_rec_t *rec = fido2_get_credential_by_index(request->rpID, request->index);
    ecc256_pub_key pub_key;
    uint32_t confirmation;

    memset(&auth_data_header, 0, sizeof(auth_data_header));

    if (rec == NULL)
    {
        response->error_code = CRED_NOT_FOUND;
        return response->error_code;
    }
    else
    {
    }
    crypto_sha256_init();
    crypto_sha256_update(request->rpID, strnlen(request->rpID, RPID_LEN));
    crypto_sha256_final(auth_data_header.rpID_hash);

    confirmation = fido2_prompt_user("Confirm auth?", TRUE, 10);

    if (confirmation == SUCCESS)
    {
        auth_data_header.flags |= UP_BIT; //User present
        auth_data_header.flags |= UV_BIT; //User verified
        if (request->new_cred)
        {
            auth_data_header.flags |= AT_BIT; //Attested credential data attached
        }
        response->error_code = SUCCESS;
    }
    else
    {
        auth_data_header.flags &= ~UP_BIT;
        auth_data_header.flags &= ~UV_BIT;
        auth_data_header.flags &= ~AT_BIT;
        response->error_code = (uint8_t) OPERATION_DENIED;
        return response->error_code;
    }

    //Create public key from stored private key
    crypto_ecc256_derive_public_key(rec->priv_key, &pub_key);

    //Update credential "count"
    //Update on every get_assertion/get_next_assertion
    rec->count = fido2_update_credential_count(rec->count);

    fido2_update_credential_in_db(rec);

    //Create attestation signature
    fido2_set_attest_sign_count(rec->count, &auth_data_header.sign_count);

    crypto_ecc256_load_key(rec->priv_key);
    fido2_calc_attestation_signature((uint8_t const *) &auth_data_header, sizeof(auth_data_header), request->client_data_hash, response->attest_sig);

    //Fill out response object
    memcpy(response->tag, rec->tag, TAG_LEN);
    memcpy(response->rpID_hash, auth_data_header.rpID_hash, RPID_HASH_LEN);
    response->count_BE = auth_data_header.sign_count;
    memcpy(response->pub_key_x, pub_key.x, PUB_KEY_X_LEN);
    memcpy(response->pub_key_y, pub_key.y, PUB_KEY_Y_LEN);
    //Attest signature already filled out above when calculating attestation signature
    memcpy(response->aaguid, MINIBLE_AAGUID, AAGUID_LEN);
    //We no longer have the credential ID. Not needed for assertion
    response->cred_ID_len = 0;
    response->flags = auth_data_header.flags;
    //error code already set
    return response->error_code;
}

/*! \fn     fido2_process_make_auth_data(FIDO2_make_auth_data_req_message_t const *request, FIDO2_make_auth_data_rsp_message_t *response)
*   \brief  Make the authentication data. This esentially creates the key pair and stores
*           the new record in the DB
*   \param  incoming request messsage
*   \param  outgoing response message
*   \return void
*/
void fido2_process_make_auth_data(FIDO2_make_auth_data_req_message_t const *request, FIDO2_make_auth_data_rsp_message_t *response)
{
    memset(response, 0, sizeof(*response));

    if (request->new_cred == 1)
    {
        fido2_make_auth_data_new_cred(request, response);
    }
    else
    {
        fido2_make_auth_data_existing_cred(request, response);
    }
}

/*! \fn     fido2_process_get_next_credential(FIDO2_get_next_credential_req_message_t const *request, FIDO2_get_next_credential_rsp_message_t *response)
*   \brief  Process getting the next credential for an rpID
*   \param  incoming request messsage
*   \param  outgoing response message
*   \return void
*/
void fido2_process_get_next_credential(FIDO2_get_next_credential_req_message_t const *request, FIDO2_get_next_credential_rsp_message_t *response)
{
    credential_DB_rec_t *rec = NULL;
    memset(response, 0, sizeof(*response));

    rec = fido2_get_credential_by_index(request->rpID, request->index);

    if (rec != NULL)
    {
        //Fill out response object
        memcpy(response->tag, rec->tag, TAG_LEN);
        memcpy(response->user_ID, rec->user_ID, USER_ID_LEN);
        response->result = 1;
    }
    else
    {
        response->result = 0;
    }
}

//DEBUG STORAGE:
#define MAX_CRED 10
static uint8_t cred_count = 0;
static credential_DB_rec_t cred_store[MAX_CRED];

static bool fido2_room_for_more_creds(uint8_t const *rpID)
{
    //TODO: Look up in DB
    return cred_count < MAX_CRED;
}

static void fido2_store_credential_in_db(credential_DB_rec_t const *cred_rec)
{
    //TODO: Debug impl
    if (fido2_room_for_more_creds(cred_rec->rpID))
    {
        memcpy(&cred_store[cred_count++], cred_rec, sizeof(cred_store[0]));
    }
}

static void fido2_update_credential_in_db(credential_DB_rec_t const *cred_rec)
{
    //TODO: Update flash
}

static credential_DB_rec_t *fido2_get_credential(uint8_t const *rpID, uint8_t const *tag)
{
    //TODO: Debug impl
    uint32_t i;

    for(i = 0; i < MAX_CRED; ++i)
    {
        if (memcmp(cred_store[i].tag, tag, TAG_LEN) == 0)
        {
            return &cred_store[i];
        }
    }
    return NULL;
}

static credential_DB_rec_t *fido2_get_credential_by_index(uint8_t const *rpID, uint32_t index)
{
    //TODO: Debug impl

    if (index < MAX_CRED)
    {
        uint32_t i;
        uint32_t curr_count_found = 0;

        for(i = 0; i < MAX_CRED; ++i)
        {
            if (memcmp(cred_store[i].rpID, rpID, RPID_LEN) == 0)
            {
                if (curr_count_found == index)
                    return &cred_store[i];
                ++curr_count_found;
            }
        }
    }
    return NULL;
}

