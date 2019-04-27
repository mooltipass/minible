/*!  \file     smartcard_highlevel.c
*    \brief    High level driver for AT88SC102 smartcard
*    Created:  29/11/2017
*    Author:   Mathieu Stephan
*/
#include "smartcard_highlevel.h"
#include "smartcard_lowlevel.h"
#include "platform_defines.h"
#include "main.h"
#include <string.h>


/*! \fn     smartcard_highlevel_read_aes_key(uint8_t* buffer)
*   \brief  Read the AES 256 bits key from the card. Note that it is up to the code calling this function to check that we're authenticated, otherwise 0s will be read
*   \param  buffer  Buffer to store the AES key
*/
void smartcard_highlevel_read_aes_key(uint8_t* buffer)
{
    smartcard_lowlevel_read_smc((SMARTCARD_AZ1_BIT_START + SMARTCARD_AZ1_BIT_RESERVED + AES_KEY_LENGTH)/8, (SMARTCARD_AZ1_BIT_START + SMARTCARD_AZ1_BIT_RESERVED)/8, buffer);
}

/*! \fn     smartcard_highlevel_read_second_aes_key(uint8_t* buffer)
*   \brief  Read the second AES 256 bits key from the card. Note that it is up to the code calling this function to check that we're authenticated, otherwise 0s will be read
*   \param  buffer  Buffer to store the AES key
*/
void smartcard_highlevel_read_second_aes_key(uint8_t* buffer)
{
    smartcard_lowlevel_read_smc((SMARTCARD_AZ2_BIT_START + SMARTCARD_AZ2_BIT_RESERVED + AES_KEY_LENGTH)/8, (SMARTCARD_AZ2_BIT_START + SMARTCARD_AZ2_BIT_RESERVED)/8, buffer);
}

/*! \fn     smartcard_highlevel_read_application_zone1(uint8_t* buffer)
*   \brief  Read Application Zone 1 data
*   \param  buffer  Buffer to store the data
*/
void smartcard_highlevel_read_application_zone1(uint8_t* buffer)
{
    smartcard_lowlevel_read_smc((SMARTCARD_AZ1_BIT_START + SMARTCARD_AZ_BIT_LENGTH)/8, (SMARTCARD_AZ1_BIT_START)/8, buffer);
}

/*! \fn     smartcard_highlevel_write_application_zone1(uint8_t* buffer)
*   \brief  Write Application Zone 1 data
*   \param  buffer  Data to be written
*/
void smartcard_highlevel_write_application_zone1(uint8_t* buffer)
{
    smartcard_lowlevel_write_smc(SMARTCARD_AZ1_BIT_START, SMARTCARD_AZ_BIT_LENGTH, buffer);
}

/*! \fn     smartcard_highlevel_read_application_zone2(uint8_t* buffer)
*   \brief  Read Application Zone 2 data
*   \param  buffer  Buffer to store the data
*/
void smartcard_highlevel_read_application_zone2(uint8_t* buffer)
{
    smartcard_lowlevel_read_smc((SMARTCARD_AZ2_BIT_START + SMARTCARD_AZ_BIT_LENGTH)/8, (SMARTCARD_AZ2_BIT_START)/8, buffer);
}

/*! \fn     smartcard_highlevel_write_application_zone2(uint8_t* buffer)
*   \brief  Write Application Zone 2 data
*   \param  buffer  Data to be written
*/
void smartcard_highlevel_write_application_zone2(uint8_t* buffer)
{
    smartcard_lowlevel_write_smc(SMARTCARD_AZ2_BIT_START, SMARTCARD_AZ_BIT_LENGTH, buffer);
}

/*! \fn     smartcard_highlevel_read_card_login(uint8_t* buffer)
*   \brief  Read the Mooltipass website login from the card. Note that it is up to the code calling this function to check that we're authenticated, otherwise 0s will be read
*   \param  buffer  Buffer to store the login
*/
void smartcard_highlevel_read_card_login(uint8_t* buffer)
{
    // We take the space left in AZ2 -> 30 bytes (512 - 256 - 16 = 30 bytes)
    smartcard_lowlevel_read_smc((SMARTCARD_AZ2_BIT_START + SMARTCARD_AZ2_BIT_RESERVED + AES_KEY_LENGTH + SMARTCARD_MTP_LOGIN_LENGTH)/8, (SMARTCARD_AZ2_BIT_START + SMARTCARD_AZ2_BIT_RESERVED + AES_KEY_LENGTH)/8, buffer);
}

