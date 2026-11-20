// Copyright 2019 SoloKeys Developers
//
// Licensed under the Apache License, Version 2.0, <LICENSE-APACHE or
// http://apache.org/licenses/LICENSE-2.0> or the MIT license <LICENSE-MIT or
// http://opensource.org/licenses/MIT>, at your option. This file may not be
// copied, modified, or distributed except according to those terms.
//
// Modified by MiniBLE developers
// Modifications:
// -Removed code related to U2F
// -Removed code related to PIN
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "solo_compat_layer.h"
#include "comms_main_mcu.h"
#include "comms_raw_hid.h"
#include "driver_timer.h"
#include "ctap_errors.h"
#include "ctap_parse.h"
#include "cose_key.h"
#include "ctaphid.h"
#include "ctap.h"
#include "cbor.h"

uint8_t ctap_get_info(CborEncoder * encoder)
{
    int ret;
    CborEncoder array;
    CborEncoder map;
    CborEncoder options;

    ret = cbor_encoder_create_map(encoder, &map, 4);
    check_ret(ret);
    {

        ret = cbor_encode_uint(&map, RESP_versions);     //  versions key
        check_ret(ret);
        {
            ret = cbor_encoder_create_array(&map, &array, 1);
            check_ret(ret);
            {
                ret = cbor_encode_text_stringz(&array, "FIDO_2_0");
                check_ret(ret);
            }
            ret = cbor_encoder_close_container(&map, &array);
            check_ret(ret);
        }

        ret = cbor_encode_uint(&map, RESP_aaguid);
        check_ret(ret);
        {
            ret = cbor_encode_byte_string(&map, CTAP_AAGUID, 16);
            check_ret(ret);
        }

        ret = cbor_encode_uint(&map, RESP_options);
        check_ret(ret);
        {
            ret = cbor_encoder_create_map(&map, &options,4);
            check_ret(ret);
            {
                ret = cbor_encode_text_string(&options, "rk", 2);
                check_ret(ret);
                {
                    ret = cbor_encode_boolean(&options, 1);     // Capable of storing keys locally
                    check_ret(ret);
                }

                ret = cbor_encode_text_string(&options, "up", 2);
                check_ret(ret);
                {
                    ret = cbor_encode_boolean(&options, 1);     // Capable of testing user presence
                    check_ret(ret);
                }

                ret = cbor_encode_text_string(&options, "uv", 2);
                check_ret(ret);
                {
                    ret = cbor_encode_boolean(&options, 1);
                     check_ret(ret);
                }

                ret = cbor_encode_text_string(&options, "plat", 4);
                check_ret(ret);
                {
                    ret = cbor_encode_boolean(&options, 0);     // Not attached to platform
                    check_ret(ret);
                }
            }
            ret = cbor_encoder_close_container(&map, &options);
            check_ret(ret);
        }

        ret = cbor_encode_uint(&map, RESP_maxMsgSize);
        check_ret(ret);
        {
            ret = cbor_encode_int(&map, CTAP_MAX_MESSAGE_SIZE);
            check_ret(ret);
        }
    }
    ret = cbor_encoder_close_container(encoder, &map);
    check_ret(ret);

    return CTAP1_ERR_SUCCESS;
}

static int ctap_add_cose_key(CborEncoder * cose_key, uint8_t * x, uint8_t * y, uint8_t keyType)
{
    int ret;
    CborEncoder map;
    uint8_t nElements = 5;
    int8_t kty = COSE_KEY_KTY_EC2;
    int8_t alg = COSE_ALG_ES256;
    int8_t crv = COSE_KEY_CRV_P256;

    if (keyType == FIDO2_KEYTYPE_EDDSA)
    {
        nElements = 4;
        kty = COSE_KEY_KTY_OKP;
        alg = COSE_ALG_EDDSA;
        crv = COSE_KEY_CRV_ED25519;
    }

    ret = cbor_encoder_create_map(cose_key, &map, nElements);
    check_ret(ret);


    {
        ret = cbor_encode_int(&map, COSE_KEY_LABEL_KTY);
        check_ret(ret);
        ret = cbor_encode_int(&map, kty);
        check_ret(ret);
    }

    {
        ret = cbor_encode_int(&map, COSE_KEY_LABEL_ALG);
        check_ret(ret);
        ret = cbor_encode_int(&map, alg);
        check_ret(ret);
    }

    {
        ret = cbor_encode_int(&map, COSE_KEY_LABEL_CRV);
        check_ret(ret);
        ret = cbor_encode_int(&map, crv);
        check_ret(ret);
    }


    {
        ret = cbor_encode_int(&map, COSE_KEY_LABEL_X);
        check_ret(ret);
        ret = cbor_encode_byte_string(&map, x, 32);
        check_ret(ret);
    }

    if (keyType == FIDO2_KEYTYPE_ES256)
    {
        ret = cbor_encode_int(&map, COSE_KEY_LABEL_Y);
        check_ret(ret);
        ret = cbor_encode_byte_string(&map, y, 32);
        check_ret(ret);
    }

    ret = cbor_encoder_close_container(cose_key, &map);
    check_ret(ret);

    return 0;
}

