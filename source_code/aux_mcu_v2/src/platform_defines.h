/*!  \file     platform_defines.h
*    \brief    Defines specific to our platform: SERCOMs, GPIOs...
*    Created:  22/08/2018
*    Author:   Mathieu Stephan
*/
#ifndef PLATFORM_DEFINES_H_
#define PLATFORM_DEFINES_H_

#ifndef BOOTLOADER
    #include <asf.h>
#endif  
#include "defines.h"

/**************** FIRMWARE DEFINES ****************/
#define FW_MAJOR    0
#define FW_MINOR    4

/* Changelog:
v0.2: added padding to USB comms 64B packet
v0.3: sleep problems solved
v0.4: discarding USB messages having the incorrect flip bit

*/

/**************** SETUP DEFINES ****************/
/*  This project should be built differently
 *  depending on the Mooltipass version.
 *  Simply define one of these:
 *
 *  PLAT_V3_SETUP
 *  => MiniBLE v2 breakout, 13/06/2018
 * - MAIN_MCU_WAKE removed (AUX_Tx used for wakeup)
 * - FORCE_RESET_AUX removed (little added benefits)
 * - NO_COMMS added (see github pages)
 */
 #define PLAT_V3_SETUP
 
/* Features depending on the defined platform */
#if defined(PLAT_V3_SETUP)
     #define NO_SECURITY_BIT_CHECK
#endif

/* USB defines */
#define USB_VENDOR_ID               0x1209              // Vendor ID
#define USB_PRODUCT_ID              0x4321              // Product ID
#define USB_RAWHID_USAGE_PAGE       0xFF31              // HID usage page, after 0xFF00: vendor-defined
#define USB_RAWHID_USAGE            0x0074              // HID usage
#define USB_STR_MANUFACTURER_VAL    "SE"                // Manufacturer string
#define USB_STR_PRODUCT_VAL         "Mooltipass"        // Product string
#define USB_RAWHID_INTERFACE        0                   // Interface for the raw HID
#define USB_RAWHID_RX_ENDPOINT      1                   // Raw HID TX endpoint
#define USB_RAWHID_TX_ENDPOINT      2                   // Raw HID RX endpoint
#define USB_RAWHID_TX_SIZE          64                  // Raw HID transmit packet size
#define USB_RAWHID_RX_SIZE          64                  // Raw HID receive packet size
#define USB_KEYBOARD_INTERFACE      1                   // Interface for keyboard
#define USB_KEYBOARD_ENDPOINT       3                   // Endpoint number for keyboard
#define USB_KEYBOARD_SIZE           8                   // Endpoint size for keyboard
#define USB_WRITE_TIMEOUT   50                          // Timeout for writing in the pipe
#define USB_READ_TIMEOUT 4                              // Timeout for reading in the pipe

/* Bluetooth defies */
#define BLE_PLATFORM_NAME           "Mooltipass Mini"

/* Enums */
typedef enum {PIN_GROUP_0 = 0, PIN_GROUP_1 = 1} pin_group_te;
typedef uint32_t PIN_MASK_T;
typedef uint32_t PIN_ID_T;

/*********************************************/
/*  Bootloader and memory related defines    */
/*********************************************/
#ifdef FEATURE_NVM_RWWEE
    #define RWWEE_MEMORY ((volatile uint16_t *)NVMCTRL_RWW_EEPROM_ADDR)
#endif
/* Simply an array point to our internal memory */
#define NVM_MEMORY ((volatile uint16_t *)FLASH_ADDR)
/* Our firmware start address */
#define APP_START_ADDR (0x800)
/* Internal storage slot for settings storage */
#define SETTINGS_STORAGE_SLOT   0

/* GCLK ID defines */
#define GCLK_ID_48M             GCLK_CLKCTRL_GEN_GCLK0_Val
#define GCLK_ID_32K             GCLK_CLKCTRL_GEN_GCLK3_Val

/* ADC defines */
#if defined(PLAT_V3_SETUP)
    #define HCURSENSE_ADC_PIN_MUXPOS    ADC_INPUTCTRL_MUXPOS_PIN18_Val
    #define LCURSENSE_ADC_PIN_MUXPOS    ADC_INPUTCTRL_MUXPOS_PIN19_Val
#endif

/* SERCOM defines */
#if defined(PLAT_V3_SETUP)
    #define AUXMCU_GCLK_SERCOM_ID       GCLK_CLKCTRL_ID_SERCOM3_CORE_Val
    #define AUXMCU_APB_SERCOM_BIT       SERCOM3_
    #define AUXMCU_SERCOM               SERCOM3
    #define AUXMCU_RX_TXPO              1
    #define AUXMCU_TX_PAD               0
#endif

/* DMA channel descriptors */
#define DMA_DESCID_RX_COMMS         0
#define DMA_DESCID_TX_COMMS         1

/* External interrupts numbers */
#if defined(PLAT_V3_SETUP)
    #define NOCOMMS_EXTINT_NUM      2
    #define NOCOMMS_EIC_SENSE_REG   SENSE2
#endif

/* SERCOM trigger for flash data transfers */
#if defined(PLAT_V3_SETUP)
    #define AUX_MCU_SERCOM_RXTRIG           0x07
    #define AUX_MCU_SERCOM_TXTRIG           0x08
#endif

/* Speed defines */
#define CPU_SPEED_HF                48000000UL
#define CPU_SPEED_MF                8000000UL

/* PORT defines */
/* AUX MCU COMMS */
#if defined(PLAT_V3_SETUP)
    #define AUX_MCU_TX_GROUP        PIN_GROUP_0
    #define AUX_MCU_TX_PINID        16
