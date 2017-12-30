/*!  \file     platform_defines.h
*    \brief    Defines specific to our platform: SERCOMs, GPIOs...
*    Created:  10/11/2017
*    Author:   Mathieu Stephan
*/
#ifndef PLATFORM_DEFINES_H_
#define PLATFORM_DEFINES_H_

#include <asf.h>

/**************** SETUP DEFINES ****************/
/*  This project should be built differently
 *  depending on the Mooltipass version.
 *  Simply define one of these:
 *
 *  PLAT_V1_SETUP
 *  => only 2 boards were made, one shipped to Miguel, one to Mathieu. No silkscreen marking
 *
 *  PLAT_V2_SETUP
 *  => board with the new LT1613 stepup, "beta v2 (new DC/DC)" silkscreen. SMC_POW_nEN pin change.
 */
 #define PLAT_V2_SETUP

/* Enums */
typedef enum {PIN_GROUP_0 = 0, PIN_GROUP_1 = 1} pin_group_te;
typedef uint32_t PIN_MASK_T;
typedef uint32_t PIN_ID_T;

/* Structs */
typedef struct
{
    Sercom* sercom_pt;
    pin_group_te cs_pin_group;
    PIN_MASK_T cs_pin_mask;
} spi_flash_descriptor_t;

/*********************************************/
/*  Bootloader and memory related defines    */
/*********************************************/
#ifdef FEATURE_NVM_RWWEE
    #define RWWEE_MEMORY ((volatile uint16_t *)NVMCTRL_RWW_EEPROM_ADDR)
#endif
/* Simply an array point to our internal memory */
#define NVM_MEMORY ((volatile uint16_t *)FLASH_ADDR)
/* Our firmware start address */
#define APP_START_ADDR (0x1000)
/* Internal storage slot for settings storage */
#define SETTINGS_STORAGE_SLOT   0

/********************/
/* Settings defines */
/********************/
#define FIRMWARE_UPGRADE_FLAG   0x5478ABAA

/*************************/
/* Functionality defines */
/*************************/
/* specify that the flash is alone on the spi bus */
#define FLASH_ALONE_ON_SPI_BUS
/* Use DMA transfers to read from external flash */
#define FLASH_DMA_FETCHES
/* Use DMA transfers to send data to OLED screen */
#define OLED_DMA_TRANSFER
/* allow printf for the screen */
#define OLED_PRINTF_ENABLED

/* GCLK ID defines */
#define GCLK_ID_48M             GCLK_CLKCTRL_GEN_GCLK0_Val
#define GCLK_ID_32K             GCLK_CLKCTRL_GEN_GCLK2_Val

/* SERCOM defines */
#define SMARTCARD_GCLK_SERCOM_ID    GCLK_CLKCTRL_ID_SERCOM5_CORE_Val
#define SMARTCARD_MOSI_SCK_PADS     MOSI_P0_SCK_P3_SS_P1
#define SMARTCARD_MISO_PAD          MISO_PAD1
#define SMARTCARD_APB_SERCOM_BIT    SERCOM5_
#define SMARTCARD_SERCOM            SERCOM5
#define DATAFLASH_GCLK_SERCOM_ID    GCLK_CLKCTRL_ID_SERCOM2_CORE_Val
#define DATAFLASH_MOSI_SCK_PADS     MOSI_P0_SCK_P1_SS_P2
#define DATAFLASH_MISO_PAD          MISO_PAD2
#define DATAFLASH_APB_SERCOM_BIT    SERCOM2_
#define DATAFLASH_SERCOM            SERCOM2
#define DBFLASH_GCLK_SERCOM_ID      GCLK_CLKCTRL_ID_SERCOM3_CORE_Val
#define DBFLASH_MOSI_SCK_PADS       MOSI_P0_SCK_P1_SS_P2
#define DBFLASH_MISO_PAD            MISO_PAD3
#define DBFLASH_APB_SERCOM_BIT      SERCOM3_
#define DBFLASH_SERCOM              SERCOM3
#define AUXMCU_GCLK_SERCOM_ID       GCLK_CLKCTRL_ID_SERCOM4_CORE_Val
#define AUXMCU_APB_SERCOM_BIT       SERCOM4_
#define AUXMCU_SERCOM               SERCOM4
#define OLED_GCLK_SERCOM_ID         GCLK_CLKCTRL_ID_SERCOM0_CORE_Val
#define OLED_MOSI_SCK_PADS          MOSI_P0_SCK_P1_SS_P2
#define OLED_MISO_PAD               MISO_PAD3
#define OLED_APB_SERCOM_BIT         SERCOM0_
#define OLED_SERCOM                 SERCOM0
#define ACC_GCLK_SERCOM_ID          GCLK_CLKCTRL_ID_SERCOM1_CORE_Val
#define ACC_MOSI_SCK_PADS           MOSI_P0_SCK_P3_SS_P1
#define ACC_MISO_PAD                MISO_PAD1
#define ACC_APB_SERCOM_BIT          SERCOM1_
#define ACC_SERCOM                  SERCOM1