static ret_type_te ctap_make_credential_aux_comm(CTAP_requestCommon *common, CTAP_credInfo * credInfo, fido2_make_credential_rsp_message_t *resp_msg)
{
    aux_mcu_message_t* temp_rx_message_pt = comms_main_mcu_get_temp_rx_message_object_pt();
    aux_mcu_message_t* temp_tx_message_pt;
    fido2_make_credential_req_message_t *req_msg;
    ret_type_te ret = RETURN_NOK;

    /* Create message to make authentication data */
    comms_main_mcu_get_empty_packet_ready_to_be_sent(&temp_tx_message_pt, AUX_MCU_MSG_TYPE_FIDO2);

    req_msg = &temp_tx_message_pt->fido2_message.fido2_make_credential_req_message;

    /* Fill message */
    memset(req_msg, 0, sizeof(*req_msg));
    memcpy(req_msg->rpID, common->rp.id, FIDO2_RPID_LEN);
    memcpy(req_msg->user_handle, credInfo->user.id, credInfo->user.id_size);
    req_msg->user_handle_len = credInfo->user.id_size;
    memcpy(req_msg->user_name, credInfo->user.name, FIDO2_USER_NAME_LEN);
    memcpy(req_msg->display_name, credInfo->user.displayName, FIDO2_DISPLAY_NAME_LEN);
    memcpy(req_msg->client_data_hash, common->clientDataHash, FIDO2_CLIENT_DATA_HASH_LEN);
    if (credInfo->COSEAlgorithmIdentifier == COSE_ALG_ES256) {
        req_msg->keyType = FIDO2_KEYTYPE_ES256;
    } else {
        req_msg->keyType = FIDO2_KEYTYPE_EDDSA;
    }
    /* Set length of message */
    temp_tx_message_pt->payload_length1 = sizeof(fido2_message_t);

    /* Set message subtype */
    temp_tx_message_pt->fido2_message.message_type = AUX_MCU_FIDO2_MC_REQ;

    /* Send packet */
    comms_main_mcu_send_message((void*)temp_tx_message_pt, (uint16_t)sizeof(aux_mcu_message_t));

    /* Wait for message from main MCU */
    do
    {
        ctaphid_update_status(2);
        timer_start_timer(TIMER_TIMEOUT_FUNCTS, 50);
        while ((timer_has_timer_expired(TIMER_TIMEOUT_FUNCTS, TRUE) == TIMER_RUNNING) && (ret != RETURN_OK))
        {
            ret = comms_main_mcu_routine(TRUE, AUX_MCU_MSG_TYPE_FIDO2);
        }
    } while (ret != RETURN_OK);

    /* Received message is in temporary buffer */
    memcpy(resp_msg, &temp_rx_message_pt->fido2_message.fido2_make_credential_rsp_message, sizeof(*resp_msg));
    memcpy(credInfo->id.tag, resp_msg->tag, sizeof(credInfo->id.tag));
    return ret;
}

