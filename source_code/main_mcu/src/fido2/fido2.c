/*!  \file     fido2.c
*    \brief    Crypto functions related to FIDO2
*    Created:  15/10/2019
*    Author:   0x0ptr@pm.me
*
*    Code inspired/reused from Solo project.
*/
#include <stdint.h>
#include <string.h>
#include "comms_aux_mcu_defines.h"
#include "logic_encryption.h"
#include "platform_defines.h"
#include "logic_security.h"
#include "comms_aux_mcu.h"
#include "logic_user.h"
#include "cose_key.h"
#include "utils.h"
#include "fido2.h"
#include "rng.h"
#include "main.h"

static bool fido2_room_for_more_creds(uint8_t const *rpID);
static void fido2_store_credential_in_db(credential_DB_rec_t const *cred_rec);
static void fido2_update_credential_in_db(credential_DB_rec_t const *cred_rec);
static credential_DB_rec_t *fido2_get_credential(uint8_t const *rpID, uint8_t const *tag);

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
    uint32_t result = FIDO2_SUCCESS;
    //TBD
    //Display request to create cred with name
    //Wait for ack
    return result;
}

/*! \fn     fido2_make_auth_tag(fido2_credential_ID_t *cred_ID)
*   \brief  Make a tag uniquely identifying credential
*   \param  credential id
*   \return void
*/
static void fido2_make_auth_tag(fido2_credential_ID_t *cred_ID)
{
    rng_fill_array(cred_ID->tag, FIDO2_CREDENTIAL_ID_LENGTH);
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
    uint8_t hashbuf[SHA256_OUTPUT_LENGTH];

    _Static_assert(sizeof(hashbuf) >= 32, "Buffer must be 32 bytes or greater");

    logic_encryption_sha256_init();
    logic_encryption_sha256_update(data, datalen);
    logic_encryption_sha256_update(client_data_hash, FIDO2_CLIENT_DATA_HASH_LEN);
    logic_encryption_sha256_final(hashbuf);

    logic_encryption_ecc256_sign(hashbuf, SHA256_OUTPUT_LENGTH, sigbuf);
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
    uint16_t output_data_length = 0;

    buf[output_data_length++] = FIDO2_CBOR_CONTAINER_START | 5; //start and number of elements in container
    buf[output_data_length++] = utils_get_cbor_encoded_value_for_val_btw_m24_p23(COSE_KEY_LABEL_KTY);
    buf[output_data_length++] = utils_get_cbor_encoded_value_for_val_btw_m24_p23(COSE_KEY_KTY_EC2);
    buf[output_data_length++] = utils_get_cbor_encoded_value_for_val_btw_m24_p23(COSE_KEY_LABEL_ALG);
    buf[output_data_length++] = utils_get_cbor_encoded_value_for_val_btw_m24_p23(COSE_ALG_ES256);
    buf[output_data_length++] = utils_get_cbor_encoded_value_for_val_btw_m24_p23(COSE_KEY_LABEL_CRV);
    buf[output_data_length++] = utils_get_cbor_encoded_value_for_val_btw_m24_p23(COSE_KEY_CRV_P256);
    buf[output_data_length++] = utils_get_cbor_encoded_value_for_val_btw_m24_p23(COSE_KEY_LABEL_X);
    output_data_length+= utils_cbor_encode_32byte_bytestring((uint8_t*)pub_key->x, &buf[output_data_length]);
    buf[output_data_length++] = utils_get_cbor_encoded_value_for_val_btw_m24_p23(COSE_KEY_LABEL_Y);
    output_data_length+= utils_cbor_encode_32byte_bytestring((uint8_t*)pub_key->y, &buf[output_data_length]);

    return output_data_length;
}