/*! \fn     smartcard_highlevel_read_card_password(uint8_t* buffer)
*   \brief  Read the Mooltipass website password from the card. Note that it is up to the code calling this function to check that we're authenticated, otherwise 0s will be read
*   \param  buffer  Buffer to store the password
*/
void smartcard_highlevel_read_card_password(uint8_t* buffer)
{
    // We take the space left in AZ1 -> 30 bytes (512 - 256 - 16 = 30 bytes)
    smartcard_lowlevel_read_smc((SMARTCARD_AZ1_BIT_START + SMARTCARD_AZ1_BIT_RESERVED + AES_KEY_LENGTH + SMARTCARD_MTP_PASS_LENGTH)/8, (SMARTCARD_AZ1_BIT_START + SMARTCARD_AZ1_BIT_RESERVED + AES_KEY_LENGTH)/8, buffer);
}

/*! \fn     smartcard_highlevel_read_fab_zone(uint8_t* buffer)
*   \brief  Read the fabrication zone (security mode 1&2)
*   \param  buffer  Pointer to a buffer (2 bytes required)
*   \return The provided pointer
*/
uint8_t* smartcard_highlevel_read_fab_zone(uint8_t* buffer)
{
    smartcard_lowlevel_read_smc(2, 0, buffer);
    return buffer;
}

/*! \fn     smartcard_highlevel_read_mem_test_zone(uint8_t* buffer)
*   \brief  Read the Test zone (security mode 1&2)
*   \param  buffer  Pointer to a buffer (2 bytes required)
*   \return The provided pointer
*/
uint8_t* smartcard_highlevel_read_mem_test_zone(uint8_t* buffer)
{
    smartcard_lowlevel_read_smc(178, 176, buffer);
    return buffer;
}

/*! \fn     smartcard_highlevel_write_mem_test_zone(uint8_t* buffer)
*   \brief  Write in the Test zone (security mode 1&2)
*   \param  buffer  Pointer to a buffer (2 bytes required)
*/
void smartcard_highlevel_write_mem_test_zone(uint8_t* buffer)
{
    smartcard_lowlevel_write_smc(1408, 16, buffer);
}

/*! \fn     smartcard_highlevel_read_manufacturer_zone(uint8_t* buffer)
*   \brief  Read the manufacturer zone (security mode 1&2)
*   \param  buffer  Pointer to a buffer (2 bytes required)
*   \return The provided pointer
*/
uint8_t* smartcard_highlevel_read_manufacturer_zone(uint8_t* buffer)
{
    smartcard_lowlevel_read_smc(180, 178, buffer);
    return buffer;
}

/*! \fn     smartcard_highlevel_read_code_attempts_counter(uint8_t* buffer)
*   \brief  Read the number of code attempts left (security mode 1&2)
*   \param  buffer  Pointer to a buffer (2 bytes required)
*   \return The provided pointer
*/
uint8_t* smartcard_highlevel_read_code_attempts_counter(uint8_t* buffer)
{
    smartcard_lowlevel_read_smc(14, 12, buffer);
    return buffer;
}

/*! \fn     smartcard_highlevel_read_issuer_zone(uint8_t* buffer)
*   \brief  Read the issuer zone (security mode 1&2)
*   \param  buffer  Pointer to a buffer (8 bytes required)
*   \return The provided pointer
*/
uint8_t* smartcard_highlevel_read_issuer_zone(uint8_t* buffer)
{
    smartcard_lowlevel_read_smc(10, 2, buffer);
    return buffer;
}