static int ctap_make_credential_auth_data(CTAP_requestCommon *req_common, uint8_t * auth_data_buf, uint32_t * len, CTAP_credInfo * credInfo, uint8_t *sigbuf)
{
    fido2_make_credential_rsp_message_t resp_msg;
    CborEncoder cose_key;
    unsigned int auth_data_sz = sizeof(CTAP_authDataHeader);
    CTAP_authData * authData = (CTAP_authData *)auth_data_buf;
    uint8_t * cose_key_buf = auth_data_buf + sizeof(CTAP_authData);
    int ret;

    if((sizeof(CTAP_authDataHeader)) > *len)
    {
        printf1(TAG_ERR,"assertion fail, auth_data_buf must be at least %d bytes", sizeof(CTAP_authData) - sizeof(CTAP_attestHeader));
        exit(1);
    }

    if (ctap_make_credential_aux_comm(req_common, credInfo, &resp_msg) != RETURN_OK)
    {
        return CTAP2_ERR_TOO_MANY_ELEMENTS;
    }

    if (resp_msg.error_code != SUCCESS)
    {
        switch(resp_msg.error_code)
        {
            case OPERATION_DENIED:
                return CTAP2_ERR_OPERATION_DENIED;
            case USER_NOT_PRESENT:
                return CTAP2_ERR_USER_ACTION_TIMEOUT;
            case STORAGE_EXHAUSTED:
                return CTAP2_ERR_KEY_STORE_FULL;
            case FIDO2_NO_CREDENTIALS:
                return CTAP2_ERR_NO_CREDENTIALS;
            default:
                return CTAP2_ERR_NOT_ALLOWED; //TOOD:?
        };
    }

    memcpy(authData->head.rpIdHash, resp_msg.rpID_hash, sizeof(authData->head.rpIdHash));
    authData->head.flags = resp_msg.flags;
    authData->head.signCount = resp_msg.count_BE;

    memcpy(authData->attest.aaguid, resp_msg.aaguid, sizeof(authData->attest.aaguid));
    authData->attest.credLenH = (resp_msg.cred_ID_len & 0xFF00) >> 8;
    authData->attest.credLenL = resp_msg.cred_ID_len & 0x00FF;

    memcpy(authData->attest.id.tag, resp_msg.tag, sizeof(authData->attest.id.tag));

    memcpy(sigbuf, resp_msg.attest_sig, FIDO2_ATTEST_SIG_LEN); //Used in calling function

    cbor_encoder_init(&cose_key, cose_key_buf, *len - sizeof(CTAP_authData), 0);

    ret = ctap_add_cose_key(&cose_key, resp_msg.pub_key_x, resp_msg.pub_key_y, resp_msg.keyType);
    check_ret(ret);

    auth_data_sz = sizeof(CTAP_authData) + cbor_encoder_get_buffer_size(&cose_key, cose_key_buf);

    *len = auth_data_sz;
    return 0;
}

static ret_type_te ctap_get_assertion_aux_comm(CTAP_requestCommon *common, CTAP_getAssertion *GA, CTAP_credInfo * credInfo, fido2_get_assertion_rsp_message_t *resp_msg)
{
    aux_mcu_message_t* temp_rx_message_pt = comms_main_mcu_get_temp_rx_message_object_pt();
    aux_mcu_message_t* temp_tx_message_pt;
    fido2_get_assertion_req_message_t *req_msg;
    ret_type_te ret = RETURN_NOK;
    uint8_t i;
    uint8_t uv_up = 0;

    /* Create message to make authentication data */
    comms_main_mcu_get_empty_packet_ready_to_be_sent(&temp_tx_message_pt, AUX_MCU_MSG_TYPE_FIDO2);

    req_msg = &temp_tx_message_pt->fido2_message.fido2_get_assertion_req_message;

    /* Fill message */
    memset(req_msg, 0, sizeof(*req_msg));
    memcpy(req_msg->rpID, common->rp.id, FIDO2_RPID_LEN);

    if (GA->credLen > FIDO2_ALLOW_LIST_MAX_SIZE)
    {
        //Programmer error
        printf1(TAG_ERR,"Programmer error. Size of arrays is inconsistent");
        return RETURN_NOK;
    }
    req_msg->allow_list.len = GA->credLen;
    for (i = 0; i < req_msg->allow_list.len; ++i)
    {
        memcpy(&req_msg->allow_list.tag[i], GA->creds[i].id.tag, sizeof(req_msg->allow_list.tag[i]));
    }

    /*
     * Determine whether we should prompt the user or not
     * Prompt under the following conditions:
     * 1. Either UV or UP or both is 1
     * 2. UP missing (case currently for demo.yubico.com/webauthn)
     */
    uv_up = GA->uv + GA->up;
    req_msg->flags = (!uv_up && GA->upPresent == 1) ? FIDO2_GA_FLAG_SILENT : 0;

    memcpy(req_msg->client_data_hash, common->clientDataHash, FIDO2_CLIENT_DATA_HASH_LEN);

    /* Set length of message */
    temp_tx_message_pt->payload_length1 = sizeof(fido2_message_t);

    /* Set message subtype */
    temp_tx_message_pt->fido2_message.message_type = AUX_MCU_FIDO2_GA_REQ;

    /* Send packet */
    comms_main_mcu_send_message((void*)temp_tx_message_pt, (uint16_t)sizeof(aux_mcu_message_t));

    /* Wait for message from main MCU */
    do
    {
        ctaphid_update_status(2);
        timer_start_timer(TIMER_TIMEOUT_FUNCTS, 50);
        while ((timer_has_timer_expired(TIMER_TIMEOUT_FUNCTS, TRUE) == TIMER_RUNNING) && (ret != RETURN_OK))
        {
            ret = comms_main_mcu_routine(TRUE, AUX_MCU_MSG_TYPE_FIDO2);
        }
    } while (ret != RETURN_OK);

    /* Received message is in temporary buffer */
    memcpy(resp_msg, &temp_rx_message_pt->fido2_message.fido2_get_assertion_rsp_message, sizeof(*resp_msg));
    memcpy(credInfo->user.id, resp_msg->user_handle, resp_msg->user_handle_len);
    credInfo->user.id_size = resp_msg->user_handle_len;
    memcpy(credInfo->id.tag, resp_msg->tag, sizeof(credInfo->id.tag));
    if (resp_msg->keyType == FIDO2_KEYTYPE_ES256)
    {
        credInfo->COSEAlgorithmIdentifier = COSE_ALG_ES256;
    }
    else
    {
        credInfo->COSEAlgorithmIdentifier = COSE_ALG_EDDSA;
    }
    return ret;
}

