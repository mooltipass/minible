/*!  \file     logic_user.c
*    \brief    General logic for user operations
*    Created:  16/02/2019
*    Author:   Mathieu Stephan
*/
#include "logic_user.h"
#include "defines.h"


/*! \fn     logic_user_create_new_user(volatile uint16_t* pin_code)
*   \brief  Add a new user with a new smart card
*   \param  pin_code The new pin code
*   \return success or not
*/
ret_type_te logic_user_create_new_user(volatile uint16_t* pin_code)
{
    // TODO
    #ifdef bla
    uint8_t temp_buffer[AES_KEY_LENGTH/8];
    uint8_t temp_nonce[AES256_CTR_LENGTH];
    uint8_t new_user_id, temp_val;
    
    // When inserting a new user and a new card, we need to setup the following elements
    // - AES encryption key, stored in the smartcard
    // - AES next available CTR, stored in the user profile
    // - AES nonce, stored in the eeprom along with the user ID
    // - Smartcard CPZ, randomly generated and stored in our eeprom along with user id & nonce
    
    // The next part can take quite a while
    guiDisplayProcessingScreen();
    
    // Get new user id if possible
    if (findAvailableUserId(&new_user_id, &temp_val) == RETURN_NOK)
    {
        return RETURN_NOK;
    }
    
    // Create user profile in flash, CTR is set to 0 by the library
    formatUserProfileMemory(new_user_id);

    // Initialize user flash context, that inits the node mgmt handle and the ctr value
    initUserFlashContext(new_user_id);

    // Generate random CPZ value
    fillArrayWithRandomBytes(temp_buffer, SMARTCARD_CPZ_LENGTH);

    // Generate random nonce to be stored in the eeprom
    fillArrayWithRandomBytes(temp_nonce, AES256_CTR_LENGTH);
    
    // Store User ID <> SMC CPZ & AES CTR <> user id
    if (writeSmartCardCPZForUserId(temp_buffer, temp_nonce, new_user_id) != RETURN_OK)
    {
        return RETURN_NOK;
    }
    
    // Write random bytes in the code protected zone in the smart card
    writeCodeProtectedZone(temp_buffer);
    
    // Generate a new random AES key, write it in the smartcard
    fillArrayWithRandomBytes(temp_buffer, AES_KEY_LENGTH/8);
    writeAES256BitsKey(temp_buffer);
    
    // Initialize encryption handling
    initEncryptionHandling(temp_buffer, temp_nonce);
    
    // Write new pin code
    writeSecurityCode(pin_code);
    #endif
    
    return RETURN_OK;    
}