/*! \fn     smartcard_highlevel_write_issuer_zone(uint8_t* buffer)
*   \brief  Write in the issuer zone (security mode 1 - Authenticated!)
*   \param  buffer  Pointer to a buffer (8 bytes required)
*/
void smartcard_highlevel_write_issuer_zone(uint8_t* buffer)
{
    smartcard_lowlevel_write_smc(16, 64, buffer);
}

/*! \fn     smartcard_highlevel_read_code_protected_zone(uint8_t* buffer)
*   \brief  Read the code protected zone (security mode 1&2 - Authenticated!)
*   \param  buffer  Pointer to a buffer (8 bytes required)
*   \return The provided pointer
*/
uint8_t* smartcard_highlevel_read_code_protected_zone(uint8_t* buffer)
{
    smartcard_lowlevel_read_smc(22, 14, buffer);
    return buffer;
}

/*! \fn     smartcard_highlevel_write_protected_zone(uint8_t* buffer)
*   \brief  Write in the code protected zone (security mode 1&2 - Authenticated!)
*   \param  buffer  Pointer to a buffer (8 bytes required)
*/
void smartcard_highlevel_write_protected_zone(uint8_t* buffer)
{
    smartcard_lowlevel_write_smc(112, 64, buffer);
}

/*! \fn     smartcard_highlevel_read_appzone1_erase_key(uint8_t* buffer)
*   \brief  Read the application zone1 erase key (security mode 1 - Authenticated!)
*   \param  buffer  Pointer to a buffer (6 bytes required)
*   \return The provided pointer
*/
uint8_t* smartcard_highlevel_read_appzone1_erase_key(uint8_t* buffer)
{
    smartcard_lowlevel_read_smc(92, 86, buffer);
    return buffer;
}

/*! \fn     smartcard_highlevel_write_appzone1_erase_key(uint8_t* buffer)
*   \brief  Write the application zone1 erase key (security mode 1 - Authenticated!)
*   \param  buffer  Pointer to a buffer (6 bytes required)
*/
void smartcard_highlevel_write_appzone1_erase_key(uint8_t* buffer)
{
    smartcard_lowlevel_write_smc(688, 48, buffer);
}

/*! \fn     smartcard_highlevel_read_appzone2_erase_key(uint8_t* buffer)
*   \brief  Read the application zone2 erase key (security mode 1 - Authenticated!)
*   \param  buffer  Pointer to a buffer (4 bytes required)
*   \return The provided pointer
*/
uint8_t* smartcard_highlevel_read_appzone2_erase_key(uint8_t* buffer)
{
    smartcard_lowlevel_read_smc(160, 156, buffer);
    return buffer;
}

/*! \fn     smartcard_highlevel_write_appzone2_erase_key(uint8_t* buffer)
*   \brief  Write the application zone2 erase key (security mode 1 - Authenticated!)
*   \param  buffer  Pointer to a buffer (4 bytes required)
*/
void smartcard_highlevel_write_appzone2_erase_key(uint8_t* buffer)
{
    smartcard_lowlevel_write_smc(1248, 32, buffer);
}

/*! \fn     smartcard_highlevel_write_manufacturer_zone(uint8_t* buffer)
*   \brief  Write in the manufacturer zone (security mode 1 - Authenticated!)
*   \param  buffer  Pointer to a buffer (2 bytes required)
*/
void smartcard_highlevel_write_manufacturer_zone(uint8_t* buffer)
{
    smartcard_lowlevel_write_smc(1424, 16, buffer);
}

/*! \fn     smartcard_highlevel_write_manufacturer_fuse(void)
*   \brief  Write manufacturer fuse, controlling access to the MFZ
*/
void smartcard_highlevel_write_manufacturer_fuse(void)
{
    smartcard_lowlevel_blow_fuse(MAN_FUSE);
}

/*! \fn     smartcard_highlevel_write_issuer_fuse(void)
*   \brief  Write issuers fuse, setting the AT88SC102 into Security Mode 2, we need to be authenticated here
*/
void smartcard_highlevel_write_issuer_fuse(void)
{
    smartcard_lowlevel_blow_fuse(ISSUER_FUSE);
}