static int ctap_make_get_assertion_auth_data(CTAP_requestCommon *req_common, CTAP_getAssertion *GA, uint8_t * auth_data_buf, uint32_t * len, CTAP_credInfo * credInfo, uint8_t *sigbuf)
{
    fido2_get_assertion_rsp_message_t resp_msg;
    unsigned int auth_data_sz = sizeof(CTAP_authDataHeader);
    CTAP_authData * authData = (CTAP_authData *)auth_data_buf;

    if((sizeof(CTAP_authDataHeader)) > *len)
    {
        printf1(TAG_ERR,"assertion fail, auth_data_buf must be at least %d bytes", sizeof(CTAP_authData) - sizeof(CTAP_attestHeader));
        exit(1);
    }

    if (ctap_get_assertion_aux_comm(req_common, GA, credInfo, &resp_msg) != RETURN_OK)
    {
        return CTAP2_ERR_TOO_MANY_ELEMENTS;
    }

    if (resp_msg.error_code != SUCCESS)
    {
        switch(resp_msg.error_code)
        {
            case OPERATION_DENIED:
                return CTAP2_ERR_OPERATION_DENIED;
            case USER_NOT_PRESENT:
                return CTAP2_ERR_USER_ACTION_TIMEOUT;
            case STORAGE_EXHAUSTED:
                return CTAP2_ERR_KEY_STORE_FULL;
            case FIDO2_NO_CREDENTIALS:
                return CTAP2_ERR_NO_CREDENTIALS;
            default:
                return CTAP2_ERR_NOT_ALLOWED; //TOOD:?
        };
    }

    memcpy(authData->head.rpIdHash, resp_msg.rpID_hash, sizeof(authData->head.rpIdHash));
    authData->head.flags = resp_msg.flags;
    authData->head.signCount = resp_msg.count_BE;

    memcpy(authData->attest.aaguid, resp_msg.aaguid, sizeof(authData->attest.aaguid));
    memcpy(authData->attest.id.tag, resp_msg.tag, sizeof(authData->attest.id.tag));

    memcpy(sigbuf, resp_msg.attest_sig, FIDO2_ATTEST_SIG_LEN); //Used in calling function
    *len = auth_data_sz;
    return 0;
}

/**
 *
 * @param in_sigbuf IN location to deposit signature (must be 64 bytes)
 * @param out_sigder OUT location to deposit der signature (must be 72 bytes)
 * @return length of der signature
 * // FIXME add tests for maximum and minimum length of the input and output
 */
