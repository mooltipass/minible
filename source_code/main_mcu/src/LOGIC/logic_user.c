/*!  \file     logic_user.c
*    \brief    General logic for user operations
*    Created:  16/02/2019
*    Author:   Mathieu Stephan
*/
#include <string.h>
#include "smartcard_highlevel.h"
#include "logic_encryption.h"
#include "logic_security.h"
#include "logic_database.h"
#include "gui_dispatcher.h"
#include "bearssl_block.h"
#include "driver_timer.h"
#include "platform_io.h"
#include "gui_prompts.h"
#include "logic_user.h"
#include "custom_fs.h"
#include "nodemgmt.h"
#include "utils.h"
#include "rng.h"


/*! \fn     logic_user_init_context(uint8_t user_id)
*   \brief  Initialize our user context
*   \param  user_id The user ID
*/
void logic_user_init_context(uint8_t user_id)
{
    nodemgmt_init_context(user_id);
}

/*! \fn     logic_user_create_new_user(volatile uint16_t* pin_code, uint8_t* provisioned_key)
*   \brief  Add a new user with a new smart card
*   \param  pin_code            The new pin code
*   \param  provisioned_key     If different than 0, provisioned aes key
*   \note   provisioned_key contents will be destroyed!
*   \return success or not
*/
ret_type_te logic_user_create_new_user(volatile uint16_t* pin_code, uint8_t* provisioned_key)
{    
    // When inserting a new user and a new card, we need to setup the following elements
    // - AES encryption key, stored in the smartcard
    // - AES next available CTR, stored in the user profile
    // - AES nonce, stored in the MCU flash along with the user ID
    // - Smartcard CPZ, randomly generated and stored in our MCU flash along with user id & nonce
    uint8_t temp_buffer[AES_KEY_LENGTH/8];
    uint8_t new_user_id;
    
    /* Check if there actually is an available slot */
    if (custom_fs_get_nb_free_cpz_lut_entries(&new_user_id) == 0)
    {
        return RETURN_NOK;
    }
    
    /* Setup user profile in MCU Flash */
    cpz_lut_entry_t user_profile;
    user_profile.user_id = new_user_id;
    
    /* Nonce & Cards CPZ: random numbers */
    rng_fill_array(user_profile.cards_cpz, sizeof(user_profile.cards_cpz));
    rng_fill_array(user_profile.nonce, sizeof(user_profile.nonce));
    
    /* Reserved field: set to 0 */
    memset(user_profile.reserved, 0, sizeof(user_profile.reserved));
    
    /* Setup user profile in external flash */
    nodemgmt_format_user_profile(new_user_id);
    
    /* Initialize nodemgmt context */
    logic_user_init_context(new_user_id);
    
    /* Write card CPZ */
    smartcard_highlevel_write_protected_zone(user_profile.cards_cpz);
    
    /* Write card random AES key */
    rng_fill_array(temp_buffer, sizeof(temp_buffer));
    if (smartcard_highlevel_write_aes_key(temp_buffer) != RETURN_OK)
    {
        return RETURN_NOK;
    }
    
    /* Use provisioned key? */
    if (provisioned_key != 0)
    {        
        /* Set flag in user profile */
        user_profile.use_provisioned_key_flag = CUSTOM_FS_PROV_KEY_FLAG;
        
        /* Buffer for empty ctr */
        uint8_t temp_ctr[AES256_CTR_LENGTH/8];
        memset(temp_ctr, 0, sizeof(temp_ctr));    
        
        /* Use card AES key to encrypt provisioned key */
        br_aes_ct_ctrcbc_keys temp_aes_context;
        br_aes_ct_ctrcbc_init(&temp_aes_context, temp_buffer, AES_KEY_LENGTH/8);
        br_aes_ct_ctrcbc_ctr(&temp_aes_context, (void*)temp_ctr, (void*)provisioned_key, AES_KEY_LENGTH/8);
        
        /* Store encrypted provisioned key in user profile */
        memcpy(user_profile.provisioned_key, provisioned_key, AES_KEY_LENGTH/8);
        
        /* Cleanup */
        memset((void*)&temp_aes_context, 0, sizeof(temp_aes_context));
        memset(provisioned_key, 0, AES_KEY_LENGTH/8);
    }
    else
    {
        user_profile.use_provisioned_key_flag = 0;
        rng_fill_array(user_profile.provisioned_key, sizeof(user_profile.provisioned_key));
    }
    
    /* Write down user profile. Will return OK as we've done the availability check before */
    custom_fs_store_cpz_entry(&user_profile, new_user_id);
    
    /* Initialize encryption context */
    cpz_lut_entry_t* cpz_stored_entry;
    custom_fs_get_cpz_lut_entry(user_profile.cards_cpz, &cpz_stored_entry);
    logic_encryption_init_context(temp_buffer, cpz_stored_entry);
    
    /* Erase AES key from memory */    
    memset(temp_buffer, 0, sizeof(temp_buffer));
    
    /* Write new pin code */
    smartcard_highlevel_write_security_code(pin_code);
    
    /* Remove power to smartcard */
    platform_io_smc_remove_function();

    /* Wait a few ms */
    timer_delay_ms(200);

    /* Reconnect it, test the card */
    if ((smartcard_highlevel_card_detected_routine() == RETURN_MOOLTIPASS_USER) && (smartcard_highlevel_check_hidden_aes_key_contents() == RETURN_OK) && (smartcard_high_level_mooltipass_card_detected_routine(pin_code) == RETURN_MOOLTIPASS_4_TRIES_LEFT))
    {
        return RETURN_OK;
    }
    else
    {
        /* Reset smartcard and delete just created user */
        smartcard_high_level_mooltipass_card_detected_routine(pin_code);
        smartcard_highlevel_erase_smartcard();
        custom_fs_detele_user_cpz_lut_entry(new_user_id);

        // Report fail
        return RETURN_NOK;
    }
}