/* SERCOM trigger for flash data transfers */
#define DATAFLASH_DMA_SERCOM_RXTRIG     0x05
#define DATAFLASH_DMA_SERCOM_TXTRIG     0x06
#define DBFLASH_DMA_SERCOM_RXTRIG       0x03
#define DBFLASH_DMA_SERCOM_TXTRIG       0x04

/* SERCOM trigger for OLED data transfers */
#define OLED_DMA_SERCOM_TX_TRIG         0x02

/* Speed defines */
#define CPU_SPEED_HF                48000000UL
#define CPU_SPEED_MF                8000000UL
/* SMARTCARD SPI SCK =  48M / (2*(119+1)) = 200kHz (Supposed Max is 300kHz) */
//#define SMARTCARD_BAUD_DIVIDER      119
#define SMARTCARD_BAUD_DIVIDER      239
/* OLED SPI SCK = 48M / 2*(5+1)) = 4MHz (Max from datasheet) */
/* Note: Has successfully been tested at 24MHz, but without speed improvements */
#define OLED_BAUD_DIVIDER           5
/* ACC SPI SCK = 48M / 2*(5+1)) = 4MHz (Max is 10MHz) */
#define ACC_BAUD_DIVIDER            5
/* DATA & DB FLASH SPI SCK = 48M / 2*(0+1)) = 24MHz */
#define DATAFLASH_BAUD_DIVIDER      0
#define DBFLASH_BAUD_DIVIDER        0

/* PORT defines */
/* WHEEL ENCODER */
#define WHEEL_A_GROUP           PIN_GROUP_0
#define WHEEL_A_PINID           0
#define WHEEL_A_MASK            (1UL << WHEEL_A_PINID)
#define WHEEL_B_GROUP           PIN_GROUP_0
#define WHEEL_B_PINID           1
#define WHEEL_B_MASK            (1UL << WHEEL_B_PINID)
#define WHEEL_SW_GROUP          PIN_GROUP_0
#define WHEEL_SW_PINID          28
#define WHEEL_SW_MASK           (1UL << WHEEL_SW_PINID)
/* POWER & BLE SYSTEM */
#define SWDET_EN_GROUP          PIN_GROUP_0
#define SWDET_EN_PINID          2
#define SWDET_EN_MASK           (1UL << SWDET_EN_PINID)
#if defined(PLAT_V1_SETUP)
    #define SMC_POW_NEN_GROUP   PIN_GROUP_0
    #define SMC_POW_NEN_PINID   3
    #define SMC_POW_NEN_MASK    (1UL << SMC_POW_NEN_PINID)
#elif defined(PLAT_V2_SETUP)
    #define SMC_POW_NEN_GROUP   PIN_GROUP_0
    #define SMC_POW_NEN_PINID   30
    #define SMC_POW_NEN_MASK    (1UL << SMC_POW_NEN_PINID)
#endif
#define BLE_EN_GROUP            PIN_GROUP_0
#define BLE_EN_PINID            13
#define BLE_EN_MASK             (1UL << BLE_EN_PINID)
#define USB_3V3_GROUP           PIN_GROUP_0
#define USB_3V3_PINID           27
#define USB_3V3_MASK            (1UL << USB_3V3_PINID)
#define VOLED_1V2_EN_GROUP      PIN_GROUP_1
#define VOLED_1V2_EN_PINID      22
#define VOLED_1V2_EN_MASK       (1UL << VOLED_1V2_EN_PINID)
#define VOLED_3V3_EN_GROUP      PIN_GROUP_1
#define VOLED_3V3_EN_PINID      23
#define VOLED_3V3_EN_MASK       (1UL << VOLED_3V3_EN_PINID)
#define MCU_AUX_RST_EN_GROUP    PIN_GROUP_0
#define MCU_AUX_RST_EN_PINID    15
#define MCU_AUX_RST_EN_MASK     (1UL << MCU_AUX_RST_EN_PINID)
/* OLED */
#define OLED_MOSI_GROUP         PIN_GROUP_0
#define OLED_MOSI_PINID         4
#define OLED_MOSI_MASK          (1UL << OLED_MOSI_PINID)
#define OLED_MOSI_PMUX_ID       PORT_PMUX_PMUXE_D_Val
#if (OLED_MOSI_PINID % 2) == 1
    #define OLED_MOSI_PMUXREGID PMUXO