int ctap_encode_der_sig(const uint8_t * const in_sigbuf, uint8_t * const out_sigder)
{
    // Need to caress into dumb der format ..
    uint8_t i;
    uint8_t lead_s = 0;  // leading zeros
    uint8_t lead_r = 0;
    for (i=0; i < 32; i++)
        if (in_sigbuf[i] == 0) lead_r++;
        else break;

    for (i=0; i < 32; i++)
        if (in_sigbuf[i+32] == 0) lead_s++;
        else break;

    int8_t pad_s = ((in_sigbuf[32 + lead_s] & 0x80) == 0x80);
    int8_t pad_r = ((in_sigbuf[0 + lead_r] & 0x80) == 0x80);

    memset(out_sigder, 0, 72);
    out_sigder[0] = 0x30;
    out_sigder[1] = 0x44 + pad_s + pad_r - lead_s - lead_r;

    // R ingredient
    out_sigder[2] = 0x02;
    out_sigder[3 + pad_r] = 0;
    out_sigder[3] = 0x20 + pad_r - lead_r;
    memmove(out_sigder + 4 + pad_r, in_sigbuf + lead_r, 32u - lead_r);

    // S ingredient
    out_sigder[4 + 32 + pad_r - lead_r] = 0x02;
    out_sigder[5 + 32 + pad_r + pad_s - lead_r] = 0;
    out_sigder[5 + 32 + pad_r - lead_r] = 0x20 + pad_s - lead_s;
    memmove(out_sigder + 6 + 32 + pad_r + pad_s - lead_r, in_sigbuf + 32u + lead_s, 32u - lead_s);

    return 0x46 + pad_s + pad_r - lead_r - lead_s;
}

/*
 * Delta from Solo implementation:
 * Removed x5c certificate
 * Signature using credential private key
 */
uint8_t ctap_add_attest_statement(CborEncoder * map, uint8_t * sigder, int len)
{
    int ret;

    CborEncoder stmtmap;


    ret = cbor_encode_int(map,RESP_attStmt);
    check_ret(ret);
    ret = cbor_encoder_create_map(map, &stmtmap, 2);
    check_ret(ret);
    {
        ret = cbor_encode_text_stringz(&stmtmap,"alg");
        check_ret(ret);
        ret = cbor_encode_int(&stmtmap,COSE_ALG_ES256);
        check_ret(ret);
    }
    {
        ret = cbor_encode_text_stringz(&stmtmap,"sig");
        check_ret(ret);
        ret = cbor_encode_byte_string(&stmtmap, sigder, len);
        check_ret(ret);
    }

    ret = cbor_encoder_close_container(map, &stmtmap);
    check_ret(ret);
    return 0;
}

// Return 1 if credential belongs to this token
// MiniBLE:
// Forward message to main_mcu instead and get response.
int ctap_authenticate_credential(struct rpId * rp, CTAP_credentialDescriptor * desc)
{
    aux_mcu_message_t* temp_rx_message_pt = comms_main_mcu_get_temp_rx_message_object_pt();
    aux_mcu_message_t* temp_tx_message_pt;
    fido2_auth_cred_req_message_t* msg;
    fido2_auth_cred_rsp_message_t* rsp_msg;
    ret_type_te ret = RETURN_NOK;

    /* Create message to authenticate a credential */
    comms_main_mcu_get_empty_packet_ready_to_be_sent(&temp_tx_message_pt, AUX_MCU_MSG_TYPE_FIDO2);

    msg = &temp_tx_message_pt->fido2_message.fido2_auth_cred_req_message;

    /* Fill message */
    memset(msg, 0, sizeof(*msg));
    memcpy(msg->rpID, rp->id, FIDO2_RPID_LEN);
    memcpy(msg->cred_ID.tag, desc->id.tag, FIDO2_CREDENTIAL_ID_LENGTH);

    /* Set length of message */
    temp_tx_message_pt->payload_length1 = sizeof(fido2_message_t);

    /* Set message subtype */
    temp_tx_message_pt->fido2_message.message_type = AUX_MCU_FIDO2_AUTH_CRED_REQ;

    /* Send packet */
    comms_main_mcu_send_message((void*)temp_tx_message_pt, (uint16_t)sizeof(aux_mcu_message_t));

    /* Wait for message from main MCU */
    do
    {
        ctaphid_update_status(2);
        timer_start_timer(TIMER_TIMEOUT_FUNCTS, 50);
        while ((timer_has_timer_expired(TIMER_TIMEOUT_FUNCTS, TRUE) == TIMER_RUNNING) && (ret != RETURN_OK))
        {
            ret = comms_main_mcu_routine(TRUE, AUX_MCU_MSG_TYPE_FIDO2);
        }
    } while (ret != RETURN_OK);

    /* Received message is in temporary buffer */
    rsp_msg = &temp_rx_message_pt->fido2_message.fido2_auth_cred_rsp_message;
    return rsp_msg->result;
}

