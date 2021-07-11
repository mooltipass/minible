/*!  \file     logic_fido2.c
*    \brief    All functions for FIDO2!
*    Created:  15/10/2019
*    Author:   0x0ptr@pm.me
*
*    Code inspired/reused from Solo project.
*/
#ifdef EMULATOR_BUILD
#ifndef WIN32
#include <arpa/inet.h>
#endif
#endif
#include <stdint.h>
#include <string.h>
#include "comms_aux_mcu_defines.h"
#include "fido2_values_defines.h"
#include "logic_encryption.h"
#include "platform_defines.h"
#include "logic_security.h"
#include "logic_database.h"
#include "gui_dispatcher.h"
#include "comms_aux_mcu.h"
#include "logic_device.h"
#include "driver_timer.h"
#include "logic_fido2.h"
#include "gui_prompts.h"
#include "logic_user.h"
#include "nodemgmt.h"
#include "text_ids.h"
#include "defines.h"
#include "utils.h"
#include "rng.h"
#include "main.h"
/* Mini BLE aaguid */
uint8_t fido2_minible_aaguid[16] = {0x6d,0xb0,0x42,0xd0,0x61,0xaf,0x40,0x4c,0xa8,0x87,0xe7,0x2e,0x09,0xba,0x7e,0xb4};


/*! \fn     logic_fido2_calc_attestation_signature(uint8_t const* data, int datalen, uint8_t const* client_data_hash, uint8_t* sigbuf, uint16_t sigbuflen)
*   \brief  Calculate the attestation signature
*   \param  data to calculate signature over
*   \param  length of data
*   \param  hash of client data
*   \param  out buffer with signature
*   \param  size of signature buffer
*   \return void
*/
static void logic_fido2_calc_attestation_signature(uint8_t const* data, int datalen, uint8_t const* client_data_hash, uint8_t* sigbuf, uint16_t sigbuflen)
{
    uint8_t hashbuf[SHA256_OUTPUT_LENGTH];

    logic_encryption_sha256_init();
    logic_encryption_sha256_update(data, datalen);
    logic_encryption_sha256_update(client_data_hash, FIDO2_CLIENT_DATA_HASH_LEN);
    logic_encryption_sha256_final(hashbuf);

    logic_encryption_ecc256_sign(hashbuf, sigbuf, sigbuflen);
}