#else
    #define OLED_MOSI_PMUXREGID PMUXE
#endif
#define OLED_SCK_GROUP          PIN_GROUP_0
#define OLED_SCK_PINID          5
#define OLED_SCK_MASK           (1UL << OLED_SCK_PINID)
#define OLED_SCK_PMUX_ID        PORT_PMUX_PMUXO_D_Val
#if (OLED_SCK_PINID % 2) == 1
    #define OLED_SCK_PMUXREGID  PMUXO
#else
    #define OLED_SCK_PMUXREGID  PMUXE
#endif
#define OLED_nCS_GROUP          PIN_GROUP_1
#define OLED_nCS_PINID          9
#define OLED_nCS_MASK           (1UL << OLED_nCS_PINID)
#define OLED_CD_GROUP           PIN_GROUP_0
#define OLED_CD_PINID           6
#define OLED_CD_MASK            (1UL << OLED_CD_PINID)
#define OLED_nRESET_GROUP       PIN_GROUP_0
#define OLED_nRESET_PINID       7
#define OLED_nRESET_MASK        (1UL << OLED_nRESET_PINID)
/* DATAFLASH FLASH */
#define DATAFLASH_MOSI_GROUP         PIN_GROUP_0
#define DATAFLASH_MOSI_PINID         8
#define DATAFLASH_MOSI_MASK          (1UL << DATAFLASH_MOSI_PINID)
#define DATAFLASH_MOSI_PMUX_ID       PORT_PMUX_PMUXE_D_Val
#if (DATAFLASH_MOSI_PINID % 2) == 1
    #define DATAFLASH_MOSI_PMUXREGID PMUXO
#else
    #define DATAFLASH_MOSI_PMUXREGID PMUXE
#endif
#define DATAFLASH_MISO_GROUP         PIN_GROUP_0
#define DATAFLASH_MISO_PINID         10
#define DATAFLASH_MISO_MASK          (1UL << DATAFLASH_MISO_PINID)
#define DATAFLASH_MISO_PMUX_ID       PORT_PMUX_PMUXE_D_Val
#if (DATAFLASH_MISO_PINID % 2) == 1
    #define DATAFLASH_MISO_PMUXREGID PMUXO
#else
    #define DATAFLASH_MISO_PMUXREGID PMUXE
#endif
#define DATAFLASH_SCK_GROUP          PIN_GROUP_0
#define DATAFLASH_SCK_PINID          9
#define DATAFLASH_SCK_MASK           (1UL << DATAFLASH_SCK_PINID)
#define DATAFLASH_SCK_PMUX_ID        PORT_PMUX_PMUXO_D_Val
#if (DATAFLASH_SCK_PINID % 2) == 1
    #define DATAFLASH_SCK_PMUXREGID  PMUXO
#else
    #define DATAFLASH_SCK_PMUXREGID  PMUXE
#endif
#define DATAFLASH_nCS_GROUP          PIN_GROUP_0
#define DATAFLASH_nCS_PINID          11
#define DATAFLASH_nCS_MASK           (1UL << DATAFLASH_nCS_PINID)
/* DBFLASH FLASH */
#define DBFLASH_MOSI_GROUP         PIN_GROUP_0
#define DBFLASH_MOSI_PINID         22
#define DBFLASH_MOSI_MASK          (1UL << DBFLASH_MOSI_PINID)
#define DBFLASH_MOSI_PMUX_ID       PORT_PMUX_PMUXE_C_Val
#if (DBFLASH_MOSI_PINID % 2) == 1
    #define DBFLASH_MOSI_PMUXREGID PMUXO
#else
    #define DBFLASH_MOSI_PMUXREGID PMUXE
#endif
#define DBFLASH_MISO_GROUP         PIN_GROUP_0
#define DBFLASH_MISO_PINID         25
#define DBFLASH_MISO_MASK          (1UL << DBFLASH_MISO_PINID)
#define DBFLASH_MISO_PMUX_ID       PORT_PMUX_PMUXE_C_Val
#if (DBFLASH_MISO_PINID % 2) == 1
    #define DBFLASH_MISO_PMUXREGID PMUXO
#else
    #define DBFLASH_MISO_PMUXREGID PMUXE
