/*!  \file     comms_aux_mcu.h
*    \brief    Communications with aux MCU
*    Created:  03/03/2018
*    Author:   Mathieu Stephan
*/


#ifndef COMMS_AUX_MCU_H_
#define COMMS_AUX_MCU_H_

#include "comms_aux_mcu_defines.h"
#include "comms_bootloader_msg.h"
#include "comms_hid_msgs.h"
#include "defines.h"
#include "ctap.h"

/* Share vars */
extern volatile BOOL comms_main_mcu_usb_msg_answered_using_first_bytes;
extern volatile BOOL comms_main_mcu_ble_msg_answered_using_first_bytes;
extern volatile BOOL comms_main_mcu_other_msg_answered_using_first_bytes;

/* Timeout when waiting for something from main MCU */
#define MAIN_MCU_COMMS_WAIT_TIMEOUT     5000

/* Defines */
#define AUX_MCU_MSG_TYPE_USB            0x0000
#define AUX_MCU_MSG_TYPE_BLE            0x0001
#define AUX_MCU_MSG_TYPE_BOOTLOADER     0x0002
#define AUX_MCU_MSG_TYPE_PLAT_DETAILS   0x0003
#define AUX_MCU_MSG_TYPE_MAIN_MCU_CMD   0x0004
#define AUX_MCU_MSG_TYPE_AUX_MCU_EVENT  0x0005
#define AUX_MCU_MSG_TYPE_NIMH_CHARGE    0x0006
#define AUX_MCU_MSG_TYPE_PING_WITH_INFO 0x0007
#define AUX_MCU_MSG_TYPE_KEYBOARD_TYPE  0x0008
#define AUX_MCU_MSG_TYPE_FIDO2          0x0009
#define AUX_MCU_MSG_TYPE_RNG_TRANSFER   0x000A
#define AUX_MCU_MSG_TYPE_BLE_CMD        0x000B

// Main MCU commands
#define MAIN_MCU_COMMAND_SLEEP              0x0001
#define MAIN_MCU_COMMAND_ATTACH_USB         0x0002
#define MAIN_MCU_COMMAND_NIMH_CHG_SLW_STRT  0x0003
#define MAIN_MCU_COMMAND_NIMH_RECOVERY_CHG  0x0004
#define MAIN_MCU_COMMAND_NIMH_CHARGE        0x0005
#define MAIN_MCU_COMMAND_NO_COMMS_UNAV      0x0006
#define MAIN_MCU_COMMAND_TYPE_SHORTCUT      0x0007
#define MAIN_MCU_COMMAND_DETACH_USB         0x0008
#define MAIN_MCU_COMMAND_FUNC_TEST          0x0009
#define MAIN_MCU_COMMAND_UPDT_DEV_STAT      0x000A
#define MAIN_MCU_COMMAND_STOP_CHARGE        0x000B
#define MAIN_MCU_COMMAND_SET_BATTERYLVL     0x000C
#define MAIN_MCU_COMMAND_GET_STATUS         0x000D
#define MAIN_MCU_COMMAND_NIMH_DANGER_CHARGE 0x000E
#define MAIN_MCU_COMMAND_DISABLE_BLE        0x000F

// Debug MCU commands
#define MAIN_MCU_COMMAND_DTM_RX_START       0x1000
#define MAIN_MCU_COMMAND_TX_TONE_CONT       0x1001
#define MAIN_MCU_COMMAND_DTM_STOP           0x1002
#define MAIN_MCU_COMMAND_FORCE_CHARGE_VOLT  0x1003
#define MAIN_MCU_COMMAND_STOP_FORCE_CHARGE  0x1004