/*! \fn     smartcard_highlevel_write_ec2en_fuse(void)
*   \brief  Write ec2en fuse, to be done before blowing issuer fuse
*/
void smartcard_highlevel_write_ec2en_fuse(void)
{
    smartcard_lowlevel_blow_fuse(EC2EN_FUSE);
}

/*! \fn     smartcard_highlevel_check_security_mode2(void)
*   \brief  Check that the smartcard is in mode two by trying to write his manufacturer zone
*   \return Success status
*/
RET_TYPE smartcard_highlevel_check_security_mode2(void)
{
    uint16_t manZoneRead, temp_uint;
    
    // Read manufacturer zone, set temp_uint to its opposite
    smartcard_highlevel_read_manufacturer_zone((uint8_t*)&manZoneRead);
    temp_uint = ~manZoneRead;
    
    // Perform test write
    smartcard_highlevel_write_manufacturer_zone((uint8_t*)&temp_uint);
    smartcard_highlevel_read_manufacturer_zone((uint8_t*)&manZoneRead);
    
    if (temp_uint != manZoneRead)
    {
        return RETURN_OK;
    } 
    else
    {
        return RETURN_NOK;
    }
}

/*! \fn     smartcard_high_level_mooltipass_card_detected_routine(volatile uint16_t* pin_code)
*   \brief  Function called when a Mooltipass is inserted into the smart card slot
*   \param  pin_code    Mooltipass pin code
*   \return If we managed to unlock it / if there is other problem (see mooltipass_detect_return_t)
*/
mooltipass_card_detect_return_te smartcard_high_level_mooltipass_card_detected_routine(volatile uint16_t* pin_code)
{ 
    // Try unlocking card with provided code
    pin_check_return_te temp_rettype = smartcard_lowlevel_validate_code(pin_code);

    if (temp_rettype == RETURN_PIN_OK)                                   // Unlock successful
    {
        // Check that the card is in security mode 2
        if (smartcard_highlevel_check_security_mode2() == RETURN_NOK)
        {
            // Card is in mode 1... how could this happen?
            return RETURN_MOOLTIPASS_PB;
        }
        else                                                            // Everything is in order - proceed
        {
            // Check that read / write accesses are correctly configured
            if (smartcard_highlevel_check_authenticated_readwrite_to_zone12() == RETURN_NOK)
            {
                return RETURN_MOOLTIPASS_PB;
            }
            else
            {
                return RETURN_MOOLTIPASS_4_TRIES_LEFT;
            }
        }
    }
    else                                                                // Unlock failed
    {
        // The enum allows us to do so
        return RETURN_MOOLTIPASS_0_TRIES_LEFT + smartcard_highlevel_get_nb_sec_tries_left();
    }
}

/*! \fn     smartcard_high_level_transform_blank_card_into_mooltipass(void)
*   \brief  Transform the card into a Mooltipass card (Security mode 1 - Authenticated!)
*   \return If we succeeded
*/
RET_TYPE smartcard_high_level_transform_blank_card_into_mooltipass(void)
{
    uint8_t temp_buffer[SMARTCARD_ISSUER_ZONE_LGTH];
    uint16_t default_pin = SMARTCARD_DEFAULT_PIN;

    /* Check that we are in security mode 1 */
    if (smartcard_highlevel_check_security_mode2() == RETURN_OK)
    {
        return RETURN_NOK;
    }

    /* Perform block erase, resetting the entire memory excluding FZ/MTZ/MFZ to FFFF... */
    smartcard_highlevel_reset_blank_card();

    /* Set new security password, keep zone 1 and zone 2 security key to FFFF... */
    smartcard_highlevel_write_security_code(&default_pin);

    /* Write "limpkin" to issuer zone */
    strcpy((char*)temp_buffer, "limpkin");
    smartcard_highlevel_write_issuer_zone(temp_buffer);

    /* Write 2017 to the manufacturer zone */
    uint16_t new_man_zone = swap16(2017);
    smartcard_highlevel_write_manufacturer_zone((uint8_t*)&new_man_zone);

    /* Set application zone 1 and zone 2 permissions: read/write when authenticated only */
    smartcard_highlevel_set_authenticated_readwrite_to_zone1and2();

    /* Burn manufacturer fuse */
    smartcard_highlevel_write_manufacturer_fuse();
    
    /* Burn EC2EN fuse */
    smartcard_highlevel_write_ec2en_fuse();

    /* Burn issuer fuse */
    smartcard_highlevel_write_issuer_fuse();

    return RETURN_OK;
}