#endif
#define AUX_MCU_TX_MASK             (1UL << AUX_MCU_TX_PINID)
#define AUX_MCU_TX_PMUX_ID          PORT_PMUX_PMUXO_D_Val
#if (AUX_MCU_TX_PINID % 2) == 1
    #define AUX_MCU_TX_PMUXREGID  PMUXO
#else
    #define AUX_MCU_TX_PMUXREGID  PMUXE
#endif

#if defined(PLAT_V3_SETUP)
    #define AUX_MCU_RX_GROUP        PIN_GROUP_0
    #define AUX_MCU_RX_PINID        17
#endif
#define AUX_MCU_RX_MASK             (1UL << AUX_MCU_RX_PINID)
#define AUX_MCU_RX_PMUX_ID          PORT_PMUX_PMUXO_D_Val
#if (AUX_MCU_RX_PINID % 2) == 1
    #define AUX_MCU_RX_PMUXREGID  PMUXO
#else
    #define AUX_MCU_RX_PMUXREGID  PMUXE
#endif

#if defined(PLAT_V3_SETUP)
    #define AUX_MCU_NOCOMMS_GROUP   PIN_GROUP_0
    #define AUX_MCU_NOCOMMS_PINID   18
#endif
#define AUX_MCU_NOCOMMS_MASK        (1UL << AUX_MCU_NOCOMMS_PINID)
#if (AUX_MCU_NOCOMMS_PINID % 2) == 1
    #define AUX_MCU_NOCOMMS_PMUXREGID  PMUXO
#else
    #define AUX_MCU_NOCOMMS_PMUXREGID  PMUXE
#endif

/* NiMH charging */
#if defined(PLAT_V3_SETUP)
    #define CHARGE_EN_GROUP   PIN_GROUP_0
    #define CHARGE_EN_PINID   22
#endif
#define CHARGE_EN_MASK        (1UL << CHARGE_EN_PINID)
#if (CHARGE_EN_PINID % 2) == 1
    #define CHARGE_EN_PMUXREGID  PMUXO
#else
    #define CHARGE_EN_PMUXREGID  PMUXE
#endif
#if defined(PLAT_V3_SETUP)
    #define MOS_CHARGE_GROUP   PIN_GROUP_0
    #define MOS_CHARGE_PINID   23
#endif
#define MOS_CHARGE_MASK        (1UL << MOS_CHARGE_PINID)
#if (MOS_CHARGE_PINID % 2) == 1
    #define MOS_CHARGE_PMUXREGID  PMUXO
#else
    #define MOS_CHARGE_PMUXREGID  PMUXE
#endif

#if defined(PLAT_V3_SETUP)
    #define HIGH_CUR_SENSE_GROUP     PIN_GROUP_0
    #define HIGH_CUR_SENSE_PINID     10
    #define HIGH_CUR_SENSE_MASK      (1UL << HIGH_CUR_SENSE_PINID)
    #define HIGH_CUR_SENSE_PMUX_ID   PORT_PMUX_PMUXE_B_Val
    #if (HIGH_CUR_SENSE_PINID % 2) == 1
        #define HIGH_CUR_SENSE_PMUXREGID PMUXO
    #else
        #define HIGH_CUR_SENSE_PMUXREGID PMUXE
    #endif
#endif

#if defined(PLAT_V3_SETUP)
    #define LOW_CUR_SENSE_GROUP     PIN_GROUP_0
    #define LOW_CUR_SENSE_PINID     11
    #define LOW_CUR_SENSE_MASK      (1UL << LOW_CUR_SENSE_PINID)
    #define LOW_CUR_SENSE_PMUX_ID   PORT_PMUX_PMUXE_B_Val
    #if (LOW_CUR_SENSE_PINID % 2) == 1
        #define LOW_CUR_SENSE_PMUXREGID PMUXO
    #else
        #define LOW_CUR_SENSE_PMUXREGID PMUXE
    #endif
#endif

#if defined(PLAT_V3_SETUP)
    #define DAC_GROUP     PIN_GROUP_0
    #define DAC_PINID     2
    #define DAC_MASK      (1UL << DAC_PINID)
    #define DAC_PMUX_ID   PORT_PMUX_PMUXE_B_Val
    #if (DAC_PINID % 2) == 1
        #define DAC_PMUXREGID PMUXO
    #else
        #define DAC_PMUXREGID PMUXE
    #endif
#endif

/* USB pads */
#if defined(PLAT_V3_SETUP)
    #define USB_DM_GROUP        PIN_GROUP_0
    #define USB_DM_PINID        24
#endif
#define USB_DM_MASK             (1UL << USB_DM_PINID)
#define USB_DM_PMUX_ID          PORT_PMUX_PMUXO_G_Val
#if (USB_DM_PINID % 2) == 1
    #define USB_DM_PMUXREGID    PMUXO
#else
    #define USB_DM_PMUXREGID    PMUXE
#endif

#if defined(PLAT_V3_SETUP)
    #define USB_DP_GROUP        PIN_GROUP_0
    #define USB_DP_PINID        25
#endif
#define USB_DP_MASK             (1UL << USB_DP_PINID)
#define USB_DP_PMUX_ID          PORT_PMUX_PMUXO_G_Val
#if (USB_DP_PINID % 2) == 1
    #define USB_DP_PMUXREGID    PMUXO
#else
    #define USB_DP_PMUXREGID    PMUXE
#endif

#endif /* PLATFORM_DEFINES_H_ */