/*! \fn     fido2_process_exclude_list_item(fido2_auth_cred_req_message_t const *request, fido2_auth_cred_rsp_message_t *response)
*   \brief  Process Exclude list check from aux_mcu.
*           Checks if tag already exists. Returns 1 if credential exists or 0
*           otherwise. If tag exists prompt the user and wait for user ack.
*   \param  incoming messsage request
*   \param  outgoing response
*   \return void
*/
void fido2_process_exclude_list_item(fido2_auth_cred_req_message_t const* request, fido2_auth_cred_rsp_message_t* response)
{
    credential_DB_rec_t *cred_rec = fido2_get_credential(request->rpID, request->cred_ID.tag);
    bool found= (cred_rec != NULL) ? 1 : 0;

    memset(response, 0, sizeof(*response));

    if (found)
    {
        fido2_prompt_user("Credential exists already and in exclude list", true, 10);
        memcpy(&response->cred_ID, &request->cred_ID, sizeof(response->cred_ID));
        memcpy(&response->user_ID, cred_rec->user_ID, sizeof(response->user_ID));
        response->result = FIDO2_CREDENTIAL_EXISTS;
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

/*! \fn     fido2_make_auth_data_new_cred(fido2_make_auth_data_req_message_t const *request, fido2_make_auth_data_rsp_message_t *response)
*   \brief  Make authentication data for a new credential
*   \param  incoming request message
*   \param  outgoing response message
*   \return error code
*/
static uint32_t fido2_make_auth_data_new_cred(fido2_make_auth_data_req_message_t const *request, fido2_make_auth_data_rsp_message_t *response)
{
    uint8_t credential_id[FIDO2_CREDENTIAL_ID_LENGTH];
    uint8_t private_key[FIDO2_PRIV_KEY_LEN];
    credential_DB_rec_t credential_rec;
    attested_data_t attested_data;
    ecc256_pub_key pub_key;
    
    /* 
    
    /* Check for logged in user first */
    if (logic_security_is_smc_inserted_unlocked() == FALSE)
    {
        response->error_code = FIDO2_USER_NOT_PRESENT;
        return response->error_code;
    }

    /* Clear new attested data */
    memset(&attested_data, 0, sizeof(attested_data));
    
    /* Create credential ID: random bytes */
    rng_fill_array(credential_id, sizeof(credential_id));

    /* Compute RPID hash, required for answer */
    logic_encryption_sha256_init();
    logic_encryption_sha256_update(request->rpID, utils_u8strnlen(request->rpID, FIDO2_RPID_LEN));
    logic_encryption_sha256_final(attested_data.auth_data_header.rpID_hash);

    /* Create encryption key pair */
    logic_encryption_ecc256_generate_private_key(private_key);
    logic_encryption_ecc256_derive_public_key(private_key, &pub_key);
    
    /* Try to store new credential, function will create a new credential id  */
    //fido2_return_code_te temp_return = logic_user_store_webauthn_credential(request->rpID, request->user_ID, request->rpID, request->display_name, private_key, credential_id);

    bool confirmation = fido2_prompt_user("Create credential?", TRUE, 10);

    if (confirmation == FIDO2_SUCCESS)
    {
        attested_data.auth_data_header.flags |= FIDO2_UP_BIT; //User present
        attested_data.auth_data_header.flags |= FIDO2_UV_BIT; //User verified
        attested_data.auth_data_header.flags |= FIDO2_AT_BIT; //Attested credential data attached
        response->error_code = FIDO2_SUCCESS;
    }
    else
    {
        attested_data.auth_data_header.flags &= ~FIDO2_UP_BIT;
        attested_data.auth_data_header.flags &= ~FIDO2_UV_BIT;
        attested_data.auth_data_header.flags &= ~FIDO2_AT_BIT;
        response->error_code = (uint8_t) FIDO2_OPERATION_DENIED;
        return response->error_code;
    }

    //Make tag to uniquely identify this credential
    fido2_make_auth_tag(&attested_data.cred_ID);

    //Fill out credential record to store in DB
    //count will always be 1 when new credential created. Update on every get_assertion/get_next_assertion
    credential_rec.count = 0;
    credential_rec.count = fido2_update_credential_count(credential_rec.count);

    memcpy(credential_rec.priv_key, private_key, FIDO2_PRIV_KEY_LEN);
    memcpy(credential_rec.tag, attested_data.cred_ID.tag, FIDO2_CREDENTIAL_ID_LENGTH);
    memcpy(credential_rec.user_ID, request->user_ID, FIDO2_USER_ID_LEN);
    credential_rec.user_ID[FIDO2_USER_ID_LEN-1] = '\0';
    memcpy(credential_rec.user_name, request->user_name, FIDO2_USER_NAME_LEN);
    credential_rec.user_name[FIDO2_USER_NAME_LEN-1] = '\0';
    memcpy(credential_rec.display_name, request->display_name, FIDO2_DISPLAY_NAME_LEN);
    credential_rec.display_name[FIDO2_DISPLAY_NAME_LEN-1] = '\0';
    credential_rec.spare = 0;
    //priv_key is already set above during logic_encryption_ecc256_generate_private_key()

    memcpy(credential_rec.rpID, request->rpID, FIDO2_RPID_LEN);
    fido2_store_credential_in_db(&credential_rec);

    //Create attestation signature
    fido2_set_attest_sign_count(credential_rec.count, &attested_data.auth_data_header.sign_count);

    memcpy(attested_data.attest_header.aaguid, FIDO2_MINIBLE_AAGUID, FIDO2_AAGUID_LEN);
    attested_data.attest_header.cred_len_l = FIDO2_CREDENTIAL_ID_LENGTH & 0x00FF;
    attested_data.attest_header.cred_len_h = (FIDO2_CREDENTIAL_ID_LENGTH & 0xFF00) >> 8;

    attested_data.enc_PK_len = fido2_cbor_encode_public_key(attested_data.enc_pub_key, sizeof(attested_data.enc_pub_key), &pub_key);
    logic_encryption_ecc256_load_key(credential_rec.priv_key);
    fido2_calc_attestation_signature((uint8_t const *) &attested_data, sizeof(attested_data) - FIDO2_ENC_PUB_KEY_LEN + attested_data.enc_PK_len - sizeof(attested_data.enc_PK_len), request->client_data_hash, response->attest_sig);

    //Fill out response object
    memcpy(response->tag, attested_data.cred_ID.tag, FIDO2_CREDENTIAL_ID_LENGTH);
    memcpy(response->user_ID, credential_rec.user_ID, FIDO2_USER_ID_LEN);
    memcpy(response->rpID_hash, attested_data.auth_data_header.rpID_hash, FIDO2_RPID_HASH_LEN);
    response->count_BE = attested_data.auth_data_header.sign_count;
    memcpy(response->pub_key_x, pub_key.x, FIDO2_PUB_KEY_X_LEN);
    memcpy(response->pub_key_y, pub_key.y, FIDO2_PUB_KEY_Y_LEN);
    //Attest signature already filled out above when calculating attestation signature
    memcpy(response->aaguid, attested_data.attest_header.aaguid, FIDO2_AAGUID_LEN);
    response->cred_ID_len = FIDO2_CREDENTIAL_ID_LENGTH;
    response->flags = attested_data.auth_data_header.flags;
    //error code already set

    //Clear private key to limit chance of leaking key
    memset(credential_rec.priv_key, 0, FIDO2_PRIV_KEY_LEN);
    return response->error_code;
}

/*! \fn     fido2_make_auth_data_existing_cred(fido2_make_auth_data_req_message_t const *request, fido2_make_auth_data_rsp_message_t *response)
*   \brief  Make authentication data for an existing credential
*   \param  incoming request message
*   \param  outgoing response message
*   \return error code
*/
static uint32_t fido2_make_auth_data_existing_cred(fido2_make_auth_data_req_message_t const *request, fido2_make_auth_data_rsp_message_t *response)
{
    auth_data_header_t auth_data_header;
    credential_DB_rec_t *rec = fido2_get_credential(request->rpID, NULL);
    ecc256_pub_key pub_key;
    uint32_t confirmation;

    memset(&auth_data_header, 0, sizeof(auth_data_header));

    if (rec == NULL)
    {
        response->error_code = FIDO2_CRED_NOT_FOUND;
        return response->error_code;
    }
    else
    {
    }
    logic_encryption_sha256_init();
    logic_encryption_sha256_update(request->rpID, strnlen(request->rpID, FIDO2_CREDENTIAL_ID_LENGTH));
    logic_encryption_sha256_final(auth_data_header.rpID_hash);

    confirmation = fido2_prompt_user("Confirm auth?", TRUE, 10);

    if (confirmation == FIDO2_SUCCESS)
    {
        auth_data_header.flags |= FIDO2_UP_BIT; //User present
        auth_data_header.flags |= FIDO2_UV_BIT; //User verified
        if (request->new_cred)
        {
            auth_data_header.flags |= FIDO2_AT_BIT; //Attested credential data attached
        }
        response->error_code = FIDO2_SUCCESS;
    }
    else
    {
        auth_data_header.flags &= ~FIDO2_UP_BIT;
        auth_data_header.flags &= ~FIDO2_UV_BIT;
        auth_data_header.flags &= ~FIDO2_AT_BIT;
        response->error_code = (uint8_t) FIDO2_OPERATION_DENIED;
        return response->error_code;
    }

    //Create public key from stored private key
    logic_encryption_ecc256_derive_public_key(rec->priv_key, &pub_key);

    //Update credential "count"
    //Update on every get_assertion/get_next_assertion
    rec->count = fido2_update_credential_count(rec->count);

    fido2_update_credential_in_db(rec);

    //Create attestation signature
    fido2_set_attest_sign_count(rec->count, &auth_data_header.sign_count);

    logic_encryption_ecc256_load_key(rec->priv_key);
    fido2_calc_attestation_signature((uint8_t const *) &auth_data_header, sizeof(auth_data_header), request->client_data_hash, response->attest_sig);

    //Fill out response object
    memcpy(response->tag, rec->tag, FIDO2_CREDENTIAL_ID_LENGTH);
    memcpy(response->user_ID, rec->user_ID, FIDO2_USER_ID_LEN);
    memcpy(response->rpID_hash, auth_data_header.rpID_hash, FIDO2_RPID_HASH_LEN);
    response->count_BE = auth_data_header.sign_count;
    memcpy(response->pub_key_x, pub_key.x, FIDO2_PUB_KEY_X_LEN);
    memcpy(response->pub_key_y, pub_key.y, FIDO2_PUB_KEY_Y_LEN);
    //Attest signature already filled out above when calculating attestation signature
    memcpy(response->aaguid, FIDO2_MINIBLE_AAGUID, FIDO2_AAGUID_LEN);
    //We no longer have the credential ID. Not needed for assertion
    response->cred_ID_len = 0;
    response->flags = auth_data_header.flags;
    //error code already set
    return response->error_code;
}

/*! \fn     fido2_process_make_auth_data(fido2_make_auth_data_req_message_t const *request, fido2_make_auth_data_rsp_message_t *response)
*   \brief  Make the authentication data. This esentially creates the key pair and stores
*           the new record in the DB
*   \param  incoming request messsage
*   \param  outgoing response message
*   \return void
*/
void fido2_process_make_auth_data(fido2_make_auth_data_req_message_t const* request, fido2_make_auth_data_rsp_message_t* response)
{
    memset(response, 0, sizeof(*response));

    if (request->new_cred == FIDO2_NEW_CREDENTIAL)
    {
        fido2_make_auth_data_new_cred(request, response);
    }
    else
    {
        fido2_make_auth_data_existing_cred(request, response);
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

//Always get the first credential for a rpID.
static credential_DB_rec_t *fido2_get_credential(uint8_t const *rpID, uint8_t const *tag)
{
    //TODO: Debug impl
    uint32_t i;

    for(i = 0; i < MAX_CRED; ++i)
    {
        if (memcmp(cred_store[i].rpID, rpID, FIDO2_RPID_LEN) == 0)
        {
            return &cred_store[i];
        }
    }
    return NULL;
}