/*! \fn     smartcard_highlevel_erase_smartcard(void)
*   \brief  Completely erase mooltipass card (Security mode 2 - Authenticated!)
*/
void smartcard_highlevel_erase_smartcard(void)
{
    uint8_t temp_buffer[SMARTCARD_CPZ_LENGTH];
    uint16_t default_pin = SMARTCARD_DEFAULT_PIN;
    
    // Write 0xFF in CPZ
    memset(temp_buffer, 0xFF, SMARTCARD_CPZ_LENGTH);
    smartcard_highlevel_write_protected_zone(temp_buffer);
    
    // Erase AZ1 & AZ2
    smartcard_lowlevel_erase_application_zone1_nzone2(FALSE);
    smartcard_lowlevel_erase_application_zone1_nzone2(TRUE);

    /* Set application zone 1 and zone 2 permissions: read/write when authenticated only */
    smartcard_highlevel_set_authenticated_readwrite_to_zone1and2();
    
    // Reset default pin code
    smartcard_highlevel_write_security_code(&default_pin);
}

/*! \fn     smartcard_highlevel_write_to_appzone_and_check(uint16_t addr, uint16_t nb_bits, uint8_t* buffer, uint8_t* temp_buffer)
*   \brief  Write to one application zone and check what we wrote
*   \param  addr    Address in bits of the place to write
*   \param  nb_bits Number of bits to write
*   \param  buffer  Buffer containing the data to write
*   \param  temp_buffer A temporary buffer having the same size as buffer
*   \return If we succeeded
*/
RET_TYPE smartcard_highlevel_write_to_appzone_and_check(uint16_t addr, uint16_t nb_bits, uint8_t* buffer, uint8_t* temp_buffer)
{    
    smartcard_lowlevel_write_smc(addr, nb_bits, buffer);
    smartcard_lowlevel_read_smc((addr + nb_bits) >> 3, (addr >> 3), temp_buffer);
    
    if (memcmp(buffer, temp_buffer, (nb_bits >> 3)) == 0)
    {
        return RETURN_OK;
    }
    else
    {
        return RETURN_NOK;
    }    
}

/*! \fn     smartcard_highlevel_write_aes_key(uint8_t* buffer)
*   \brief  Write the AES 256 bits key to the card
*   \param  buffer  Buffer containing the AES key
*   \return Operation success
*/
RET_TYPE smartcard_highlevel_write_aes_key(uint8_t* buffer)
{
    uint8_t temp_buffer[AES_KEY_LENGTH/8];
    return smartcard_highlevel_write_to_appzone_and_check(SMARTCARD_AZ1_BIT_START + SMARTCARD_AZ1_BIT_RESERVED, AES_KEY_LENGTH, buffer, temp_buffer);
}

/*! \fn     smartcard_highlevel_write_second_aes_key(uint8_t* buffer)
*   \brief  Write the second AES 256 bits key to the card
*   \param  buffer  Buffer containing the AES key
*   \return Operation success
*/
RET_TYPE smartcard_highlevel_write_second_aes_key(uint8_t* buffer)
{
    uint8_t temp_buffer[AES_KEY_LENGTH/8];
    return smartcard_highlevel_write_to_appzone_and_check(SMARTCARD_AZ2_BIT_START + SMARTCARD_AZ2_BIT_RESERVED, AES_KEY_LENGTH, buffer, temp_buffer);
}