/*! \fn     logic_user_check_credential(cust_char_t* service, cust_char_t* login, cust_char_t* password)
*   \brief  Check if credential exists
*   \param  service     Pointer to service string
*   \param  login       Pointer to login string
*   \param  password    Pointer to password string
*   \return success or not
*/
RET_TYPE logic_user_check_credential(cust_char_t* service, cust_char_t* login, cust_char_t* password)
{
    cust_char_t encrypted_password[MEMBER_SIZE(child_cred_node_t,password)/sizeof(cust_char_t)];
    uint8_t temp_cred_ctr[MEMBER_SIZE(nodemgmt_profile_main_data_t, current_ctr)];
    BOOL prev_gen_credential_flag = FALSE;
    
    /* Smartcard present and unlocked? */
    if (logic_security_is_smc_inserted_unlocked() == FALSE)
    {
        return -1;
    }
    
    /* Does service already exist? */
    uint16_t parent_address = logic_database_search_service(service, COMPARE_MODE_MATCH, TRUE, 0);
    
    /* Service doesn't exist, deny request with a variable timeout for privacy concerns */
    if (parent_address == NODE_ADDR_NULL)
    {
        return RETURN_NOK;
    }

    /* Check if child actually exists */
    uint16_t child_address = logic_database_search_login_in_service(parent_address, login);
            
    /* Check for existing login */
    if (child_address == NODE_ADDR_NULL)
    {
        return RETURN_NOK;
    }
    
    /* Fetch password */
    logic_database_fetch_encrypted_password(child_address, (uint8_t*)encrypted_password, temp_cred_ctr, &prev_gen_credential_flag);
        
    /* User approved, decrypt password */
    logic_encryption_ctr_decrypt((uint8_t*)encrypted_password, temp_cred_ctr, MEMBER_SIZE(child_cred_node_t, password), prev_gen_credential_flag);
        
    /* If old generation password, convert it to unicode */
    if (prev_gen_credential_flag != FALSE)
    {
        _Static_assert(MEMBER_SIZE(child_cred_node_t, password) >= NODEMGMT_OLD_GEN_ASCII_PWD_LENGTH*2 + 2, "Backward compatibility problem");
        utils_ascii_to_unicode((uint8_t*)encrypted_password, NODEMGMT_OLD_GEN_ASCII_PWD_LENGTH);
    }

    /* Finally do the comparison */
    if (utils_custchar_strncmp(encrypted_password, password, sizeof(encrypted_password)/sizeof(cust_char_t)) == 0)
    {
        memset(encrypted_password, 0, sizeof(encrypted_password));
        return RETURN_OK;
    } 
    else
    {
        memset(encrypted_password, 0, sizeof(encrypted_password));
        return RETURN_NOK;
    }
}