/*! \fn     logic_fido2_cbor_encode_public_key(uint8_t* buf, uint32_t bufLen, ecc256_pub_key const* pub_key)
*   \brief  CBOR encoder for the public key. All cbor encoding and decoding belongs to aux_mcu.
*           Unfortunately, to be able to create attestation signature we have to encode the public key
*           in cbor format so we have to do some cbor encoding on main_mcu also.
*           Essentially the same as Solo function ctap_add_coseKey() but
*           removed error checking and pass in buffer instead of the cbor encoder object
*           in addition to hardcoding algtype.
*   \param  buffer for encoded output
*   \param  buffer length
*   \param  public key
*   \return Number of bytes written in the buffer (77)
*/
static uint32_t logic_fido2_cbor_encode_public_key(uint8_t* buf, uint32_t bufLen, ecc256_pub_key const* pub_key)
{
    uint16_t output_data_length = 0;
    
    /* Ok, I know it's cheating... but if you read all functions descriptions below you'll see we know already the size required */
    if (bufLen < (1+1+1+1+1+1+1+1+34+1+34))
    {
        main_reboot();
    }

    /* Store the bytes ! */
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

/*! \fn     logic_fido2_process_exclude_list_item(fido2_auth_cred_req_message_t* request)
*   \brief  Process Exclude list check from aux_mcu.
*           Checks if tag already exists. Returns 1 if credential exists or 0
*           otherwise. If tag exists prompt the user and wait for user ack.
*   \param  incoming messsage request
*   \return void
*/
void logic_fido2_process_exclude_list_item(fido2_auth_cred_req_message_t* request)
{
    /* Input sanitation & buffer for UTF8 to Unicode BMP conversion */
    cust_char_t rp_id_copy[MEMBER_ARRAY_SIZE(parent_data_node_t, service)];
    request->rpID[MEMBER_SIZE(fido2_auth_cred_req_message_t, rpID)-1] = 0;
    memset(rp_id_copy, 0, sizeof(rp_id_copy));
    
    /* Try to convert to unicode BMP */
    int16_t rpid_conv_length = utils_utf8_string_to_bmp_string(request->rpID, rp_id_copy, MEMBER_SIZE(fido2_auth_cred_req_message_t, rpID), ARRAY_SIZE(rp_id_copy));
    
    /* Did the conversion go badly? */
    if (rpid_conv_length < 0)
    {
        aux_mcu_message_t* temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_FIDO2);
        temp_tx_message_pt->fido2_message.message_type = AUX_MCU_FIDO2_AUTH_CRED_RSP;
        temp_tx_message_pt->payload_length1 = sizeof(fido2_message_t);
        comms_aux_mcu_send_message(temp_tx_message_pt);
        return;
    }
    
    /* Does service already exist? */
    uint16_t parent_address = logic_database_search_service(rp_id_copy, COMPARE_MODE_MATCH, TRUE, NODEMGMT_WEBAUTHN_CRED_TYPE_ID);
    
    /* Service doesn't exist, deny request with a variable timeout for privacy concerns */
    if (parent_address == NODE_ADDR_NULL)
    {
        /* From 1s to 3s */
        timer_delay_ms(1000 + (rng_get_random_uint16_t()&0x07FF));
        aux_mcu_message_t* temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_FIDO2);
        temp_tx_message_pt->fido2_message.message_type = AUX_MCU_FIDO2_AUTH_CRED_RSP;
        temp_tx_message_pt->payload_length1 = sizeof(fido2_message_t);
        comms_aux_mcu_send_message(temp_tx_message_pt);
        return;
    }
    
    /* Look for credential ID */
    uint16_t child_address = logic_database_search_webauthn_credential_id_in_service(parent_address, request->cred_ID.tag);
    
    /* Static asserts */
    _Static_assert(MEMBER_SIZE(fido2_auth_cred_rsp_message_t, user_handle) >= MEMBER_SIZE(child_webauthn_node_t, user_handle), "user handle size not big enough");
    
    /* Check for existing login */
    if (child_address == NODE_ADDR_NULL)
    {
        /* From 3s to 7s, do not leak information */
        timer_delay_ms(3000 + (rng_get_random_uint16_t()&0x0FFF));
        aux_mcu_message_t* temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_FIDO2);
        temp_tx_message_pt->fido2_message.message_type = AUX_MCU_FIDO2_AUTH_CRED_RSP;
        temp_tx_message_pt->payload_length1 = sizeof(fido2_message_t);
        comms_aux_mcu_send_message(temp_tx_message_pt);
        return;
    }
    else
    {
        /* Wait for user ACK */
        cust_char_t* display_cred_prompt_text;
        custom_fs_get_string_from_file(CRED_ALREAD_PRESENT_TEXT_ID, &display_cred_prompt_text, TRUE);
        confirmationText_t prompt_object = {.lines[0] = display_cred_prompt_text};
        gui_prompts_ask_for_confirmation(1, &prompt_object, FALSE, TRUE, FALSE);

        /* Back to current screen */
        gui_dispatcher_get_back_to_current_screen();

        /* Send answer */
        aux_mcu_message_t* temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_FIDO2);
        temp_tx_message_pt->fido2_message.message_type = AUX_MCU_FIDO2_AUTH_CRED_RSP;
        temp_tx_message_pt->payload_length1 = sizeof(fido2_message_t);
        logic_database_get_webauthn_userhandle_for_address(child_address, temp_tx_message_pt->fido2_message.fido2_auth_cred_rsp_message.user_handle, &temp_tx_message_pt->fido2_message.fido2_auth_cred_rsp_message.user_handle_len);
        memcpy(&temp_tx_message_pt->fido2_message.fido2_auth_cred_rsp_message.cred_ID, &request->cred_ID, sizeof(temp_tx_message_pt->fido2_message.fido2_auth_cred_rsp_message.cred_ID));
        temp_tx_message_pt->fido2_message.fido2_auth_cred_rsp_message.result = FIDO2_CREDENTIAL_EXISTS;
        comms_aux_mcu_send_message(temp_tx_message_pt);
    }
}