#endif
#define DBFLASH_SCK_GROUP          PIN_GROUP_0
#define DBFLASH_SCK_PINID          23
#define DBFLASH_SCK_MASK           (1UL << DBFLASH_SCK_PINID)
#define DBFLASH_SCK_PMUX_ID        PORT_PMUX_PMUXO_C_Val
#if (DBFLASH_SCK_PINID % 2) == 1
    #define DBFLASH_SCK_PMUXREGID  PMUXO
#else
    #define DBFLASH_SCK_PMUXREGID  PMUXE
#endif
#define DBFLASH_nCS_GROUP          PIN_GROUP_0
#define DBFLASH_nCS_PINID          24
#define DBFLASH_nCS_MASK           (1UL << DBFLASH_nCS_PINID)
/* ACCELEROMETER */
#define ACC_MOSI_GROUP         PIN_GROUP_0
#define ACC_MOSI_PINID         16
#define ACC_MOSI_MASK          (1UL << ACC_MOSI_PINID)
#define ACC_MOSI_PMUX_ID       PORT_PMUX_PMUXE_C_Val
#if (ACC_MOSI_PINID % 2) == 1
    #define ACC_MOSI_PMUXREGID PMUXO
#else
    #define ACC_MOSI_PMUXREGID PMUXE
#endif
#define ACC_MISO_GROUP         PIN_GROUP_0
#define ACC_MISO_PINID         17
#define ACC_MISO_MASK          (1UL << ACC_MISO_PINID)
#define ACC_MISO_PMUX_ID       PORT_PMUX_PMUXE_C_Val
#if (ACC_MISO_PINID % 2) == 1
    #define ACC_MISO_PMUXREGID PMUXO
#else
    #define ACC_MISO_PMUXREGID PMUXE
#endif
#define ACC_SCK_GROUP          PIN_GROUP_0
#define ACC_SCK_PINID          19
#define ACC_SCK_MASK           (1UL << ACC_SCK_PINID)
#define ACC_SCK_PMUX_ID        PORT_PMUX_PMUXO_C_Val
#if (ACC_SCK_PINID % 2) == 1
    #define ACC_SCK_PMUXREGID  PMUXO
#else
    #define ACC_SCK_PMUXREGID  PMUXE
#endif
#define ACC_nCS_GROUP          PIN_GROUP_0
#define ACC_nCS_PINID          18
#define ACC_nCS_MASK           (1UL << ACC_nCS_PINID)
#define ACC_INT_GROUP          PIN_GROUP_0
#define ACC_INT_PINID          20
#define ACC_INT_MASK           (1UL << ACC_INT_PINID)
/* SMARTCARD */
#define SMC_MOSI_GROUP         PIN_GROUP_1
#define SMC_MOSI_PINID         2
#define SMC_MOSI_MASK          (1UL << SMC_MOSI_PINID)
#define SMC_MOSI_PMUX_ID       PORT_PMUX_PMUXE_D_Val
#if (SMC_MOSI_PINID % 2) == 1
    #define SMC_MOSI_PMUXREGID PMUXO
#else
    #define SMC_MOSI_PMUXREGID PMUXE
#endif
#define SMC_MISO_GROUP         PIN_GROUP_1
#define SMC_MISO_PINID         3
#define SMC_MISO_MASK          (1UL << SMC_MISO_PINID)
#define SMC_MISO_PMUX_ID       PORT_PMUX_PMUXE_D_Val
#if (SMC_MISO_PINID % 2) == 1
    #define SMC_MISO_PMUXREGID PMUXO
#else
    #define SMC_MISO_PMUXREGID PMUXE
#endif
#define SMC_SCK_GROUP          PIN_GROUP_0
#define SMC_SCK_PINID          21
#define SMC_SCK_MASK           (1UL << SMC_SCK_PINID)
#define SMC_SCK_PMUX_ID        PORT_PMUX_PMUXO_C_Val
#if (SMC_SCK_PINID % 2) == 1
    #define SMC_SCK_PMUXREGID  PMUXO
#else
    #define SMC_SCK_PMUXREGID  PMUXE
#endif
#define SMC_RST_GROUP          PIN_GROUP_0
#define SMC_RST_PINID          14
#define SMC_RST_MASK           (1UL << SMC_RST_PINID)
#define SMC_PGM_GROUP          PIN_GROUP_0
#define SMC_PGM_PINID          31
#define SMC_PGM_MASK           (1UL << SMC_PGM_PINID)
#define SMC_DET_GROUP          PIN_GROUP_0
#define SMC_DET_PINID          12
#define SMC_DET_MASK           (1UL << SMC_DET_PINID)

/* Display defines */
#define DEFAULT_FONT_ID         1

#endif /* PLATFORM_DEFINES_H_ */