/*! \fn     logic_user_store_credential(cust_char_t* service, cust_char_t* login, cust_char_t* desc, cust_char_t* third, cust_char_t* password)
*   \brief  Store new credential
*   \param  service     Pointer to service string
*   \param  login       Pointer to login string
*   \param  desc        Pointer to description string, or 0 if not specified
*   \param  third       Pointer to arbitrary third field, or 0 if not specified
*   \param  password    Pointer to password string, or 0 if not specified
*   \return success or not
*/
RET_TYPE logic_user_store_credential(cust_char_t* service, cust_char_t* login, cust_char_t* desc, cust_char_t* third, cust_char_t* password)
{
    cust_char_t encrypted_password[MEMBER_SIZE(child_cred_node_t, password)/sizeof(cust_char_t)];
    uint8_t temp_cred_ctr_val[MEMBER_SIZE(nodemgmt_profile_main_data_t, current_ctr)];
    
    /* Smartcard present and unlocked? */
    if (logic_security_is_smc_inserted_unlocked() == FALSE)
    {
        return RETURN_NOK;
    }
    
    /* Does service already exist? */
    uint16_t parent_address = logic_database_search_service(service, COMPARE_MODE_MATCH, TRUE, 0);
    uint16_t child_address = NODE_ADDR_NULL;
    
    /* If service exist, does login exist? */
    if (parent_address != NODE_ADDR_NULL)
    {
        child_address = logic_database_search_login_in_service(parent_address, login);
    }

    /* Special case: in MMM and user chose to not be prompted */
    if ((logic_security_is_management_mode_set() == FALSE) || ((logic_encryption_get_user_security_flags() & USER_SEC_FLG_CRED_SAVE_PROMPT_MMM) != 0))
    {        
        /* Prepare prompt text */
        cust_char_t* three_line_prompt_2;
        if (child_address == NODE_ADDR_NULL)
        {
            custom_fs_get_string_from_file(ADD_CRED_TEXT_ID, &three_line_prompt_2, TRUE);
        }
        else
        {
            custom_fs_get_string_from_file(CHANGE_PWD_TEXT_ID, &three_line_prompt_2, TRUE);
        }
        confirmationText_t conf_text_3_lines = {.lines[0]=service, .lines[1]=three_line_prompt_2, .lines[2]=login};
        
        /* Request user approval */
        mini_input_yes_no_ret_te prompt_return = gui_prompts_ask_for_confirmation(3, &conf_text_3_lines, TRUE);
        gui_dispatcher_get_back_to_current_screen();
        
        /* Did the user approve? */
        if (prompt_return != MINI_INPUT_RET_YES)
        {
            return RETURN_NOK;
        }
    }
    
    /* If needed, add service */
    if (parent_address == NODE_ADDR_NULL)
    {
        parent_address = logic_database_add_service(service, SERVICE_CRED_TYPE, 0);
        
        /* Check for operation success */
        if (parent_address == NODE_ADDR_NULL)
        {
            return RETURN_NOK;
        }
    }
    
    /* Fill RNG array with random numbers */
    rng_fill_array((uint8_t*)encrypted_password, sizeof(encrypted_password));
    
    /* Password provided? */
    if (password != 0)
    {
        /* Copy password into array, 0 terminate it */
        utils_strncpy(encrypted_password, password, sizeof(encrypted_password)/sizeof(cust_char_t));
        encrypted_password[(sizeof(encrypted_password)/sizeof(cust_char_t))-1] = 0;
        
        /* CTR encrypt password */
        logic_encryption_ctr_encrypt((uint8_t*)encrypted_password, sizeof(encrypted_password), temp_cred_ctr_val);
    }
    else if (child_address == NODE_ADDR_NULL)
    {
        /* New credential but password somehow not specified */
        encrypted_password[0] = 0;
        
        /* CTR encrypt password */
        logic_encryption_ctr_encrypt((uint8_t*)encrypted_password, sizeof(encrypted_password), temp_cred_ctr_val);
    }
    
    /* Update existing login or create new one? */
    if (child_address != NODE_ADDR_NULL)
    {
        if (password != 0)
        {
            logic_database_update_credential(child_address, desc, third, (uint8_t*)encrypted_password, temp_cred_ctr_val);
        } 
        else
        {
            logic_database_update_credential(child_address, desc, third, 0, 0);
        }
        return RETURN_OK;
    }
    else
    {
        return logic_database_add_credential_for_service(parent_address, login, desc, third, (uint8_t*)encrypted_password, temp_cred_ctr_val);
    }
}