/*! \fn     logic_fido2_process_make_credential(fido2_make_credential_req_message_t* request)
*   \brief  Make a new credential. This essentially creates the key pair and stores the new record in the DB
*   \param  incoming request message
*/
void logic_fido2_process_make_credential(fido2_make_credential_req_message_t* request)
{
    /* Local vars */
    uint8_t private_key[FIDO2_PRIV_KEY_LEN];
    attested_data_t attested_data;
    ecc256_pub_key pub_key;
    uint8_t user_handle_len;

    /* Clear attested data that will be signed later */
    memset(&attested_data, 0, sizeof(attested_data));
    
    /* Input sanitation: displayable fields (eg strings) in webauthn spec are rpID / user name / display name */
    request->display_name[MEMBER_SIZE(fido2_make_credential_req_message_t, display_name)-1] = 0;
    request->user_name[MEMBER_SIZE(fido2_make_credential_req_message_t, user_name)-1] = 0;
    request->rpID[MEMBER_SIZE(fido2_make_credential_req_message_t, rpID)-1] = 0;
    
    /* Local copies as the contents of the request message may get overwritten later */
    uint8_t client_hash_copy[MEMBER_ARRAY_SIZE(fido2_make_credential_req_message_t, client_data_hash)];
    cust_char_t display_name_copy[MEMBER_ARRAY_SIZE(child_webauthn_node_t, display_name)];
    uint8_t user_handle_copy[MEMBER_ARRAY_SIZE(child_webauthn_node_t, user_handle)];
    cust_char_t user_name_copy[MEMBER_ARRAY_SIZE(child_webauthn_node_t, user_name)];
    cust_char_t rp_id_copy[MEMBER_ARRAY_SIZE(parent_data_node_t, service)];
    
    /* Zero out that stuff */
    memset(display_name_copy, 0, sizeof(display_name_copy));
    memset(client_hash_copy, 0, sizeof(client_hash_copy));
    memset(user_handle_copy, 0, sizeof(user_handle_copy));
    memset(user_name_copy, 0, sizeof(user_name_copy));
    memset(rp_id_copy, 0, sizeof(rp_id_copy));
    
    /* Sanity check */
    _Static_assert(sizeof(user_handle_copy) <= sizeof(request->user_handle), "user id is too large");

    /* Sanitize user_handle_len to prevent overflow */
    if (request->user_handle_len > sizeof(user_handle_copy))
    {
        request->user_handle_len = sizeof(user_handle_copy);
    }

    /* Copies */
    memcpy(client_hash_copy, request->client_data_hash, sizeof(client_hash_copy));
    memcpy(user_handle_copy, request->user_handle, request->user_handle_len);
    user_handle_len = request->user_handle_len;
    
    /* Conversions from UTF8 to BMP (who stores emoticons anyway....) */
    int16_t display_name_conv_length = utils_utf8_string_to_bmp_string(request->display_name, display_name_copy, MEMBER_SIZE(fido2_make_credential_req_message_t, display_name), ARRAY_SIZE(display_name_copy));
    int16_t user_name_conv_length = utils_utf8_string_to_bmp_string(request->user_name, user_name_copy, MEMBER_SIZE(fido2_make_credential_req_message_t, user_name), ARRAY_SIZE(user_name_copy));
    int16_t rpid_conv_length = utils_utf8_string_to_bmp_string(request->rpID, rp_id_copy, MEMBER_SIZE(fido2_make_credential_req_message_t, rpID), ARRAY_SIZE(rp_id_copy));
    
    /* Has the time been set? */
    if (logic_device_is_time_set() == FALSE)
    {
        /* Display warning */
        gui_prompts_display_information_on_screen_and_wait(TIME_NOT_SET_TEXT_ID, DISP_MSG_WARNING, FALSE);
        gui_dispatcher_get_back_to_current_screen();
        
        /* Send answer */
        aux_mcu_message_t* temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_FIDO2);
        temp_tx_message_pt->fido2_message.fido2_make_credential_rsp_message.error_code = FIDO2_OPERATION_DENIED;
        temp_tx_message_pt->fido2_message.message_type = AUX_MCU_FIDO2_MC_RSP;
        temp_tx_message_pt->payload_length1 = sizeof(fido2_message_t);
        comms_aux_mcu_send_message(temp_tx_message_pt);
        return;
    }
    
    /* Did any conversion go badly? */
    if ((display_name_conv_length < 0) || (user_name_conv_length < 0) || (rpid_conv_length < 0))
    {
        aux_mcu_message_t* temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_FIDO2);
        temp_tx_message_pt->fido2_message.fido2_make_credential_rsp_message.error_code = FIDO2_STORAGE_EXHAUSTED;
        temp_tx_message_pt->fido2_message.message_type = AUX_MCU_FIDO2_MC_RSP;
        temp_tx_message_pt->payload_length1 = sizeof(fido2_message_t);
        comms_aux_mcu_send_message(temp_tx_message_pt);
        return;
    }
    
    /* Check for logged in user first */
    if (logic_security_is_smc_inserted_unlocked() == FALSE)
    {
        aux_mcu_message_t* temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_FIDO2);
        temp_tx_message_pt->fido2_message.fido2_make_credential_rsp_message.error_code = FIDO2_USER_NOT_PRESENT;
        temp_tx_message_pt->fido2_message.message_type = AUX_MCU_FIDO2_MC_RSP;
        temp_tx_message_pt->payload_length1 = sizeof(fido2_message_t);
        comms_aux_mcu_send_message(temp_tx_message_pt);
        return;
    }
    
    /* Create credential ID: random bytes */
    rng_fill_array(attested_data.cred_ID.tag, sizeof(attested_data.cred_ID.tag));

    /* Create encryption key pair */
    logic_encryption_ecc256_generate_private_key(private_key, (uint16_t)sizeof(private_key));
    
    /* Try to store new credential */
    fido2_return_code_te temp_return = logic_user_store_webauthn_credential(rp_id_copy, user_handle_copy, user_handle_len, user_name_copy, display_name_copy, private_key, attested_data.cred_ID.tag);

    /* Success? */
    if (temp_return == FIDO2_SUCCESS)
    {
        attested_data.auth_data_header.flags |= FIDO2_UP_BIT;   // User present
        attested_data.auth_data_header.flags |= FIDO2_UV_BIT;   // User verified
        attested_data.auth_data_header.flags |= FIDO2_AT_BIT;   // Attested credential data attached
    }
    else
    {
        aux_mcu_message_t* temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_FIDO2);
        temp_tx_message_pt->fido2_message.fido2_make_credential_rsp_message.error_code = temp_return;
        temp_tx_message_pt->fido2_message.message_type = AUX_MCU_FIDO2_MC_RSP;
        temp_tx_message_pt->payload_length1 = sizeof(fido2_message_t);
        comms_aux_mcu_send_message(temp_tx_message_pt);
        return;
    }
    
    /**************************/
    /* Prepare message answer */
    /**************************/
    aux_mcu_message_t* temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_FIDO2);
    temp_tx_message_pt->fido2_message.fido2_make_credential_rsp_message.error_code = FIDO2_SUCCESS;
    temp_tx_message_pt->fido2_message.message_type = AUX_MCU_FIDO2_MC_RSP;
    temp_tx_message_pt->payload_length1 = sizeof(fido2_message_t);
    
    /********************************/
    /* Create attestation signature */
    /********************************/
    
    /* 1) Authenticated data header */
    logic_encryption_sha256_init();                                                                 // Init SHA256 engine
    logic_encryption_sha256_update(request->rpID, utils_u8strnlen(request->rpID, FIDO2_RPID_LEN));  // Update with our RPID
    logic_encryption_sha256_final(attested_data.auth_data_header.rpID_hash);                        // Compute final hash
    attested_data.auth_data_header.sign_count = cpu_to_be32(1);                                     // First signature, woot!
    // Flags have been previously set
    
    /* 2) Attestation header */
    memcpy(attested_data.attest_header.aaguid, fido2_minible_aaguid, sizeof(fido2_minible_aaguid)); // Copy our AAGUID
    attested_data.attest_header.cred_len_l = FIDO2_CREDENTIAL_ID_LENGTH & 0x00FF;                   // Credential ID length
    attested_data.attest_header.cred_len_h = (FIDO2_CREDENTIAL_ID_LENGTH & 0xFF00) >> 8;            // Credential ID length
    
    /* 3) Credential ID, previously set with random values */
    
    /* 4) Encoded public key + generate signature */
    logic_encryption_ecc256_load_key(private_key);
    logic_encryption_ecc256_derive_public_key(private_key, &pub_key);
    attested_data.enc_PK_len = logic_fido2_cbor_encode_public_key(attested_data.enc_pub_key, sizeof(attested_data.enc_pub_key), &pub_key);
    logic_fido2_calc_attestation_signature((uint8_t const *)&attested_data, sizeof(attested_data) - sizeof(attested_data.enc_pub_key) + attested_data.enc_PK_len - sizeof(attested_data.enc_PK_len), request->client_data_hash, temp_tx_message_pt->fido2_message.fido2_make_credential_rsp_message.attest_sig, sizeof(temp_tx_message_pt->fido2_message.fido2_make_credential_rsp_message.attest_sig));
    
    /*****************/
    /* Sanity checks */
    /*****************/
    _Static_assert(sizeof(temp_tx_message_pt->fido2_message.fido2_make_credential_rsp_message.tag) == sizeof(attested_data.cred_ID.tag), "tag size is too large");
    _Static_assert(sizeof(temp_tx_message_pt->fido2_message.fido2_make_credential_rsp_message.rpID_hash) == sizeof(attested_data.auth_data_header.rpID_hash), "rpid hash is too large");
    _Static_assert(sizeof(temp_tx_message_pt->fido2_message.fido2_make_credential_rsp_message.pub_key_x) == sizeof(pub_key.x), "pub key x is too large");
    _Static_assert(sizeof(temp_tx_message_pt->fido2_message.fido2_make_credential_rsp_message.pub_key_y) == sizeof(pub_key.y), "pub key y is too large");
    _Static_assert(sizeof(temp_tx_message_pt->fido2_message.fido2_make_credential_rsp_message.aaguid) == sizeof(attested_data.attest_header.aaguid), "aaguid is too large");

    /*************************/
    /* Create answer to host */
    /*************************/
    memcpy(temp_tx_message_pt->fido2_message.fido2_make_credential_rsp_message.tag, attested_data.cred_ID.tag, sizeof(attested_data.cred_ID.tag));
    memcpy(temp_tx_message_pt->fido2_message.fido2_make_credential_rsp_message.rpID_hash, attested_data.auth_data_header.rpID_hash, sizeof(attested_data.auth_data_header.rpID_hash));
    temp_tx_message_pt->fido2_message.fido2_make_credential_rsp_message.count_BE = attested_data.auth_data_header.sign_count;
    temp_tx_message_pt->fido2_message.fido2_make_credential_rsp_message.flags = attested_data.auth_data_header.flags;
    memcpy(temp_tx_message_pt->fido2_message.fido2_make_credential_rsp_message.pub_key_x, pub_key.x, sizeof(pub_key.x));
    memcpy(temp_tx_message_pt->fido2_message.fido2_make_credential_rsp_message.pub_key_y, pub_key.y, sizeof(pub_key.y));
    // Attest signature already filled out above when calculating attestation signature
    memcpy(temp_tx_message_pt->fido2_message.fido2_make_credential_rsp_message.aaguid, attested_data.attest_header.aaguid, sizeof(attested_data.attest_header.aaguid));
    temp_tx_message_pt->fido2_message.fido2_make_credential_rsp_message.cred_ID_len = sizeof(attested_data.cred_ID.tag);
    comms_aux_mcu_send_message(temp_tx_message_pt);
    
    //Clear private key to limit chance of leaking key
    memset(private_key, 0, sizeof(private_key));
}


