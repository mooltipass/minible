/*!  \file     comms_hid_msgs.h
*    \brief    HID communications
*    Created:  06/03/2018
*    Author:   Mathieu Stephan
*/
#include <asf.h>
#include <string.h>
#include "logic_smartcard.h"
#include "gui_dispatcher.h"
#include "comms_hid_msgs.h"
#include "logic_security.h"
#include "comms_aux_mcu.h"
#include "logic_device.h"
#include "gui_prompts.h"
#include "logic_user.h"
#include "custom_fs.h"
#include "nodemgmt.h"
#include "dbflash.h"
#include "utils.h"
#include "dma.h"
#include "rng.h"


/*! \fn     comms_hid_msgs_parse(hid_message_t* rcv_msg, uint16_t supposed_payload_length, hid_message_t* send_msg, msg_restrict_type_te answer_restrict_type)
*   \brief  Parse an incoming message from USB or BLE
*   \param  rcv_msg                 Received message
*   \param  supposed_payload_length Supposed payload length
*   \param  send_msg                Where to write a possible reply
*   \param  answer_restrict_type    Enum restricting which messages we can answer
*   \return something >= 0 if an answer needs to be sent, otherwise -1
*/
int16_t comms_hid_msgs_parse(hid_message_t* rcv_msg, uint16_t supposed_payload_length, hid_message_t* send_msg, msg_restrict_type_te answer_restrict_type)
{    
    /* Check correct payload length */
    if ((supposed_payload_length != rcv_msg->payload_length) || (supposed_payload_length > sizeof(rcv_msg->payload)))
    {
        /* Silent error */
        return -1;
    }
    
    /* Checks based on restriction type */
    BOOL should_ignore_message = FALSE;
    if ((answer_restrict_type == MSG_RESTRICT_ALL) && (rcv_msg->message_type != HID_CMD_ID_PING))
    {
        should_ignore_message = TRUE;
    }
    if ((answer_restrict_type == MSG_RESTRICT_ALLBUT_CANCEL) && (rcv_msg->message_type != HID_CMD_ID_PING) && (rcv_msg->message_type != HID_CMD_ID_CANCEL_REQ))
    {
        should_ignore_message = TRUE;
    }
    // TODO: deal with all but bundle
    
    /* Depending on restriction, answer please retry */
    if (should_ignore_message != FALSE)
    {
        /* Send please retry */
        send_msg->message_type = HID_CMD_ID_RETRY;
        send_msg->payload_length = 0;
        return 0;
    }
    
    /* By default: copy the same CMD identifier for TX message */
    send_msg->message_type = rcv_msg->message_type;
    uint16_t rcv_message_type = rcv_msg->message_type;
    
    /* Check for commands for management mode */
    if ((rcv_msg->message_type >= HID_FIRST_CMD_FOR_MMM) && (rcv_msg->message_type <= HID_LAST_CMD_FOR_MMM) && (logic_security_is_management_mode_set() == FALSE))
    {
        /* Set nack, leave same command id */
        send_msg->payload[0] = HID_1BYTE_NACK;
        send_msg->payload_length = 1;
        return 1;
    }
    
    /* Switch on command id */
    switch (rcv_msg->message_type)
    {
        case HID_CMD_ID_PING:
        {
            /* Simple ping: copy the message contents */
            memcpy((void*)send_msg->payload, (void*)rcv_msg->payload, rcv_msg->payload_length);
            send_msg->payload_length = rcv_msg->payload_length;
            return send_msg->payload_length;
        }
        
        case HID_CMD_ID_PLAT_INFO:
        {
            aux_mcu_message_t* temp_rx_message;
            aux_mcu_message_t* temp_tx_message_pt;
            
            /* Generate our packet */
            comms_aux_mcu_get_empty_packet_ready_to_be_sent(&temp_tx_message_pt, AUX_MCU_MSG_TYPE_PLAT_DETAILS);
            
            /* Wait for current packet reception and arm reception */
            dma_aux_mcu_wait_for_current_packet_reception_and_clear_flag();
            comms_aux_arm_rx_and_clear_no_comms();
            
            /* Send message */
            comms_aux_mcu_send_message(TRUE);
            
            /* Wait for message from aux MCU */
            while(comms_aux_mcu_active_wait(&temp_rx_message, TRUE, AUX_MCU_MSG_TYPE_PLAT_DETAILS) == RETURN_NOK){}
            
            /* Copy message contents into send packet */
            send_msg->platform_info.main_mcu_fw_major = FW_MAJOR;
            send_msg->platform_info.main_mcu_fw_minor = FW_MINOR;
            send_msg->platform_info.aux_mcu_fw_major = temp_rx_message->aux_details_message.aux_fw_ver_major;
            send_msg->platform_info.aux_mcu_fw_minor = temp_rx_message->aux_details_message.aux_fw_ver_minor;
            send_msg->platform_info.plat_serial_number = 12345678;
            send_msg->platform_info.memory_size = DBFLASH_CHIP;            
            send_msg->payload_length = sizeof(send_msg->platform_info);
            send_msg->message_type = rcv_message_type;
            return sizeof(send_msg->platform_info);            
        }
        
        case HID_CMD_ID_SET_DATE:
        {
            /* Set Date */
            nodemgmt_set_current_date(rcv_msg->payload_as_uint16[0]);
            
            /* Set ack, leave same command id */
            send_msg->payload[0] = HID_1BYTE_ACK;
            send_msg->payload_length = 1;
            return 1;
        }
        
        case HID_CMD_ID_GET_32B_RNG:
        {            
            rng_fill_array(send_msg->payload, 32);
            send_msg->payload_length = 32;
            return 32;
        }
        
        case HID_CMD_GET_START_PARENTS:
        {
            /* Get all starting parents */
            send_msg->payload_as_uint16[0] = nodemgmt_get_starting_parent_addr();
            for (uint16_t i = 0; i < MEMBER_ARRAY_SIZE(nodemgmt_profile_main_data_t, data_start_address); i++)
            {
                send_msg->payload_as_uint16[1+i] = nodemgmt_get_starting_data_parent_addr(i);
            }
            
            /* Return correct size */
            send_msg->payload_length = sizeof(uint16_t) + MEMBER_SIZE(nodemgmt_profile_main_data_t, data_start_address);
            return send_msg->payload_length;
        }

        case HID_CMD_SET_CRED_ST_PARENT:
        {
            /* Check address length */
            if (rcv_msg->payload_length == sizeof(uint16_t))
            {
                /* Store new address */
                nodemgmt_set_cred_start_address(rcv_msg->payload_as_uint16[0]);

                /* Set success byte */
                send_msg->payload[0] = HID_1BYTE_ACK;
            }
            else
            {
                /* Set failure byte */
                send_msg->payload[0] = HID_1BYTE_NACK;
            }
            
            send_msg->payload_length = 1;
            return 1;
        }

        case HID_CMD_SET_DATA_ST_PARENT:
        {
            /* Check address length */
            if (rcv_msg->payload_length == 2*sizeof(uint16_t))
            {
                /* Store new address */
                nodemgmt_set_data_start_address(rcv_msg->payload_as_uint16[1], rcv_msg->payload_as_uint16[0]);

                /* Set success byte */
                send_msg->payload[0] = HID_1BYTE_ACK;
            }
            else
            {
                /* Set failure byte */
                send_msg->payload[0] = HID_1BYTE_NACK;
            }
            
            send_msg->payload_length = 1;
            return 1;
        }

        case HID_CMD_SET_START_PARENTS:
        {
            /* Check address length */
            if (rcv_msg->payload_length == (sizeof(uint16_t) + MEMBER_SIZE(nodemgmt_profile_main_data_t, data_start_address)))
            {
                /* Store new addresses */
                nodemgmt_set_start_addresses(rcv_msg->payload_as_uint16);

                /* Set success byte */
                send_msg->payload[0] = HID_1BYTE_ACK;
            }
            else
            {
                /* Set failure byte */
                send_msg->payload[0] = HID_1BYTE_NACK;
            }
            
            send_msg->payload_length = 1;
            return 1;
        }

        case HID_CMD_GET_FREE_NODES:
        {
            /* Check for correct number of args and that not too many free slots have been requested */
            if ((rcv_msg->payload_length == 3*sizeof(uint16_t)) && ((rcv_msg->payload_as_uint16[1] + rcv_msg->payload_as_uint16[2]) <= MEMBER_ARRAY_SIZE(hid_message_t,payload_as_uint16)))
            {
                uint16_t nb_nodes_found = nodemgmt_find_free_nodes(rcv_msg->payload_as_uint16[1], send_msg->payload_as_uint16, rcv_msg->payload_as_uint16[2], &(send_msg->payload_as_uint16[rcv_msg->payload_as_uint16[1]]), nodemgmt_page_from_address(rcv_msg->payload_as_uint16[0]), nodemgmt_node_from_address(rcv_msg->payload_as_uint16[0]));
                
                /* Send free nodes */
                send_msg->payload_length = nb_nodes_found*sizeof(uint16_t);
                return nb_nodes_found*sizeof(uint16_t);
            }
            else
            {
                /* Send failure */
                send_msg->payload[0] = HID_1BYTE_NACK;
                send_msg->payload_length = 1;
                return 1;
            }
        }
        
        case HID_CMD_END_MMM:
        {
            if (logic_security_is_management_mode_set() != FALSE)
            {
                /* Clear bool */
                logic_device_activity_detected();
                logic_security_clear_management_mode();
                
                /* Set next screen */
                gui_dispatcher_set_current_screen(GUI_SCREEN_MAIN_MENU, TRUE, GUI_INTO_MENU_TRANSITION);
                gui_dispatcher_get_back_to_current_screen();
                nodemgmt_scan_node_usage();
            }
            
            /* Set ack, leave same command id */
            send_msg->payload[0] = HID_1BYTE_ACK;
            send_msg->payload_length = 1;
            return 1;
        }
        
        case HID_CMD_START_MMM:
        {
            /* Smartcard unlocked? */
            if (logic_security_is_smc_inserted_unlocked() != FALSE)
            {
                // TODO: if it makes sense, ask user to enter his PIN
                
                /* Prompt the user */
                if (gui_prompts_ask_for_one_line_confirmation(ID_STRING_ENTER_MMM, TRUE) == MINI_INPUT_RET_YES)
                {
                    /* Approved, set next screen */
                    gui_dispatcher_set_current_screen(GUI_SCREEN_MEMORY_MGMT, TRUE, GUI_INTO_MENU_TRANSITION);
                    
                    /* Update current state */
                    logic_security_set_management_mode();
                    
                    /* Set success byte */
                    send_msg->payload[0] = HID_1BYTE_ACK;
                }
                else
                {
                    /* Set failure byte */
                    send_msg->payload[0] = HID_1BYTE_NACK;
                }
            } 
            else
            {
                /* Set failure byte */
                send_msg->payload[0] = HID_1BYTE_NACK;
            }
            
            /* Go back to old or updated screen */
            gui_dispatcher_get_back_to_current_screen();
            
            /* Return success or failure */
            send_msg->payload_length = 1;
            return 1;
        }
        
        case HID_CMD_READ_NODE:
        {
            /* Check address length */
            if (rcv_msg->payload_length == sizeof(uint16_t))
            {
                node_type_te temp_node_type;
                
                /* Check user permission */
                if (nodemgmt_check_user_permission(rcv_msg->payload_as_uint16[0], &temp_node_type) == RETURN_OK)
                {
                    if ((temp_node_type == NODE_TYPE_PARENT) || (temp_node_type == NODE_TYPE_PARENT_DATA) || (temp_node_type == NODE_TYPE_NULL))
                    {
                        /* Read and send parent node */
                        nodemgmt_read_parent_node_data_block_from_flash(rcv_msg->payload_as_uint16[0], (parent_node_t*)send_msg->payload_as_uint16);
                        send_msg->payload_length = sizeof(parent_node_t);
                        return sizeof(parent_node_t);
                    } 
                    else
                    {
                        /* Read and send child node */
                        nodemgmt_read_child_node_data_block_from_flash(rcv_msg->payload_as_uint16[0], (child_node_t*)send_msg->payload_as_uint16);
                        send_msg->payload_length = sizeof(child_node_t);
                        return sizeof(child_node_t);
                    }
                } 
                else
                {
                    /* Set nack, leave same command id */
                    send_msg->payload[0] = HID_1BYTE_NACK;
                    send_msg->payload_length = 1;
                    return 1;
                }
            } 
            else
            {
                /* Set nack, leave same command id */
                send_msg->payload[0] = HID_1BYTE_NACK;
                send_msg->payload_length = 1;
                return 1;
            }
        }

        // Read user db change number
        case HID_CMD_GET_USER_CHANGE_NB :
        {
            /* Smartcard unlocked? */
            if (logic_security_is_smc_inserted_unlocked() != FALSE)
            {
                send_msg->payload_as_uint32[0] = nodemgmt_get_cred_change_number();
                send_msg->payload_as_uint32[1] = nodemgmt_get_data_change_number();
                send_msg->payload_length = 2*sizeof(uint32_t);
            }
            else
            {
                /* Set failure byte */
                send_msg->payload[0] = HID_1BYTE_NACK;
                send_msg->payload_length = 1;
            }

            return send_msg->payload_length;
        }

        case HID_CMD_SET_CRED_CHANGE_NB :
        {
            /* Check address length */
            if (rcv_msg->payload_length == sizeof(uint32_t))
            {
                /* Store change number */
                nodemgmt_set_cred_change_number(rcv_msg->payload_as_uint32[0]);

                /* Set success byte */                
                send_msg->payload[0] = HID_1BYTE_ACK;
            }
            else
            {
                /* Set failure byte */
                send_msg->payload[0] = HID_1BYTE_NACK;
            }
            
            send_msg->payload_length = 1;
            return 1;
        }

        case HID_CMD_SET_DATA_CHANGE_NB :
        {
            /* Check address length */
            if (rcv_msg->payload_length == sizeof(uint32_t))
            {
                /* Store change number */
                nodemgmt_set_data_change_number(rcv_msg->payload_as_uint32[0]);

                /* Set success byte */                
                send_msg->payload[0] = HID_1BYTE_ACK;
            }
            else
            {
                /* Set failure byte */
                send_msg->payload[0] = HID_1BYTE_NACK;
            }
            
            send_msg->payload_length = 1;
            return 1;
        }

        case HID_CMD_GET_CTR_VALUE:
        {
            /* Read CTR value from flash and send it */
            nodemgmt_read_profile_ctr(send_msg->payload);            
            send_msg->payload_length = MEMBER_SIZE(nodemgmt_profile_main_data_t, current_ctr);
            return MEMBER_SIZE(nodemgmt_profile_main_data_t, current_ctr);
        }

        case HID_CMD_SET_CTR_VALUE:
        {
            if (rcv_msg->payload_length == MEMBER_SIZE(nodemgmt_profile_main_data_t, current_ctr))
            {
                nodemgmt_set_profile_ctr(rcv_msg->payload);

                /* Set success byte */
                send_msg->payload[0] = HID_1BYTE_ACK;         
            }
            else
            {
                /* Set failure byte */
                send_msg->payload[0] = HID_1BYTE_NACK;
            }
            
            send_msg->payload_length = 1;
            return 1;
        }

        case HID_CMD_SET_FAVORITE:
        {
            if (rcv_msg->payload_length == 4*sizeof(uint16_t))
            {
                nodemgmt_set_favorite(rcv_msg->payload_as_uint16[0], rcv_msg->payload_as_uint16[1], rcv_msg->payload_as_uint16[2], rcv_msg->payload_as_uint16[3]);
                
                /* Set success byte */
                send_msg->payload[0] = HID_1BYTE_ACK;
            }
            else
            {
                /* Set failure byte */
                send_msg->payload[0] = HID_1BYTE_NACK;
            }
            
            send_msg->payload_length = 1;
            return 1;            
        }

        case HID_CMD_GET_FAVORITE:
        {
            if (rcv_msg->payload_length == 2*sizeof(uint16_t))
            {
                nodemgmt_read_favorite(rcv_msg->payload_as_uint16[0], rcv_msg->payload_as_uint16[1], &(send_msg->payload_as_uint16[0]), &(send_msg->payload_as_uint16[1]));
                send_msg->payload_length = 2*sizeof(uint16_t);
            }
            else
            {
                /* Set failure byte */
                send_msg->payload[0] = HID_1BYTE_NACK;
                send_msg->payload_length = 1;
            }
            
            return send_msg->payload_length;            
        }
        
        case HID_CMD_ID_GET_CRED:
        {
            /* Default answer: Nope! */
            send_msg->payload_length = 0;
            
            /* Incorrect service name index */
            if (rcv_msg->get_credential_request.service_name_index != 0)
            {
                return 0;
            }
            
            /* Empty service name */
            if (rcv_msg->get_credential_request.concatenated_strings[0] == 0)
            {
                return 0;
            }
            
            /* Max string length */
            uint16_t max_cust_char_length = (sizeof(rcv_msg->payload) \
                                            - sizeof(rcv_msg->get_credential_request.service_name_index) \
                                            - sizeof(rcv_msg->get_credential_request.login_name_index))/sizeof(cust_char_t);
            
            /* Get service string length */
            uint16_t service_length = utils_strnlen(&(rcv_msg->get_credential_request.concatenated_strings[0]), max_cust_char_length);
            
            /* Too long length */
            if (service_length == max_cust_char_length)
            {
                return 0;
            }
            
            /* Reduce max length */
            max_cust_char_length -= (service_length + 1);
            
            /* Is the login specified? */
            if (rcv_msg->get_credential_request.login_name_index == UINT16_MAX)
            { 
                /* Request user to send credential */
                int16_t payload_length = logic_user_get_credential(&(rcv_msg->get_credential_request.concatenated_strings[0]), 0, send_msg);
                if (payload_length < 0)
                {
                    return 0;
                }
                else
                {
                    send_msg->payload_length = payload_length;
                    return payload_length;
                }                
            } 
            else
            {
                /* Check correct index */
                if (rcv_msg->get_credential_request.login_name_index != service_length + 1)
                {
                    return 0;
                }
                
                /* Check login format */
                uint16_t login_length = utils_strnlen(&(rcv_msg->get_credential_request.concatenated_strings[rcv_msg->get_credential_request.login_name_index]), max_cust_char_length);
                
                /* Too long length */
                if (login_length == max_cust_char_length)
                {
                    return 0;
                }
                
                /* Request user to send credential */
                int16_t payload_length = logic_user_get_credential(&(rcv_msg->get_credential_request.concatenated_strings[0]), &(rcv_msg->get_credential_request.concatenated_strings[rcv_msg->get_credential_request.login_name_index]), send_msg);
                if (payload_length < 0)
                {
                    return 0;
                }
                else
                {
                    send_msg->payload_length = payload_length;
                    return payload_length;
                }
            }
        }
        
        case HID_CMD_ID_STORE_CRED:
        {   
            /* Default answer: Nope! */
            send_msg->payload[0] = HID_1BYTE_NACK;
            send_msg->payload_length = 1;
            
            /********************************/
            /* Here comes the sanity checks */
            /********************************/
            
            /* Incorrect service name index */
            if (rcv_msg->store_credential.service_name_index != 0)
            {
                return 1;
            }
            
            /* Empty service name */
            if (rcv_msg->store_credential.concatenated_strings[0] == 0)
            {
                return 1;
            }
            
            /* Sequential order check */
            uint16_t temp_length = 0;
            uint16_t prev_length = 0;
            uint16_t prev_check_index = 0;
            uint16_t current_check_index = 0;
            uint16_t check_indexes[5] = {   rcv_msg->store_credential.service_name_index, \
                                            rcv_msg->store_credential.login_name_index, \
                                            rcv_msg->store_credential.description_index, \
                                            rcv_msg->store_credential.third_field_index, \
                                            rcv_msg->store_credential.password_index};
            uint16_t max_cust_char_length = (sizeof(rcv_msg->payload) \
                                            - sizeof(rcv_msg->store_credential.service_name_index) \
                                            - sizeof(rcv_msg->store_credential.login_name_index) \
                                            - sizeof(rcv_msg->store_credential.description_index) \
                                            - sizeof(rcv_msg->store_credential.third_field_index) \
                                            - sizeof(rcv_msg->store_credential.password_index))/sizeof(cust_char_t);
            
            /* Check all fields */                              
            for (uint16_t i = 0; i < ARRAY_SIZE(check_indexes); i++)
            {
                current_check_index = check_indexes[i];
                
                /* If index indicates present field */
                if (current_check_index != UINT16_MAX)
                {
                    /* If index is correct */
                    if (current_check_index != prev_check_index + prev_length)
                    {
                        return 1;
                    }
                    
                    /* Get string length */
                    temp_length = utils_strnlen(&(rcv_msg->store_credential.concatenated_strings[current_check_index]), max_cust_char_length);
                    
                    /* Too long length */
                    if (temp_length == max_cust_char_length)
                    {
                        return 1;
                    }
                    
                    /* Store previous index & length */
                    prev_length = temp_length + 1;
                    prev_check_index = current_check_index;
                    
                    /* Reduce max length */
                    max_cust_char_length -= (temp_length + 1);
                }              
            }    
            
            /* Dirty hack for fields not set */
            uint16_t empty_string_index = utils_strlen(rcv_msg->store_credential.concatenated_strings);
            if (rcv_msg->store_credential.login_name_index == UINT16_MAX)
            {
                rcv_msg->store_credential.login_name_index = empty_string_index;
            }
            
            /* In case we only want to update certain fields */
            cust_char_t* description_field_pt = (cust_char_t*)0;
            cust_char_t* third_field_pt = (cust_char_t*)0;
            cust_char_t* password_field_pt = (cust_char_t*)0;
            if (rcv_msg->store_credential.description_index != UINT16_MAX)
            {
                description_field_pt = &(rcv_msg->store_credential.concatenated_strings[rcv_msg->store_credential.description_index]);
            }
            if (rcv_msg->store_credential.third_field_index != UINT16_MAX)
            {
                third_field_pt = &(rcv_msg->store_credential.concatenated_strings[rcv_msg->store_credential.third_field_index]);
            }
            if (rcv_msg->store_credential.password_index != UINT16_MAX)
            {
                password_field_pt = &(rcv_msg->store_credential.concatenated_strings[rcv_msg->store_credential.password_index]);
            }
            
            /* Proceed to other logic */
            if (logic_user_store_credential(    &(rcv_msg->store_credential.concatenated_strings[rcv_msg->store_credential.service_name_index]),\
                                                &(rcv_msg->store_credential.concatenated_strings[rcv_msg->store_credential.login_name_index]),\
                                                description_field_pt, third_field_pt, password_field_pt) == RETURN_OK)
            {
                send_msg->payload[0] = HID_1BYTE_ACK;                
            }
            
            return 1;
        }
        
        default: break;
    }
    
    return -1;
}