/*! \fn     smartcard_highlevel_write_card_password(uint8_t* buffer)
*   \brief  Write the Mooltipass website password to the card
*   \param  buffer  Buffer containing the password
*   \return Operation success
*/
RET_TYPE smartcard_highlevel_write_card_password(uint8_t* buffer)
{
    uint8_t temp_buffer[SMARTCARD_MTP_PASS_LENGTH/8];
    return smartcard_highlevel_write_to_appzone_and_check(SMARTCARD_AZ1_BIT_START + SMARTCARD_AZ1_BIT_RESERVED + AES_KEY_LENGTH, SMARTCARD_MTP_PASS_LENGTH, buffer, temp_buffer);
}

/*! \fn     smartcard_highlevel_write_card_login(uint8_t* buffer)
*   \brief  Write the Mooltipass website login to the card
*   \param  buffer  Buffer containing the login
*   \return Operation success
*/
RET_TYPE smartcard_highlevel_write_card_login(uint8_t* buffer)
{
    uint8_t temp_buffer[SMARTCARD_MTP_LOGIN_LENGTH/8];
    return smartcard_highlevel_write_to_appzone_and_check(SMARTCARD_AZ2_BIT_START + SMARTCARD_AZ2_BIT_RESERVED + AES_KEY_LENGTH, SMARTCARD_MTP_LOGIN_LENGTH, buffer, temp_buffer);
}

/*! \fn     smartcard_highlevel_reset_blank_card(void)
*   \brief  Reinitialize the card to its default settings & default pin (Security mode 1 - Authenticated!)
*/
void smartcard_highlevel_reset_blank_card(void)
{
    uint16_t default_pin = SMARTCARD_FACTORY_PIN;
    uint8_t data_buffer[2] = {0xFF, 0xFF};
    
    smartcard_lowlevel_write_smc(1441, 1, data_buffer);
    smartcard_highlevel_write_security_code(&default_pin);
}

/*! \fn     smartcard_highlevel_read_security_code(void)
*   \brief  Read the security code (security mode 1 - Authenticated!)
*   \return The security code
*/
uint16_t smartcard_highlevel_read_security_code(void)
{
    uint16_t temp_uint;
    smartcard_lowlevel_read_smc(12, 10, (uint8_t*)&temp_uint);
    return swap16(temp_uint);
}

/*! \fn     smartcard_highlevel_write_security_code(uint16_t* code)
*   \brief  Write a new security code (security mode 1&2 - Authenticated!)
*   \param  code  The pin code
*/
void smartcard_highlevel_write_security_code(volatile uint16_t* code)
{
    *code = swap16(*code);
    smartcard_lowlevel_write_smc(80, 16, (uint8_t*)code);
    *code = swap16(*code);
}

/*! \fn     smartcard_highlevel_set_authenticated_readwrite_to_zone1(void)
*   \brief  Function called to only allow reads and writes to the application zone 1 when authenticated
*   \return Operation success
*/
RET_TYPE smartcard_highlevel_set_authenticated_readwrite_to_zone1(void)
{
    // Set P1 to 1 to allow write, remove R1 to prevent non authenticated reads
    uint8_t temp_buffer[2] = {0x80, 0x00};
    smartcard_lowlevel_write_smc(176, 16, temp_buffer);
    
    return smartcard_highlevel_check_authenticated_readwrite_to_zone1();
}

/*! \fn     smartcard_highlevel_check_authenticated_readwrite_to_zone1(void)
*   \brief  Function called to check that only reads and writes are allowed to the application zone 1 when authenticated
*   \return OK or NOK
*/
RET_TYPE smartcard_highlevel_check_authenticated_readwrite_to_zone1(void)
{
    uint8_t temp_buffer[2];

    smartcard_lowlevel_read_smc(24, 22, temp_buffer);

    if ((temp_buffer[0] == 0x80) && (temp_buffer[1] == 0x00))
    {
        return RETURN_OK;
    }
    else
    {
        return RETURN_NOK;
    }
}

