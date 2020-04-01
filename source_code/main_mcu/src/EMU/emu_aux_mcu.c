#include "emu_aux_mcu.h"
#include "comms_aux_mcu.h"
#include "emulator.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

static BOOL response_valid;
static aux_mcu_message_t response;

static void send_hid_message(aux_mcu_message_t *msg);
static BOOL process_main_cmd(aux_mcu_message_t *msg, aux_mcu_message_t *response);
static BOOL process_ble_cmd(aux_mcu_message_t *msg, aux_mcu_message_t *response);

/* copied from aux mcu/src/LOGIC/logic_battery.h */
typedef enum    {LB_IDLE = 0, LB_CHARGE_START_RAMPING = 1, LB_CHARGING_REACH = 2, LB_ERROR_ST_RAMPING = 3, LB_CUR_MAINTAIN = 4, LB_ERROR_CUR_REACH = 5, LB_ERROR_CUR_MAINTAIN = 6, LB_CHARGING_DONE = 7} lb_state_machine_te;
static lb_state_machine_te emu_charger_status;

void emu_send_aux(char *data, int size)
{
    aux_mcu_message_t *msg = (aux_mcu_message_t*)data;
    assert(size == sizeof(aux_mcu_message_t));
    assert(response_valid == FALSE);

    switch(msg->message_type) {
        case AUX_MCU_MSG_TYPE_USB:
            send_hid_message(msg);
            break;

        case AUX_MCU_MSG_TYPE_PLAT_DETAILS:
            memset(&response, 0, sizeof(response));
            response.message_type = msg->message_type;
            response.payload_length1 = sizeof(aux_plat_details_message_t);
            response.aux_details_message.aux_fw_ver_major = 99;
            response.aux_details_message.aux_fw_ver_minor = 99;
            response.aux_details_message.aux_did_register = 123;
            response.aux_details_message.aux_uid_registers[0] = 0x01234567;
            response.aux_details_message.aux_uid_registers[1] = 0x89abcdef;
            response.aux_details_message.aux_uid_registers[2] = 0x08192a3b;
            response.aux_details_message.aux_uid_registers[3] = 0x4c5d6e7f;
            
            response.aux_details_message.blusdk_lib_maj = 88;
            response.aux_details_message.blusdk_lib_min = 88;
            response.aux_details_message.blusdk_fw_maj = 77;
            response.aux_details_message.blusdk_fw_min = 77;
            response.aux_details_message.blusdk_fw_build = 77;
            response.aux_details_message.atbtlc_rf_ver = 66;
            response.aux_details_message.atbtlc_chip_id = 55;
            memcpy(response.aux_details_message.atbtlc_address, "\x11\x22\x33\x44\x55\x66", 6);
            response_valid = TRUE;
            break;

        case AUX_MCU_MSG_TYPE_MAIN_MCU_CMD:
            memset(&response, 0, sizeof(response));
            response_valid = process_main_cmd(msg, &response);
            break;

        case AUX_MCU_MSG_TYPE_BLE_CMD:
            memset(&response, 0, sizeof(response));
            response_valid = process_ble_cmd(msg, &response);
            break;

        case AUX_MCU_MSG_TYPE_NIMH_CHARGE:
            memset(&response, 0, sizeof(response));
            response.message_type = msg->message_type;
            response.payload_length1 = sizeof(nimh_charge_message_t);
            response.nimh_charge_message.charge_status = emu_charger_status;
            response.nimh_charge_message.battery_voltage = 1;
            response.nimh_charge_message.charge_current = 2;
            response_valid = TRUE;
            break;

        case AUX_MCU_MSG_TYPE_PING_WITH_INFO:
            memset(&response, 0, sizeof(response));
            response.message_type = AUX_MCU_MSG_TYPE_AUX_MCU_EVENT;
            response.payload_length1 = sizeof(response.aux_mcu_event_message.event_id);
            response.aux_mcu_event_message.event_id = AUX_MCU_EVENT_IM_HERE;
            response_valid = TRUE;
            break;
    }
}

static BOOL process_ble_cmd(aux_mcu_message_t *msg, aux_mcu_message_t *resp)
{
    switch(msg->main_mcu_command_message.command) {

        case BLE_MESSAGE_CMD_ENABLE:
            resp->message_type = AUX_MCU_MSG_TYPE_AUX_MCU_EVENT;
            resp->aux_mcu_event_message.event_id = AUX_MCU_EVENT_BLE_ENABLED;
            resp->payload_length1 = sizeof(resp->aux_mcu_event_message.event_id);
            return TRUE;

        case BLE_MESSAGE_CMD_DISABLE:
            resp->message_type = AUX_MCU_MSG_TYPE_AUX_MCU_EVENT;
            resp->aux_mcu_event_message.event_id = AUX_MCU_EVENT_BLE_DISABLED;
            resp->payload_length1 = sizeof(resp->aux_mcu_event_message.event_id);
            return TRUE;
    }

    return FALSE;
}