/*! \fn     logic_user_usb_get_credential(cust_char_t* service, cust_char_t* login, hid_message_t* send_msg)
*   \brief  Get credential for service
*   \param  service     Pointer to service string
*   \param  login       Pointer to login string, or 0 if not specified
*   \param  send_msg    Pointer to where to store our answer
*   \return payload size or -1 if error
*/
int16_t logic_user_usb_get_credential(cust_char_t* service, cust_char_t* login, hid_message_t* send_msg)
{
    uint8_t temp_cred_ctr[MEMBER_SIZE(nodemgmt_profile_main_data_t, current_ctr)];
    BOOL prev_gen_credential_flag = FALSE;
    
    /* Smartcard present and unlocked? */
    if (logic_security_is_smc_inserted_unlocked() == FALSE)
    {
        return -1;
    }
    
    /* Does service already exist? */
    uint16_t parent_address = logic_database_search_service(service, COMPARE_MODE_MATCH, TRUE, 0);
    uint16_t child_address = NODE_ADDR_NULL;
    
    /* Service doesn't exist, deny request with a variable timeout for privacy concerns */
    if (parent_address == NODE_ADDR_NULL)
    {
        /* From 1s to 3s */
        timer_delay_ms(1000 + (rng_get_random_uint16_t()&0x07FF));
        return -1;
    }    
    
    /* See how many credentials there are for this service */
    uint16_t nb_logins_for_cred = logic_database_get_number_of_creds_for_service(parent_address, &child_address);
    
    /* Check if wanted login has been specified or if there's only one credential for that service */
    if ((login != 0) || (nb_logins_for_cred == 1))
    {
        /* Login specified? look for it */
        if (login != 0)
        {
            child_address = logic_database_search_login_in_service(parent_address, login);
            
            /* Check for existing login */
            if (child_address == NODE_ADDR_NULL)
            {
                /* From 3s to 7s */
                timer_delay_ms(3000 + (rng_get_random_uint16_t()&0x0FFF));
                return -1;
            }
        }       
        
        /* Get prefilled message */
        uint16_t return_payload_size_without_pwd = logic_database_fill_get_cred_message_answer(child_address, send_msg, temp_cred_ctr, &prev_gen_credential_flag);
        
        /* If user specified to be prompted for login confirmation */
        if ((logic_encryption_get_user_security_flags() & USER_SEC_FLG_LOGIN_CONF) != 0)
        {
            /* Prepare prompt message */
            cust_char_t* three_line_prompt_2;
            custom_fs_get_string_from_file(SEND_CREDS_FOR_TEXT_ID, &three_line_prompt_2, TRUE);
            confirmationText_t conf_text_3_lines = {.lines[0]=service, .lines[1]=three_line_prompt_2, .lines[2]=&(send_msg->get_credential_answer.concatenated_strings[send_msg->get_credential_answer.login_name_index])};

            /* Request user approval */
            mini_input_yes_no_ret_te prompt_return = gui_prompts_ask_for_confirmation(3, &conf_text_3_lines, TRUE);
            gui_dispatcher_get_back_to_current_screen();

            /* Did the user approve? */
            if (prompt_return != MINI_INPUT_RET_YES)
            {
                memset(send_msg->payload, 0, sizeof(send_msg->payload));
                return -1;
            }
        }
        else
        {
            /* Prepare notification message */
            cust_char_t* three_line_notif_2;
            //custom_fs_get_string_from_file(SEND_CREDS_FOR_TEXT_ID, &three_line_prompt_2, TRUE);
            confirmationText_t notif_text_3_lines = {.lines[0]=service, .lines[1]=three_line_notif_2, .lines[2]=&(send_msg->get_credential_answer.concatenated_strings[send_msg->get_credential_answer.login_name_index])};

            // TODO: 3 lines notification website / logging you in with / username
            gui_prompts_display_3line_information_on_screen(&notif_text_3_lines, DISP_MSG_INFO);

            /* Set information screen, do not call get back to current screen as screen is already updated */
            gui_dispatcher_set_current_screen(GUI_SCREEN_LOGIN_NOTIF, FALSE, GUI_INTO_MENU_TRANSITION);
        }
        
        /* User approved, decrypt password */
        logic_encryption_ctr_decrypt((uint8_t*)&(send_msg->get_credential_answer.concatenated_strings[send_msg->get_credential_answer.password_index]), temp_cred_ctr, MEMBER_SIZE(child_cred_node_t, password), prev_gen_credential_flag);
        
        /* 0 terminate password */
        send_msg->get_credential_answer.concatenated_strings[send_msg->get_credential_answer.password_index + (MEMBER_SIZE(child_cred_node_t, password)/sizeof(cust_char_t)) - 1] = 0;
        
        /* If old generation password, convert it to unicode */
        if (prev_gen_credential_flag != FALSE)
        {
            _Static_assert(MEMBER_SIZE(child_cred_node_t, password) >= NODEMGMT_OLD_GEN_ASCII_PWD_LENGTH*2 + 2, "Backward compatibility problem");
            utils_ascii_to_unicode((uint8_t*)&(send_msg->get_credential_answer.concatenated_strings[send_msg->get_credential_answer.password_index]), NODEMGMT_OLD_GEN_ASCII_PWD_LENGTH);
        }
                
        /* Get password length */
        uint16_t pwd_length = utils_strlen(&(send_msg->get_credential_answer.concatenated_strings[send_msg->get_credential_answer.password_index]));
        
        /* COmpute payload size */
        uint16_t return_payload_size = return_payload_size_without_pwd + (pwd_length + 1)*sizeof(cust_char_t);
        
        /* Return payload size */
        send_msg->payload_length = return_payload_size;
        return return_payload_size;
    }
    else
    {        
        /* 3 cases: no login, 1 login, several logins */
        if (nb_logins_for_cred == 0)
        {
            /* From 1s to 3s */
            timer_delay_ms(1000 + (rng_get_random_uint16_t()&0x07FF));
            return -1;
        }
        else
        {
            /* 2 or more, as 1 is tackled in the previous if */
            uint16_t selected_child_addr = gui_prompts_ask_for_login_select(parent_address);
            gui_dispatcher_get_back_to_current_screen();
            
            /* So.... what did the user select? */
            if (selected_child_addr == NODE_ADDR_NULL)
            {
                return -1;
            }
            else
            {
                /* Get prefilled message */
                uint16_t return_payload_size_without_pwd = logic_database_fill_get_cred_message_answer(selected_child_addr, send_msg, temp_cred_ctr, &prev_gen_credential_flag);
                
                /* User approved, decrypt password */
                logic_encryption_ctr_decrypt((uint8_t*)&(send_msg->get_credential_answer.concatenated_strings[send_msg->get_credential_answer.password_index]), temp_cred_ctr, MEMBER_SIZE(child_cred_node_t, password), prev_gen_credential_flag);
                
                /* 0 terminate password */
                send_msg->get_credential_answer.concatenated_strings[send_msg->get_credential_answer.password_index + (MEMBER_SIZE(child_cred_node_t, password)/sizeof(cust_char_t)) - 1] = 0;
                
                /* If old generation password, convert it to unicode */
                if (prev_gen_credential_flag != FALSE)
                {
                    _Static_assert(MEMBER_SIZE(child_cred_node_t, password) >= NODEMGMT_OLD_GEN_ASCII_PWD_LENGTH*2 + 2, "Backward compatibility problem");
                    utils_ascii_to_unicode((uint8_t*)&(send_msg->get_credential_answer.concatenated_strings[send_msg->get_credential_answer.password_index]), NODEMGMT_OLD_GEN_ASCII_PWD_LENGTH);
                }
                
                /* Get password length */
                uint16_t pwd_length = utils_strlen(&(send_msg->get_credential_answer.concatenated_strings[send_msg->get_credential_answer.password_index]));
                
                /* COmpute payload size */
                uint16_t return_payload_size = return_payload_size_without_pwd + (pwd_length + 1)*sizeof(cust_char_t);
                
                /* Return payload size */
                send_msg->payload_length = return_payload_size;
                return return_payload_size;                
            }
        }
    }    
}