/*! \fn     smartcard_highlevel_check_hidden_aes_key_contents(void)
*   \brief  Function called to check that the AES key contents are indeed well hidden
*   \return OK or NOK
*/
RET_TYPE smartcard_highlevel_check_hidden_aes_key_contents(void)
{
    for (uint16_t i = 0; i < 20; i++)
    {
        if (smartcard_lowlevel_check_for_const_val_in_smc_array((SMARTCARD_AZ1_BIT_START + SMARTCARD_AZ1_BIT_RESERVED + AES_KEY_LENGTH)/8, (SMARTCARD_AZ1_BIT_START + SMARTCARD_AZ1_BIT_RESERVED)/8, 0xFF) != RETURN_OK)
        {
            return RETURN_NOK;
        }
    }

    return RETURN_OK;
}

/*! \fn     smartcard_highlevel_set_authenticated_readwrite_to_zone2(void)
*   \brief  Function called to only allow reads and writes to the application zone 2 when authenticated
*   \return Operation success
*/
RET_TYPE smartcard_highlevel_set_authenticated_readwrite_to_zone2(void)
{
    // Set P2 to 1 to allow write, remove R2 to prevent non authenticated reads
    uint8_t temp_buffer[2] = {0x80, 0x00};
    smartcard_lowlevel_write_smc(736, 16, temp_buffer);
    
    return smartcard_highlevel_check_authenticated_readwrite_to_zone2();
}

/*! \fn     smartcard_highlevel_set_authenticated_readwrite_to_zone1and2(void)
*   \brief  Function called to only allow reads and writes to the application zone 1 & 2 when authenticated
*/
void smartcard_highlevel_set_authenticated_readwrite_to_zone1and2(void)
{
    uint8_t temp_buffer[2] = {0x80, 0x00};
    // Set P1 to 1 to allow write, remove R1 to prevent non authenticated reads
    smartcard_lowlevel_write_smc(176, 16, temp_buffer);
    // Set P2 to 1 to allow write, remove R2 to prevent non authenticated reads
    smartcard_lowlevel_write_smc(736, 16, temp_buffer);
}

/*! \fn     smartcard_highlevel_check_authenticated_readwrite_to_zone2(void)
*   \brief  Function called to check that only reads and writes are allowed to the application zone 2 when authenticated
*   \return OK or NOK
*/
RET_TYPE smartcard_highlevel_check_authenticated_readwrite_to_zone2(void)
{
    uint8_t temp_buffer[2];

    smartcard_lowlevel_read_smc(94, 92, temp_buffer);

    if ((temp_buffer[0] == 0x80) && (temp_buffer[1] == 0x00))
    {
        return RETURN_OK;
    }
    else
    {
        return RETURN_NOK;
    }
}

/*! \fn     smartcard_highlevel_check_authenticated_readwrite_to_zone12(void)
*   \brief  Function called to check that only reads and writes are allowed to the application zone 1 when authenticated
*   \return OK or NOK
*/
RET_TYPE smartcard_highlevel_check_authenticated_readwrite_to_zone12(void)
{
    uint8_t temp_buffer[2];
    uint8_t temp_buffer2[2];

    smartcard_lowlevel_read_smc(24, 22, temp_buffer);
    smartcard_lowlevel_read_smc(94, 92, temp_buffer2);

    if ((temp_buffer[0] == 0x80) && (temp_buffer[1] == 0x00) && (temp_buffer2[0] == 0x80) && (temp_buffer2[1] == 0x00))
    {
        return RETURN_OK;
    }
    else
    {
        return RETURN_NOK;
    }
}

/*! \fn     smartcard_highlevel_get_nb_az2_writes_left(void)
*   \brief  Get the number of AZ2 writes left in case EC2 is not blown
*   \return Number of tries left
*/
uint8_t smartcard_highlevel_get_nb_az2_writes_left(void)
{
    uint8_t temp_buffer[16];
    uint8_t return_val = 0;
    uint8_t i;

    smartcard_lowlevel_read_smc(176, 160, temp_buffer);
    for(i = 0; i < 128; i++)
    {
        if ((temp_buffer[i>>3] >> (i&0x07)) & 0x01)
        {
            return_val++;
        }
    }

    return return_val;  
}