// Aux MCU events
#define AUX_MCU_EVENT_BLE_ENABLED           0x0001
#define AUX_MCU_EVENT_BLE_DISABLED          0x0002
#define AUX_MCU_EVENT_TX_SWEEP_DONE         0x0003
#define AUX_MCU_EVENT_FUNC_TEST_DONE        0x0004
#define AUX_MCU_EVENT_USB_ENUMERATED        0x0005
#define AUX_MCU_EVENT_CHARGE_DONE           0x0006
#define AUX_MCU_EVENT_CHARGE_FAIL           0x0007
#define AUX_MCU_EVENT_SLEEP_RECEIVED        0x0008
#define AUX_MCU_EVENT_IM_HERE               0x0009
#define AUX_MCU_EVENT_BLE_CONNECTED         0x000A
#define AUX_MCU_EVENT_BLE_DISCONNECTED      0x000B
#define AUX_MCU_EVENT_USB_DETACHED          0x000C
#define AUX_MCU_EVENT_CHARGE_LVL_UPDATE     0x000D
#define AUX_MCU_EVENT_USB_TIMEOUT           0x000E
#define AUX_MCU_EVEN_HERES_MY_STATUS        0x000F
#define AUX_MCU_EVENT_ATTACH_CMD_RCVD       0x0010
#define AUX_MCU_EVENT_CHARGE_STARTED        0x0011
#define AUX_MCU_EVENT_NO_COMMS_INFO_RCVD    0x0012
#define AUX_MCU_EVENT_SHORTCUT_TYPED        0x0013
#define AUX_MCU_EVENT_NEW_STATUS_RCVD       0x0014
#define AUX_MCU_EVENT_CHARGE_STOPPED        0x0015
#define AUX_MCU_EVENT_NEW_BATTERY_LVL_RCVD  0x0016
#define AUX_MCU_EVENT_RX_DTM_DONE           0x0017
#define AUX_MCU_EVENT_BLE_CON_SPAM          0x0018

// BLE commands
#define BLE_MESSAGE_CMD_ENABLE              0x0001
#define BLE_MESSAGE_CMD_RESERVED            0x0002
#define BLE_MESSAGE_STORE_BOND_INFO         0x0003
#define BLE_MESSAGE_RECALL_BOND_INFO        0x0004
#define BLE_MESSAGE_CLEAR_BOND_INFO         0x0005
#define BLE_MESSAGE_ENABLE_PAIRING          0x0006
#define BLE_MESSAGE_DISABLE_PAIRING         0x0007
#define BLE_MESSAGE_GET_IRK_KEYS            0x0008
#define BLE_MESSAGE_RECALL_BOND_INFO_IRK    0x0009
#define BLE_MESSAGE_GET_BT_6_DIGIT_CODE     0x000A
#define BLE_MESSAGE_DISCONNECT_FOR_NEXT     0x000B

/* FIDO2 messages start */
#define AUX_MCU_MSG_TYPE_FIDO2_START 0x0001
#define AUX_MCU_FIDO2_AUTH_CRED_REQ  AUX_MCU_MSG_TYPE_FIDO2_START
#define AUX_MCU_FIDO2_AUTH_CRED_RSP  0x0002
//MC = MAKE CREDENTIAL
#define AUX_MCU_FIDO2_MC_REQ         0x0003
#define AUX_MCU_FIDO2_MC_RSP         0x0004
//GA = GET ASSERTION
#define AUX_MCU_FIDO2_GA_REQ         0x0005
#define AUX_MCU_FIDO2_GA_RSP         0x0006
#define AUX_MCU_FIDO2_RETRY          0x0007
#define AUX_MCU_MSG_TYPE_FIDO2_END   AUX_MCU_FIDO2_RETRY
/* FIDO2 messages end */

/*
 * Flags for Get Assertion request to Main MCU
 */
#define FIDO2_GA_FLAG_SILENT        0x1 // Indicates to not prompt for user ACK

/* Typedefs */
typedef struct
{
    uint16_t aux_fw_ver_major;
    uint16_t aux_fw_ver_minor;
    uint32_t aux_did_register;
    uint32_t aux_uid_registers[4];
    uint16_t blusdk_lib_maj;
    uint16_t blusdk_lib_min;
    uint16_t blusdk_fw_maj;
    uint16_t blusdk_fw_min;
    uint16_t blusdk_fw_build;
    uint32_t atbtlc_rf_ver;
    uint32_t atbtlc_chip_id;
    uint8_t atbtlc_address[6];
    uint32_t aux_stack_low_watermark;
} aux_plat_details_message_t;

typedef struct
{
    uint16_t command;
    union
    {
        uint8_t payload[AUX_MCU_MSG_PAYLOAD_LENGTH-sizeof(uint16_t)];
        uint16_t payload_as_uint16[(AUX_MCU_MSG_PAYLOAD_LENGTH-sizeof(uint16_t))/2];
    };
} main_mcu_command_message_t;

typedef struct
{
    uint16_t event_id;
    union
    {
        uint8_t payload[AUX_MCU_MSG_PAYLOAD_LENGTH-sizeof(uint16_t)];
        uint16_t payload_as_uint16[(AUX_MCU_MSG_PAYLOAD_LENGTH-sizeof(uint16_t))/2];
    };
} aux_mcu_event_message_t;