/*
 * Delta from Solo implementation:
 * Signing with credential private key instead of attestation private key
 * Removed anything related to PIN.
 * Sending message to main_mcu to do the crypto work
 */
uint8_t ctap_make_credential(CborEncoder * encoder, uint8_t * request, int length)
{
    CTAP_makeCredential MC;
    int ret;
    unsigned int i;
    uint8_t auth_data_buf[310];
    CTAP_credentialDescriptor * excl_cred = (CTAP_credentialDescriptor *) auth_data_buf;
    uint8_t sigbuf[FIDO2_ATTEST_SIG_LEN];// = auth_data_buf + 32;
    uint8_t sigder[72];// = auth_data_buf + 32 + 64;

    ret = ctap_parse_make_credential(&MC,encoder,request,length);

    if (ret != 0)
    {
        printf2(TAG_ERR,"error, parse_make_credential failed");
        return ret;
    }
    if ((MC.common.paramsParsed & MC_requiredMask) != MC_requiredMask)
    {
        printf2(TAG_ERR,"error, required parameter(s) for makeCredential are missing");
        return CTAP2_ERR_MISSING_PARAMETER;
    }

    //MiniBle does not support pin protocol
    if (MC.common.pinAuthPresent)
    {
        return CTAP2_ERR_PIN_AUTH_INVALID;
    }

    if (MC.up)
    {
        return CTAP2_ERR_INVALID_OPTION;
    }

    /*
     * Special case for Windows 10 (W10).
     * W10 might send a make credential request with "SelectDevice" as the RP/name.
     * W10 uses this to select a device if more than one device is hooked up to the PC.
     * Ignore this request and don't create the credential
     * .dummy: used by some services thinking that the mini BLE is FIDO compatible
     */
    uint8_t rpid_is_SD = (strcmp((char const *) MC.common.rp.id, "SelectDevice") == 0) || (strcmp((char const *) MC.common.rp.id, ".dummy") == 0);
    uint8_t rpname_is_SD = (strcmp((char const *) MC.common.rp.name, "SelectDevice") == 0) || (strcmp((char const *) MC.common.rp.id, ".dummy") == 0);

    if (rpid_is_SD && rpname_is_SD)
    {
        //Silently return success
        printf2(TAG_ERR, "Windows workaround: Silently returning SUCCESS");
        return CTAP1_ERR_SUCCESS;
    }

    // crypto_aes256_init(CRYPTO_TRANSPORT_KEY, NULL);
    for (i = 0; i < MC.excludeListSize; i++)
    {
        ret = parse_credential_descriptor(&MC.excludeList, excl_cred);
        if (ret == CTAP2_ERR_CBOR_UNEXPECTED_TYPE)
        {
            continue;
        }
        check_retr(ret);

        printf1(TAG_GREEN, "checking credId: "); dump_hex1(TAG_GREEN, (uint8_t*) &excl_cred->id, sizeof(CredentialId));

        if (ctap_authenticate_credential(&MC.common.rp, excl_cred))
        {
            printf1(TAG_MC, "Cred %d failed!\r",i);
            return CTAP2_ERR_CREDENTIAL_EXCLUDED;
        }

        ret = cbor_value_advance(&MC.excludeList);
        check_ret(ret);
    }


    CborEncoder map;
    ret = cbor_encoder_create_map(encoder, &map, 3);
    check_ret(ret);

    {
        ret = cbor_encode_int(&map,RESP_fmt);
        check_ret(ret);
        ret = cbor_encode_text_stringz(&map, "packed");
        check_ret(ret);
    }

    uint32_t auth_data_sz = sizeof(auth_data_buf);

    ret = ctap_make_credential_auth_data(&MC.common, auth_data_buf, &auth_data_sz, &MC.credInfo, sigbuf);
    if (ret != CTAP1_ERR_SUCCESS)
    {
        printf1(TAG_ERR, "Error returned from make credential: %d", ret);
        return ret;
    }

    {
        ret = cbor_encode_int(&map,RESP_authData);
        check_ret(ret);
        ret = cbor_encode_byte_string(&map, auth_data_buf, auth_data_sz);
        check_ret(ret);
    }

    //sigbuf was calculated by main_mcu. Convert to DER format
    if (MC.credInfo.COSEAlgorithmIdentifier == COSE_ALG_ES256)
    {
        int sigder_sz = ctap_encode_der_sig(sigbuf, sigder);
        ret = ctap_add_attest_statement(&map, sigder, sigder_sz);
    }
    else
    {
        ret = ctap_add_attest_statement(&map, sigbuf, FIDO2_ATTEST_SIG_LEN);
    }
    check_retr(ret);

    ret = cbor_encoder_close_container(encoder, &map);
    check_ret(ret);
    return CTAP1_ERR_SUCCESS;
}