/*! \fn     logic_fido2_process_get_assertion(fido2_get_assertion_req_message_t* request)
*   \brief  Process an assertion for a credential
*   \param  incoming request message
*/
void logic_fido2_process_get_assertion(fido2_get_assertion_req_message_t* request)
{
    /* Local vars */
    uint8_t credential_id[MEMBER_ARRAY_SIZE(child_webauthn_node_t, credential_id)];
    uint8_t user_handle[MEMBER_ARRAY_SIZE(child_webauthn_node_t, user_handle)];
    cust_char_t rp_id_copy[MEMBER_ARRAY_SIZE(parent_data_node_t, service)];
    uint8_t private_key[FIDO2_PRIV_KEY_LEN];
    auth_data_header_t auth_data_header;
    uint32_t temp_sign_count;
    uint8_t user_handle_len;
    ecc256_pub_key pub_key;
    
    /* Zero out that stuff */
    memset(&auth_data_header, 0, sizeof(auth_data_header));
    memset(credential_id, 0, sizeof(credential_id));
    memset(user_handle, 0, sizeof(user_handle));
    memset(rp_id_copy, 0, sizeof(rp_id_copy));
    user_handle_len = 0;

    /* Input sanitation */
    request->rpID[MEMBER_SIZE(fido2_get_assertion_req_message_t, rpID)-1] = 0;
    
    /* Conversions from UTF8 to BMP (who stores emoticons anyway....) */
    int16_t rpid_conv_length = utils_utf8_string_to_bmp_string(request->rpID, rp_id_copy, MEMBER_SIZE(fido2_get_assertion_req_message_t, rpID), ARRAY_SIZE(rp_id_copy));
    
    /* Did the conversion go badly? */
    if (rpid_conv_length < 0)
    {
        aux_mcu_message_t* temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_FIDO2);
        temp_tx_message_pt->fido2_message.fido2_get_assertion_rsp_message.error_code = FIDO2_NO_CREDENTIALS;
        temp_tx_message_pt->fido2_message.message_type = AUX_MCU_FIDO2_GA_RSP;
        temp_tx_message_pt->payload_length1 = sizeof(fido2_message_t);
        comms_aux_mcu_send_message(temp_tx_message_pt);
        return;
    }
    
    /* Has the time been set? */
    if (logic_device_is_time_set() == FALSE)
    {
        /* Display warning */
        gui_prompts_display_information_on_screen_and_wait(TIME_NOT_SET_TEXT_ID, DISP_MSG_WARNING, FALSE);
        gui_dispatcher_get_back_to_current_screen();
        
        /* Send answer */
        aux_mcu_message_t* temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_FIDO2);
        temp_tx_message_pt->fido2_message.fido2_make_credential_rsp_message.error_code = FIDO2_OPERATION_DENIED;
        temp_tx_message_pt->fido2_message.message_type = AUX_MCU_FIDO2_GA_RSP;
        temp_tx_message_pt->payload_length1 = sizeof(fido2_message_t);
        comms_aux_mcu_send_message(temp_tx_message_pt);
        return;
    }
    
    /* Check for logged in user first */
    if (logic_security_is_smc_inserted_unlocked() == FALSE)
    {
        aux_mcu_message_t* temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_FIDO2);
        temp_tx_message_pt->fido2_message.fido2_get_assertion_rsp_message.error_code = FIDO2_USER_NOT_PRESENT;
        temp_tx_message_pt->fido2_message.message_type = AUX_MCU_FIDO2_GA_RSP;
        temp_tx_message_pt->payload_length1 = sizeof(fido2_message_t);
        comms_aux_mcu_send_message(temp_tx_message_pt);
        return;
    }
    
    /* Compute RPid Hash */
    logic_encryption_sha256_init();
    logic_encryption_sha256_update(request->rpID, utils_u8strnlen(request->rpID, sizeof(request->rpID)));
    logic_encryption_sha256_final(auth_data_header.rpID_hash);
    
    /* Ask for user permission, automatically pre increment signing counter upon success recall */
    fido2_return_code_te temp_return = FIDO2_SUCCESS;
    temp_return = logic_user_get_webauthn_credential_key_for_rp(rp_id_copy, user_handle, &user_handle_len, credential_id, private_key, &temp_sign_count, request->allow_list.tag, request->allow_list.len, request->flags);

    /* Success? */
    if (temp_return == FIDO2_SUCCESS)
    {
        auth_data_header.flags &= ~FIDO2_AT_BIT;
        if ( (request->flags & FIDO2_GA_FLAG_SILENT) == FIDO2_GA_FLAG_SILENT)
        {
            /*
             * A Silent Asserting (SA) is a request from the host OS that is used to verify that this is the authenticator that has the requested account.
             * The UP flag on the request is set to 0 on a SA and thus the authenticator is not supposed to prompt the user. This is to avoid the user
             * being prompted twice (once for SA and one for the real Assertion).
             * The SA will not be used for an actual login. A second request will come that is not SA and that will be used for the actual login.
             * To make sure the request is not valid set the sign count to 0 and clear any UV and UP flag. According to FIDO2 specification an RP SHALL
             * test for at least UP=1 before allowing login with this token. Also, sign count cannot go backwards so 0 is not a valid sign count.
             */
            auth_data_header.sign_count = 0;
            auth_data_header.flags &= ~FIDO2_UP_BIT;
            auth_data_header.flags &= ~FIDO2_UV_BIT;
        }
        else
        {
            auth_data_header.sign_count = cpu_to_be32(temp_sign_count);
            auth_data_header.flags |= FIDO2_UP_BIT;
            auth_data_header.flags |= FIDO2_UV_BIT;
        }
    }
    else
    {
        aux_mcu_message_t* temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_FIDO2);
        temp_tx_message_pt->fido2_message.fido2_get_assertion_rsp_message.error_code = temp_return;
        temp_tx_message_pt->fido2_message.message_type = AUX_MCU_FIDO2_GA_RSP;
        temp_tx_message_pt->payload_length1 = sizeof(fido2_message_t);
        comms_aux_mcu_send_message(temp_tx_message_pt);
        return;
    }
    
    /**************************/
    /* Prepare message answer */
    /**************************/
    aux_mcu_message_t* temp_tx_message_pt = comms_aux_mcu_get_empty_packet_ready_to_be_sent(AUX_MCU_MSG_TYPE_FIDO2);
    temp_tx_message_pt->fido2_message.fido2_get_assertion_rsp_message.error_code = FIDO2_SUCCESS;
    temp_tx_message_pt->fido2_message.message_type = AUX_MCU_FIDO2_GA_RSP;
    temp_tx_message_pt->payload_length1 = sizeof(fido2_message_t);

    /* Sign header */
    logic_encryption_ecc256_load_key(private_key);
    logic_encryption_ecc256_derive_public_key(private_key, &pub_key);
    logic_fido2_calc_attestation_signature((uint8_t const *)&auth_data_header, sizeof(auth_data_header), request->client_data_hash, temp_tx_message_pt->fido2_message.fido2_get_assertion_rsp_message.attest_sig, sizeof(temp_tx_message_pt->fido2_message.fido2_get_assertion_rsp_message.attest_sig));

    /*****************/
    /* Sanity checks */
    /*****************/
    _Static_assert(sizeof(temp_tx_message_pt->fido2_message.fido2_get_assertion_rsp_message.tag) == sizeof(credential_id), "tag size is too large");
    _Static_assert(sizeof(temp_tx_message_pt->fido2_message.fido2_get_assertion_rsp_message.user_handle) >= sizeof(user_handle), "user handle is too large");
    _Static_assert(sizeof(temp_tx_message_pt->fido2_message.fido2_get_assertion_rsp_message.rpID_hash) == sizeof(auth_data_header.rpID_hash), "rpid hash is too large");
    _Static_assert(sizeof(temp_tx_message_pt->fido2_message.fido2_get_assertion_rsp_message.aaguid) == sizeof(fido2_minible_aaguid), "aaguid is too large");
    

    /* Sanitize user_handle_len to prevent overflow */
    if (user_handle_len > MEMBER_SIZE(fido2_get_assertion_rsp_message_t, user_handle))
    {
        user_handle_len = MEMBER_SIZE(fido2_get_assertion_rsp_message_t, user_handle);
    }

    /*************************/
    /* Create answer to host */
    /*************************/
    memcpy(temp_tx_message_pt->fido2_message.fido2_get_assertion_rsp_message.tag, credential_id, sizeof(credential_id));
    memcpy(temp_tx_message_pt->fido2_message.fido2_get_assertion_rsp_message.user_handle, user_handle, user_handle_len);
    temp_tx_message_pt->fido2_message.fido2_get_assertion_rsp_message.user_handle_len = user_handle_len;
    memcpy(temp_tx_message_pt->fido2_message.fido2_get_assertion_rsp_message.rpID_hash, auth_data_header.rpID_hash, sizeof(auth_data_header.rpID_hash));
    temp_tx_message_pt->fido2_message.fido2_get_assertion_rsp_message.count_BE = auth_data_header.sign_count;
    // Attest signature already filled out above when calculating attestation signature
    memcpy(temp_tx_message_pt->fido2_message.fido2_get_assertion_rsp_message.aaguid, fido2_minible_aaguid, sizeof(temp_tx_message_pt->fido2_message.fido2_get_assertion_rsp_message.aaguid));
    // We no longer have the credential ID. Not needed for assertion
    temp_tx_message_pt->fido2_message.fido2_get_assertion_rsp_message.flags = auth_data_header.flags;
    comms_aux_mcu_send_message(temp_tx_message_pt);
    
    //Clear private key to limit chance of leaking key
    memset(private_key, 0, sizeof(private_key));
    
    return;
}