/*! \fn     smartcard_highlevel_get_nb_sec_tries_left(void)
*   \brief  Get the number of security code tries left
*   \return Number of tries left
*/
uint8_t smartcard_highlevel_get_nb_sec_tries_left(void)
{
    uint8_t temp_buffer[2];
    uint8_t return_val = 0;
    uint8_t i;

    smartcard_highlevel_read_code_attempts_counter(temp_buffer);
    for(i = 0; i < 4; i++)
    {
        if ((temp_buffer[0] >> (4+i)) & 0x01)
        {
            return_val++;
        }
    }

    return return_val;
}

/*! \fn     smartcard_highlevel_card_detected_routine(void)
*   \brief  Function called when something is inserted into the smart card slot
*   \return What the card is / what needs to be done (see mooltipass_detect_return_t)
*/
mooltipass_card_detect_return_te smartcard_highlevel_card_detected_routine(void)
{
    card_detect_return_te card_detection_result;
    uint8_t temp_buffer[SMARTCARD_CPZ_LENGTH];
    uint16_t manufacturer_zone;

    /* Get a first card detection result */
    card_detection_result = smartcard_lowlevel_first_detect_function();

    if (card_detection_result == RETURN_CARD_NDET)
    {
        /* Not a card */
        return RETURN_MOOLTIPASS_INVALID;
    }
    else if (card_detection_result == RETURN_CARD_TEST_PB)
    {
        /* Card test problem */
        return RETURN_MOOLTIPASS_PB;
    }
    else if (card_detection_result == RETURN_CARD_0_TRIES_LEFT)
    {
        /* Card blocked */
        return RETURN_MOOLTIPASS_BLOCKED;
    }
    else
    {
        /* Card is of the correct type and not blocked */
        smartcard_highlevel_read_manufacturer_zone((uint8_t*)&manufacturer_zone);
        
        /* Detect if the card is blank by checking that the manufacturer zone is different from FFFF */
        if (manufacturer_zone == 0xFFFF)
        {
            /* Card is new - transform it into a Mooltipass card */
            uint16_t factory_pin = SMARTCARD_FACTORY_PIN;

            /* Try to authenticate with factory pin */
            pin_check_return_te pin_try_return = smartcard_lowlevel_validate_code(&factory_pin);

            if (pin_try_return == RETURN_PIN_OK)
            {
                /* Card is unlocked - transform */    
                if (smartcard_high_level_transform_blank_card_into_mooltipass() == RETURN_OK)
                {
                    return RETURN_MOOLTIPASS_BLANK;
                }
                else
                {
                    return RETURN_MOOLTIPASS_PB;
                }
            }
            else
            {
                /* Card unlock failed */
                return RETURN_MOOLTIPASS_PB;
            }
        }
        else
        {
            /* Card is already converted into a mooltipass card */
            smartcard_highlevel_read_code_protected_zone(temp_buffer);

            /* Check that the user setup his mooltipass card by checking that the CPZ is different from FFFF.... */
            if (memcmp(temp_buffer, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", SMARTCARD_CPZ_LENGTH) != 0)
            {
                /* Special dev feature to detect special dev card */
                #ifdef SPECIAL_DEVELOPER_CARD_FEATURE
                if (memcmp(temp_buffer, "\x00\x00\x00\x00\x00\x00\x00\x00", SMARTCARD_CPZ_LENGTH) == 0)
                {
                    special_dev_card_inserted = TRUE;
                } 
                else
                {
                    special_dev_card_inserted = FALSE;
                }
                #endif
                return RETURN_MOOLTIPASS_USER;                
            }

            /* If we're here it means the user hasn't configured his blank mooltipass card, so try to unlock it using the default pin */
            uint16_t default_pin = SMARTCARD_DEFAULT_PIN;
            mooltipass_card_detect_return_te card_return = smartcard_high_level_mooltipass_card_detected_routine(&default_pin);

            /* If we unlocked it, it means we can personalize it */
            if (card_return != RETURN_MOOLTIPASS_4_TRIES_LEFT)
            {
                return RETURN_MOOLTIPASS_PB;
            }
            else
            {
                return RETURN_MOOLTIPASS_BLANK;
            }
        }
    }
}