static uint8_t ctap_add_credential_descriptor(CborEncoder * map, CTAP_credInfo *cred_info)
{
    CborEncoder desc;
    int ret = cbor_encode_int(map, RESP_credential);
    check_ret(ret);

    ret = cbor_encoder_create_map(map, &desc, 2);
    check_ret(ret);

    {
        ret = cbor_encode_text_string(&desc, "id", 2);
        check_ret(ret);

        ret = cbor_encode_byte_string(&desc, (uint8_t*)&cred_info->id, sizeof(cred_info->id.tag));
        check_ret(ret);
    }

    {
        ret = cbor_encode_text_string(&desc, "type", 4);
        check_ret(ret);

        ret = cbor_encode_text_string(&desc, "public-key", 10);
        check_ret(ret);
    }


    ret = cbor_encoder_close_container(map, &desc);
    check_ret(ret);

    return 0;
}

/**
 * Delta from Solo impl:
 * Removed addition of user information other than required user ID
 */
uint8_t ctap_add_user_entity(CborEncoder * map, CTAP_userEntity * user)
{
    CborEncoder entity;
    int ret = cbor_encode_int(map, RESP_publicKeyCredentialUserEntity);
    check_ret(ret);

    ret = cbor_encoder_create_map(map, &entity, 1);
    check_ret(ret);

    {
        ret = cbor_encode_text_string(&entity, "id", 2);
        check_ret(ret);

        ret = cbor_encode_byte_string(&entity, user->id, user->id_size);
        check_ret(ret);
    }

    ret = cbor_encoder_close_container(map, &entity);
    check_ret(ret);

    return 0;
}

// adds 2 to map, or 3 if add_user is true
static uint8_t ctap_end_get_assertion(CborEncoder * map, CTAP_credInfo *cred_info, uint8_t * auth_data_buf, unsigned int auth_data_buf_sz, uint8_t * sigbuf)
{
    int ret;
    uint8_t sigder[72];
    int sigder_sz;

    _Static_assert(sizeof(sigder) >= 72, "sigder buffer must be 72 bytes or more");
    ret = ctap_add_credential_descriptor(map, cred_info);  // 1
    check_retr(ret);

    {
        ret = cbor_encode_int(map,RESP_authData);  // 2
        check_ret(ret);
        ret = cbor_encode_byte_string(map, auth_data_buf, auth_data_buf_sz);
        check_ret(ret);
    }


    ret = cbor_encode_int(map, RESP_signature);  // 3
    check_ret(ret);
    if (cred_info->COSEAlgorithmIdentifier == COSE_ALG_ES256)
    {
        //sigbuf was calculated by main_mcu. Convert to DER format
        sigder_sz = ctap_encode_der_sig(sigbuf, sigder);
        ret = cbor_encode_byte_string(map, sigder, sigder_sz);
        check_ret(ret);
    }
    else
    {
        ret = cbor_encode_byte_string(map, sigbuf, FIDO2_ATTEST_SIG_LEN);
        check_ret(ret);
    }

    printf1(TAG_GREEN, "adding user details to output\r");
    ret = ctap_add_user_entity(map, &cred_info->user);  // 4
    check_retr(ret);

    return 0;
}