/*! \fn     logic_user_manual_select_login(void)
*   \brief  Logic for finding a given login
*/
void logic_user_manual_select_login(void)
{
    uint16_t chosen_service_addr = nodemgmt_get_starting_parent_addr();
    uint16_t nb_logins_for_cred;
    uint16_t chosen_login_addr;
    uint16_t state_machine = 0;

    while (TRUE)
    {
        if (state_machine == 0)
        {
            /* Ask user to select a service */
            chosen_service_addr = gui_prompts_service_selection_screen(chosen_service_addr);

            /* No service was chosen or card removed */
            if (chosen_service_addr == NODE_ADDR_NULL)
            {
                return;
            }

            /* Continue */
            state_machine++;
        }
        else if (state_machine == 1)
        {
            /* See how many credentials there are for this service */
            nb_logins_for_cred = logic_database_get_number_of_creds_for_service(chosen_service_addr, &chosen_login_addr);

            /* More than one login */
            if (nb_logins_for_cred != 1)
            {
                chosen_login_addr = gui_prompts_ask_for_login_select(chosen_service_addr);
            }

            /* Card removed, user going back... going one step back in the step machine as the previous code can take care of function exit */
            if (chosen_login_addr == NODE_ADDR_NULL)
            {
                state_machine--;
            }
            else
            {
                state_machine++;
            }
        }
        else if (state_machine == 2)
        {
            // Ask the user permission to enter login / password, check for back action
            if (logic_user_ask_for_credentials_keyb_output(chosen_service_addr, chosen_login_addr) == RETURN_BACK)
            {
                /* Depending on number of child nodes, go back in history */
                if (nb_logins_for_cred == 1)
                {
                    state_machine = 0;
                } 
                else
                {
                    state_machine--;
                }
            }
            else
            {
                return;
            }
        }
    }
}