static BOOL process_main_cmd(aux_mcu_message_t *msg, aux_mcu_message_t *resp)
{
    switch(msg->main_mcu_command_message.command) {
        case MAIN_MCU_COMMAND_SLEEP:
            break;
            
        case MAIN_MCU_COMMAND_UPDT_DEV_STAT:
            int cache_payload_size = 4;
            resp->hid_message.message_type = HID_CMD_GET_DEVICE_STATUS;
            resp->hid_message.payload_length = cache_payload_size;
            memcpy(resp->hid_message.payload, msg->main_mcu_command_message.payload, cache_payload_size);
            resp->payload_length1 = sizeof(resp->hid_message.message_type) + sizeof(resp->hid_message.payload_length) + resp->hid_message.payload_length;
            send_hid_message(resp);
            return FALSE;

        case MAIN_MCU_COMMAND_NIMH_CHARGE:
            emu_charger_status = LB_CHARGE_START_RAMPING;
            emu_charger_enable(TRUE);
            return FALSE;

        case MAIN_MCU_COMMAND_DETACH_USB:
            emu_charger_status = LB_IDLE;
            emu_charger_enable(FALSE);
            resp->message_type = AUX_MCU_MSG_TYPE_AUX_MCU_EVENT;
            resp->aux_mcu_event_message.event_id = AUX_MCU_EVENT_USB_DETACHED;
            resp->payload_length1 = sizeof(resp->aux_mcu_event_message.event_id);
            return TRUE;
    }

    return FALSE;
}
 
static int emu_rcv_aux_hid(aux_mcu_message_t *msg);

int emu_rcv_aux(char *data, int size)
{
    if(size == 0)
        return 0;

    assert(size == sizeof(aux_mcu_message_t));

    if(response_valid) {
        memcpy(data, &response, sizeof(response));
        response_valid = FALSE;
        return sizeof(response);
    }

    /* generate "charging done" message */
    if(emu_charger_status == LB_CHARGE_START_RAMPING) {
        if(emu_get_battery_level() == 100) {
            emu_charger_enable(FALSE);
            emu_charger_status = LB_CHARGING_DONE;
            memset(&response, 0, sizeof(response));
            response.message_type = AUX_MCU_MSG_TYPE_AUX_MCU_EVENT;
            response.payload_length1 = sizeof(response.aux_mcu_event_message.event_id);
            response.aux_mcu_event_message.event_id = AUX_MCU_EVENT_CHARGE_DONE;
            memcpy(data, &response, sizeof(response));
            return sizeof(response);
        }
    }

    return emu_rcv_aux_hid((aux_mcu_message_t*)data);
}

/*! \fn     send_hid_message(aux_mcu_message_t *msg)
*   \brief  Send simulated "hid" messages to moolticute
*   \param  msg   The message to be sent
*   \note   The message will possibly be split into multiple hid packets
*/

static void send_hid_message(aux_mcu_message_t *msg)
{
    uint8_t *payload = msg->payload;
    int payload_length = msg->payload_length1;

    int n_hid_packets = (payload_length+61) / 62;
    int p;

    for(p=0;p < n_hid_packets;p++) {
        char hidPacket[64];
        int bytesRemain = payload_length - p * 62;
        if(bytesRemain > 62)
            bytesRemain = 62;

        hidPacket[0] = bytesRemain;
        hidPacket[1] = (p<<4) | (n_hid_packets-1);
        memcpy(hidPacket+2, payload + p * 62, bytesRemain);

        emu_send_hid(hidPacket, bytesRemain+2);
    }
}


/* Low-level hid packets from moolticute are received into this buffer */
static uint8_t incomingHidPacket[64];

/* State related to reassembling hid packets into MP messages */
static int incomingHidFill;
static BOOL expectFlipBit;
static uint8_t expectByte1;

/* Mooltipass protocol messages are assembled inside this structure */
#define INVALID_LENGTH (-1)
static aux_mcu_message_t hid_response;
static int hid_response_fill;

static void reset_hid_processing(void) 
{
    incomingHidFill = 0;
    hid_response_fill = INVALID_LENGTH;
    expectFlipBit = FALSE;
    expectByte1 = 0;
}