uint8_t ctap_get_assertion(CborEncoder * encoder, uint8_t * request, int length)
{
    CTAP_getAssertion GA;
    uint8_t sigbuf[64];

    _Static_assert(sizeof(sigbuf) >= 64, "sigbuf must be 64 bytes or greater");

    uint8_t auth_data_buf[sizeof(CTAP_authDataHeader) + 80];
    int ret = ctap_parse_get_assertion(&GA,request,length);

    if (ret != 0)
    {
        printf2(TAG_ERR,"error, parse_get_assertion failed");
        return ret;
    }

    if (GA.common.pinAuthEmpty)
    {
        return CTAP2_ERR_PIN_NOT_SET;
    }
    if (GA.common.pinAuthPresent)
    {
        return CTAP1_ERR_INVALID_PARAMETER;
    }

    if (!GA.common.rp.size || !GA.clientDataHashPresent)
    {
        return CTAP2_ERR_MISSING_PARAMETER;
    }
    CborEncoder map;

    int map_size = 3;

    printf1(TAG_GA, "ALLOW_LIST has %d creds", GA.credLen);

    map_size += 1;

    ret = cbor_encoder_create_map(encoder, &map, map_size);
    check_ret(ret);

    CTAP_credInfo cred_info;
    uint32_t auth_data_buf_sz = sizeof(auth_data_buf);
    {
        ret = ctap_make_get_assertion_auth_data(&GA.common, &GA, auth_data_buf, &auth_data_buf_sz, &cred_info, sigbuf);
        if (ret != CTAP1_ERR_SUCCESS)
        {
            printf1(TAG_ERR, "Error returned from get assertion credential: %d", ret);
            return ret;
        }
    }

    ret = ctap_end_get_assertion(&map, &cred_info, auth_data_buf, auth_data_buf_sz, sigbuf);  // 1,2,3,4
    check_retr(ret);

    ret = cbor_encoder_close_container(encoder, &map);
    check_ret(ret);

    return 0;
}

void ctap_response_init(CTAP_RESPONSE * resp)
{
    memset(resp, 0, sizeof(CTAP_RESPONSE));
    resp->data_size = CTAP_RESPONSE_BUFFER_SIZE;
}

/**
 * Delta from Solo implementation:
 * Disallowed CTAP_RESET
 * Removed GetNextAssertion
 */
uint8_t ctap_request(uint8_t * pkt_raw, int length, CTAP_RESPONSE * resp)
{
    CborEncoder encoder;
    memset(&encoder,0,sizeof(CborEncoder));
    uint8_t status = 0;
    uint8_t cmd = *pkt_raw;
    pkt_raw++;
    length--;

    uint8_t * buf = resp->data;

    cbor_encoder_init(&encoder, buf, resp->data_size, 0);

    printf1(TAG_CTAP,"cbor input structure: %d bytes", length);
    printf1(TAG_DUMP,"cbor req: "); dump_hex1(TAG_DUMP, pkt_raw, length);
    printf1(TAG_CTAP,"cbor cmd: 0x%x", cmd);

    switch(cmd)
    {
        case CTAP_MAKE_CREDENTIAL:
            printf1(TAG_CTAP,"CTAP_MAKE_CREDENTIAL");
            timestamp();
            status = ctap_make_credential(&encoder, pkt_raw, length);
            printf1(TAG_TIME,"make_credential time: %d ms", timestamp());

            resp->length = cbor_encoder_get_buffer_size(&encoder, buf);
            dump_hex1(TAG_DUMP, buf, resp->length);

            break;
        case CTAP_GET_ASSERTION:
            printf1(TAG_CTAP,"CTAP_GET_ASSERTION");
            timestamp();
            status = ctap_get_assertion(&encoder, pkt_raw, length);
            printf1(TAG_TIME,"get_assertion time: %d ms", timestamp());

            resp->length = cbor_encoder_get_buffer_size(&encoder, buf);

            printf1(TAG_DUMP,"cbor [%d]: ",  resp->length);
                dump_hex1(TAG_DUMP,buf, resp->length);
            break;
        case CTAP_CANCEL:
            printf1(TAG_CTAP,"CTAP_CANCEL");
            break;
        case CTAP_GET_INFO:
            printf1(TAG_CTAP,"CTAP_GET_INFO");
            status = ctap_get_info(&encoder);

            resp->length = cbor_encoder_get_buffer_size(&encoder, buf);

            dump_hex1(TAG_DUMP, buf, resp->length);

            break;
        case CTAP_RESET:
            //Not supported
            status = CTAP2_ERR_OPERATION_DENIED;
            break;
        case GET_NEXT_ASSERTION:
            status = CTAP2_ERR_NOT_ALLOWED;
            break;
        default:
            status = CTAP1_ERR_INVALID_COMMAND;
            printf2(TAG_ERR,"error, invalid cmd");
    }

    if (status != CTAP1_ERR_SUCCESS)
    {
        resp->length = 0;
    }

    printf1(TAG_CTAP,"cbor output structure: %d bytes.  Return 0x%02x", resp->length, status);

    return status;
}


/**
 * Removed Solo specific initialization
 */
void ctap_init()
{
}

/**
 * Removed Solo specific initialization
 */
void ctap_reset()
{
}