// Bluetooth bonding information
typedef struct
{
    uint16_t zero_to_be_valid;
    uint8_t address_resolv_type;
    uint8_t mac_address[6];
    uint8_t auth_type;
    uint8_t peer_ltk_key[16];
    uint16_t peer_ltk_ediv;
    uint8_t peer_ltk_random_nb[8];
    uint16_t peer_ltk_key_size;
    uint8_t peer_csrk_key[16];
    uint8_t peer_irk_key[16];
    uint8_t peer_irk_resolv_type;
    uint8_t peer_irk_address[6];
    uint8_t peer_irk_reserved;
    uint8_t host_ltk_key[16];
    uint16_t host_ltk_ediv;
    uint8_t host_ltk_random_nb[8];
    uint16_t host_ltk_key_size;
    uint8_t host_csrk_key[16];
    uint8_t reserved[10];
} nodemgmt_bluetooth_bonding_information_t;

typedef struct
{
    uint16_t message_id;
    union
    {
        nodemgmt_bluetooth_bonding_information_t bonding_information_to_store_message;
        uint8_t payload[AUX_MCU_MSG_PAYLOAD_LENGTH-sizeof(uint16_t)];
        uint16_t payload_as_uint16_t[(AUX_MCU_MSG_PAYLOAD_LENGTH-sizeof(uint16_t))/2];
    };
} ble_message_t;

typedef struct
{
    uint16_t charge_status;
    uint16_t battery_voltage;
    int16_t charge_current;
    uint16_t stepdown_voltage;
    uint16_t dac_data_reg;
} nimh_charge_message_t;

typedef struct
{
    uint16_t place_holder;
} ping_with_info_message_t;

typedef struct
{
    uint16_t interface_identifier;
    uint16_t delay_between_types;
    uint16_t keyboard_symbols[(AUX_MCU_MSG_PAYLOAD_LENGTH/2)-sizeof(uint16_t)-sizeof(uint16_t)];
} keyboard_type_message_t;

typedef struct fido2_credential_ID_s
{
    uint8_t tag[FIDO2_CREDENTIAL_ID_LENGTH];
} fido2_credential_ID_t;

typedef struct fido2_auth_cred_req_message_s
{
    uint8_t rpID[FIDO2_RPID_LEN];
    fido2_credential_ID_t cred_ID;
} fido2_auth_cred_req_message_t;

typedef struct fido2_auth_cred_rsp_message_s
{
    fido2_credential_ID_t cred_ID;
    uint8_t user_handle[FIDO2_USER_HANDLE_LEN];
    uint8_t user_handle_len;
    uint8_t result;
} fido2_auth_cred_rsp_message_t;

typedef struct fido2_allow_list_s
{
    uint8_t len;
    uint8_t tag[FIDO2_ALLOW_LIST_MAX_SIZE][FIDO2_CREDENTIAL_ID_LENGTH]; //160 bytes
} fido2_allow_list_t;

typedef struct fido2_make_credential_req_message_s
{
    uint8_t rpID[FIDO2_RPID_LEN];
    uint8_t user_handle[FIDO2_USER_HANDLE_LEN];
    uint8_t user_handle_len;
    uint8_t user_name[FIDO2_USER_NAME_LEN];
    uint8_t display_name[FIDO2_DISPLAY_NAME_LEN];
    uint8_t client_data_hash[FIDO2_CLIENT_DATA_HASH_LEN];
} fido2_make_credential_req_message_t;

typedef struct fido2_make_credential_rsp_message_s
{
    uint8_t tag[FIDO2_CREDENTIAL_ID_LENGTH];
    uint8_t rpID_hash[FIDO2_RPID_HASH_LEN];
    uint32_t count_BE;
    uint32_t flags;
    uint8_t pub_key_x[FIDO2_PUB_KEY_X_LEN];
    uint8_t pub_key_y[FIDO2_PUB_KEY_Y_LEN];
    uint8_t attest_sig[FIDO2_ATTEST_SIG_LEN];
    uint8_t aaguid[FIDO2_AAGUID_LEN];
    uint32_t cred_ID_len;
    uint8_t error_code;
} fido2_make_credential_rsp_message_t;