/*! \fn     logic_user_ask_for_credentials_keyb_output(uint16_t parent_address, uint16_t child_address)
*   \brief  Ask the user to enter the login & password of a given service
*   \param  parent_address  Address of the parent
*   \param  child_address   Address of the child
*   \param  RETURN_OK or RETURN_BACK
*/
RET_TYPE logic_user_ask_for_credentials_keyb_output(uint16_t parent_address, uint16_t child_address)
{
    child_cred_node_t* temp_half_cnode_pt;
    parent_node_t temp_half_cnode;
    parent_node_t temp_pnode;
    
    /* Dirty trick */
    temp_half_cnode_pt = (child_cred_node_t*)&temp_half_cnode;

    /* Read nodes */
    nodemgmt_read_parent_node(parent_address, &temp_pnode, TRUE);
    nodemgmt_read_cred_child_node_except_pwd(child_address, temp_half_cnode_pt);

    /* Prepare first line display (service / user), store it in the service field. fields are 0 terminated by previous calls */
    if (utils_strlen(temp_pnode.cred_parent.service) + utils_strlen(temp_half_cnode_pt->login) + 4 <= (uint16_t)MEMBER_ARRAY_SIZE(parent_cred_node_t, service))
    {
        uint16_t parent_length = utils_strlen(temp_pnode.cred_parent.service);
        temp_pnode.cred_parent.service[parent_length] = ' ';
        temp_pnode.cred_parent.service[parent_length+1] = '/';
        temp_pnode.cred_parent.service[parent_length+2] = ' ';
        utils_strcpy(&temp_pnode.cred_parent.service[parent_length+3], temp_half_cnode_pt->login);
    }
    
    /* Prepare prompt and state machine */
    uint16_t state_machine = 0;
    cust_char_t* two_line_prompt_2;
    confirmationText_t conf_text_2_lines = {.lines[0]=temp_pnode.cred_parent.service, .lines[1]=two_line_prompt_2};
    
    while (TRUE)
    {
        if (state_machine == 0)
        {
            // TODO
            /* Ask for login confirmation */
            //custom_fs_get_string_from_file(SEND_CREDS_FOR_TEXT_ID, &two_line_prompt_2, TRUE);
            mini_input_yes_no_ret_te prompt_return = gui_prompts_ask_for_confirmation(2, &conf_text_2_lines, FALSE);

            /* Approved, back, card removed... */
            if (prompt_return == MINI_INPUT_RET_CARD_REMOVED)
            {
                return RETURN_OK;
            } 
            else if (prompt_return == MINI_INPUT_RET_BACK)
            {
                return RETURN_BACK;
            }
            else
            {
                if (prompt_return == MINI_INPUT_RET_YES)
                {
                    // TODO
                }

                /* Move on */
                state_machine++;
            }
        } 
        else if (state_machine == 1)
        {
            // TODO
            /* Ask for password confirmation */
            //custom_fs_get_string_from_file(SEND_CREDS_FOR_TEXT_ID, &two_line_prompt_2, TRUE);
            mini_input_yes_no_ret_te prompt_return = gui_prompts_ask_for_confirmation(2, &conf_text_2_lines, FALSE);

            /* Approved, back, card removed... */
            if (prompt_return == MINI_INPUT_RET_CARD_REMOVED)
            {
                return RETURN_OK;
            }
            else if (prompt_return == MINI_INPUT_RET_BACK)
            {
                state_machine--;
            }
            else
            {
                if (prompt_return == MINI_INPUT_RET_YES)
                {
                    // TODO
                }

                /* Move on */
                return RETURN_OK;
            }
        }
    }

    /*
    confirmationText_t temp_conf_text;
    
    // If the user picked a credential set
    if (child_address != NODE_ADDR_NULL)
    {
        // Read child node
        readChildNode(&temp_cnode, child_address);
        temp_conf_text.lines[0] = service_name;
        
        #ifdef MINI_VERSION
        while(TRUE)
        {
            // If login isn't empty, ask the user if he wants to output the login
            if (temp_cnode.login[0] != 0)
            {
                // Check if we're connected through USB
                if (isUsbConfigured())
                {
                    temp_conf_text.lines[1] = readStoredStringToBuffer(ID_STRING_ENTERLOGINQ);
                    RET_TYPE confirmation_result = guiAskForConfirmation(2, &temp_conf_text);
                    if (confirmation_result == RETURN_OK)
                    {
                        usbKeybPutStr((char*)temp_cnode.login);
                        if (getMooltipassParameterInEeprom(KEY_AFTER_LOGIN_SEND_BOOL_PARAM) != FALSE)
                        {
                            usbKeyboardPress(getMooltipassParameterInEeprom(KEY_AFTER_LOGIN_SEND_PARAM), 0);
                        }
                    }
                    else if (confirmation_result == RETURN_BACK)
                    {
                        return RETURN_BACK;
                    }
                }
                else
                {
                    temp_conf_text.lines[1] = readStoredStringToBuffer(ID_STRING_SHOW_LOGINQ);
                    RET_TYPE confirmation_result = guiAskForConfirmation(2, &temp_conf_text);
                    if (confirmation_result == RETURN_OK)
                    {
                        guiDisplayLoginOrPasswordOnScreen((char*)temp_cnode.login);
                    }
                    else if (confirmation_result == RETURN_BACK)
                    {
                        return RETURN_BACK;
                    }
                }
            }
            
            decrypt32bBlockOfDataAndClearCTVFlag(temp_cnode.password, temp_cnode.ctr);
            temp_cnode.password[sizeof(temp_cnode.password)-1] = 0;
            // Ask the user if he wants to output the password
            if (isUsbConfigured())
            {
                temp_conf_text.lines[1] = readStoredStringToBuffer(ID_STRING_ENTERPASSQ);
                RET_TYPE confirmation_result = guiAskForConfirmation(2, &temp_conf_text);
                if (confirmation_result == RETURN_OK)
                {
                    usbKeybPutStr((char*)temp_cnode.password);
                    memset((void*)temp_cnode.password, 0x00, NODE_CHILD_SIZE_OF_PASSWORD);
                    if (getMooltipassParameterInEeprom(KEY_AFTER_PASS_SEND_BOOL_PARAM) != FALSE)
                    {
                        usbKeyboardPress(getMooltipassParameterInEeprom(KEY_AFTER_PASS_SEND_PARAM), 0);
                    }
                    return RETURN_OK;
                }
                else if (confirmation_result == RETURN_BACK)
                {
                    if (temp_cnode.login[0] != 0)
                    {
                        continue;
                    }
                    else
                    {
                        return RETURN_BACK;
                    }
                }
                else
                {
                    return RETURN_NOK;
                }
            }
            else
            {
                temp_conf_text.lines[1] = readStoredStringToBuffer(ID_STRING_SHOW_PASSQ);
                RET_TYPE confirmation_result = guiAskForConfirmation(2, &temp_conf_text);
                if (confirmation_result == RETURN_OK)
                {
                    guiDisplayLoginOrPasswordOnScreen((char*)temp_cnode.password);
                    memset((void*)temp_cnode.password, 0x00, NODE_CHILD_SIZE_OF_PASSWORD);
                    return RETURN_OK;
                }
                else if (confirmation_result == RETURN_BACK)
                {
                    if (temp_cnode.login[0] != 0)
                    {
                        continue;
                    }
                    else
                    {
                        return RETURN_BACK;
                    }
                }
                else
                {
                    return RETURN_NOK;
                }
            }
        }
        #else
        // If login isn't empty, ask the user if he wants to output the login
        if (temp_cnode.login[0] != 0)
        {
            // Check if we're connected through USB
            if (isUsbConfigured())
            {
                temp_conf_text.lines[1] = readStoredStringToBuffer(ID_STRING_ENTERLOGINQ);
                if (guiAskForConfirmation(2, &temp_conf_text) == RETURN_OK)
                {
                    usbKeybPutStr((char*)temp_cnode.login);
                    if (getMooltipassParameterInEeprom(KEY_AFTER_LOGIN_SEND_BOOL_PARAM) != FALSE)
                    {
                        usbKeyboardPress(getMooltipassParameterInEeprom(KEY_AFTER_LOGIN_SEND_PARAM), 0);
                    }
                }
            }
            else
            {
                temp_conf_text.lines[1] = readStoredStringToBuffer(ID_STRING_SHOW_LOGINQ);
                if (guiAskForConfirmation(2, &temp_conf_text) == RETURN_OK)
                {
                    guiDisplayLoginOrPasswordOnScreen((char*)temp_cnode.login);
                }
            }
        }
        
        decrypt32bBlockOfDataAndClearCTVFlag(temp_cnode.password, temp_cnode.ctr);
        // Ask the user if he wants to output the password
        if (isUsbConfigured())
        {
            temp_conf_text.lines[1] = readStoredStringToBuffer(ID_STRING_ENTERPASSQ);
            if (guiAskForConfirmation(2, &temp_conf_text) == RETURN_OK)
            {
                usbKeybPutStr((char*)temp_cnode.password);
                memset((void*)temp_cnode.password, 0x00, NODE_CHILD_SIZE_OF_PASSWORD);
                if (getMooltipassParameterInEeprom(KEY_AFTER_PASS_SEND_BOOL_PARAM) != FALSE)
                {
                    usbKeyboardPress(getMooltipassParameterInEeprom(KEY_AFTER_PASS_SEND_PARAM), 0);
                }
            }
        }
        else
        {
            temp_conf_text.lines[1] = readStoredStringToBuffer(ID_STRING_SHOW_PASSQ);
            if (guiAskForConfirmation(2, &temp_conf_text) == RETURN_OK)
            {
                guiDisplayLoginOrPasswordOnScreen((char*)temp_cnode.password);
                memset((void*)temp_cnode.password, 0x00, NODE_CHILD_SIZE_OF_PASSWORD);
            }
        }
        #endif
    }*/

    return RETURN_OK;
}