/*! \fn     process_hid_packet(void)
*   \brief  Reassemble HID packets coming from moolticute into Mooltipass protocol messages
*   \param  packet  pointer to packet bytes
*   \param  payloadLength  number of *payload* bytes, the packet is two bytes longer than this
*   \note   This copies the packet data into hid_response
*   \return TRUE if a message is ready
*/
static BOOL process_hid_packet(uint8_t *packet, int payloadLength)
{
    uint8_t byte0 = packet[0], byte1 = packet[1];

    if(byte0 == 0xff && byte1 == 0xff) {
        /* To reset the flip bit state machine, the computer may simply send a packet with the first two bytes set to 0xFF.
         * The device will then expect the next packet to have the flip bit set to 0. */
        expectFlipBit = FALSE;
        hid_response_fill = INVALID_LENGTH;
        return FALSE;
    }

    if(((byte0 >> 7) & 1) == expectFlipBit) {
        /* Prepare to receive a new packet. If there was anything in the buffer, flush it */
        hid_response_fill = INVALID_LENGTH;
        expectFlipBit = !expectFlipBit;

    } else if(hid_response_fill == INVALID_LENGTH) {
        /* The flip bit hasn't changed, AND incoming buffer is empty. This means that a new
         * message was sent without changing the flip bit. Not ok */
        fprintf(stderr, "Incoming HID packet with invalid flip bit\n");
        return FALSE;

    } else if(expectByte1 != byte1) {
        /* Byte1 can only have one valid value for packets which don't start a message */
        fprintf(stderr, "Incoming HID packet with invalid byte1: %x != %x\n", byte1, expectByte1);
        hid_response_fill = INVALID_LENGTH;
        return FALSE;
    }

    if(hid_response_fill == INVALID_LENGTH) {
        /* Initial packet */
        memset(&hid_response, 0, sizeof(hid_response));

        /* These are already zero, but let's make it clear what is happening */
        hid_response.message_type = AUX_MCU_MSG_TYPE_USB;
        hid_response_fill = 0;

        expectByte1 = byte1;
    }

    if(payloadLength + hid_response_fill > AUX_MCU_MSG_PAYLOAD_LENGTH) {
        fprintf(stderr, "Incoming HID packets overflow buffer\n");
        hid_response_fill = INVALID_LENGTH;

    } else {
        memcpy(hid_response.payload + hid_response_fill, packet+2, payloadLength);
        hid_response_fill += payloadLength;

        if(((byte1 & 0xf0) >> 4) == (byte1 & 0x0f)) {
            /* This was the final packet, the message is ready */
            hid_response.payload_length1 = hid_response_fill;
            hid_response_fill = INVALID_LENGTH;
            return TRUE;

        } else {
            expectByte1 += 0x10;
        }
    }

    return FALSE;
}

/*! \fn     rcv_hid_messages(void)
*   \brief  Receive simulated "hid" messages from moolticute & reassemble messages
*   \note   The packets are concatenated into a stream, but we can split them up based on the payload length byte.
*/
static int emu_rcv_aux_hid(aux_mcu_message_t *msg)
{
    int nr = emu_rcv_hid((char*)incomingHidPacket + incomingHidFill, sizeof(incomingHidPacket) - incomingHidFill);
    BOOL hid_response_valid = FALSE;

    if(nr < 0) {
        /* Moolticute not connected, reset buffers */
        reset_hid_processing();
        return 0;
    }

    incomingHidFill += nr;

    if(incomingHidFill >= 2) {
        int hidPayloadLength = incomingHidPacket[0] & 63;
        if(incomingHidPacket[0] == 0xff && incomingHidPacket[1] == 0xff)
            hidPayloadLength = 0; /* special case */

        if(hidPayloadLength > 62) {
            fprintf(stderr, "Invalid HID packet received, byte0 = %x\n", incomingHidPacket[0]);
            reset_hid_processing();
            return 0;

        } else if(incomingHidFill >= hidPayloadLength+2) {
            hid_response_valid = process_hid_packet(incomingHidPacket, hidPayloadLength);
        
            if(hid_response_valid && (incomingHidPacket[0] & 0x40)) {
                /* send acknowledgement */
                emu_send_hid((char*)incomingHidPacket, 2 + hidPayloadLength);
            }
            
            /* Shift in next packet, if any */
            incomingHidFill -= 2 + hidPayloadLength;
            memmove(incomingHidPacket, incomingHidPacket + 2 + hidPayloadLength, incomingHidFill);
        }
    }

    if(hid_response_valid) {
        memcpy(msg, &hid_response, sizeof(hid_response));
        hid_response_valid = FALSE;
        return sizeof(hid_response);
    }

    return 0;

}