typedef struct fido2_get_assertion_req_message_s
{
    uint8_t rpID[FIDO2_RPID_LEN];
    uint8_t client_data_hash[FIDO2_CLIENT_DATA_HASH_LEN];
    fido2_allow_list_t allow_list;
    uint8_t flags;
} fido2_get_assertion_req_message_t;

typedef struct fido2_get_assertion_rsp_message_s
{
    uint8_t tag[FIDO2_CREDENTIAL_ID_LENGTH];
    uint8_t user_handle[FIDO2_USER_HANDLE_LEN];
    uint8_t user_handle_len;
    uint8_t rpID_hash[FIDO2_RPID_HASH_LEN];
    uint32_t count_BE;
    uint32_t flags;
    uint8_t attest_sig[FIDO2_ATTEST_SIG_LEN];
    uint8_t aaguid[FIDO2_AAGUID_LEN];
    uint8_t error_code;
} fido2_get_assertion_rsp_message_t;

typedef struct fido2_message_s
{
    uint16_t message_type;
    uint16_t reserved;
    union
    {
        fido2_auth_cred_req_message_t fido2_auth_cred_req_message;
        fido2_auth_cred_rsp_message_t fido2_auth_cred_rsp_message;
        fido2_make_credential_req_message_t fido2_make_credential_req_message;
        fido2_make_credential_rsp_message_t fido2_make_credential_rsp_message;
        fido2_get_assertion_req_message_t fido2_get_assertion_req_message;
        fido2_get_assertion_rsp_message_t fido2_get_assertion_rsp_message;
    };
} fido2_message_t;

typedef struct
{
    uint16_t message_type;
    uint16_t payload_length1;
    union
    {
        aux_mcu_bootloader_message_t bootloader_message;
        aux_plat_details_message_t aux_details_message;
        main_mcu_command_message_t main_mcu_command_message;
        ping_with_info_message_t ping_with_info_message;
        aux_mcu_event_message_t aux_mcu_event_message;
        keyboard_type_message_t keyboard_type_message;
        nimh_charge_message_t nimh_charge_message;
        fido2_message_t fido2_message;
        hid_message_t hid_message;
        ble_message_t ble_message;
        uint8_t payload[AUX_MCU_MSG_PAYLOAD_LENGTH];
        uint16_t payload_as_uint16[AUX_MCU_MSG_PAYLOAD_LENGTH/2];
        uint32_t payload_as_uint32[AUX_MCU_MSG_PAYLOAD_LENGTH/4];    
    };
    uint16_t payload_length2;
    union
    {
        uint16_t rx_payload_valid_flag;
        uint16_t tx_not_used;
    };
} aux_mcu_message_t;

/* Prototypes */
ret_type_te comms_main_mcu_routine(BOOL filter_and_force_use_of_temp_receive_buffer, uint16_t expected_message_type, BOOL resend_send_msg_if_retry_of_type_received);
ret_type_te comms_main_mcu_fetch_bonding_info_for_mac(uint8_t address_resolv_type, uint8_t* mac_addr, nodemgmt_bluetooth_bonding_information_t* bonding_info);
ret_type_te comms_main_mcu_fetch_bonding_info_for_irk(uint8_t* irk_key, nodemgmt_bluetooth_bonding_information_t* bonding_info);
void comms_main_mcu_get_empty_packet_ready_to_be_sent(aux_mcu_message_t** message_pt_pt, uint16_t message_type);
void comms_main_mcu_send_simple_event_alt_buffer(uint16_t event_id, aux_mcu_message_t* buffer);
void comms_main_mcu_send_message(aux_mcu_message_t* message, uint16_t message_length);
BOOL comms_aux_mcu_get_received_packet(aux_mcu_message_t** message, BOOL arm_new_rx);
void comms_main_mcu_deal_with_non_usb_non_ble_message(aux_mcu_message_t* message);
uint16_t comms_main_mcu_get_bonding_info_irks(uint8_t** irk_keys_buffer);
aux_mcu_message_t* comms_main_mcu_get_temp_tx_message_object_pt(void);
aux_mcu_message_t* comms_main_mcu_get_temp_rx_message_object_pt(void);
void comms_main_mcu_get_32_rng_bytes_from_main_mcu(uint8_t* buffer);
RET_TYPE comms_main_mcu_fetch_6_digits_pin(uint8_t* pin_array);
void comms_main_mcu_send_simple_event(uint16_t event_id);
void comms_main_init_rx(void);


#endif /* COMMS_AUX_MCU_H_ */
