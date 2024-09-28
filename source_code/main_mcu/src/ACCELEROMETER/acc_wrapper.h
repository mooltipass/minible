/* 
 * This file is part of the Mooltipass Project (https://github.com/mooltipass).
 * Copyright (c) 2024 Stephan Mathieu
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
/*!  \file     acc_wrapper.h
*    \brief    Wrapper functions for accelerometer
*    Created:  28/09/2024
*    Author:   Mathieu Stephan
*/


#ifndef ACC_WRAPPER_H_
#define ACC_WRAPPER_H_

#include "platform_defines.h"

/* Defines */

/* Function defines */
#ifndef MINIBLE_V2
    #include "lis2hh12.h"
    
    #define acc_check_data_received_flag_and_arm_other_transfer     lis2hh12_check_data_received_flag_and_arm_other_transfer
    #define acc_check_presence_and_configure                        lis2hh12_check_presence_and_configure
    #define acc_deassert_ncs_and_go_to_sleep                        lis2hh12_deassert_ncs_and_go_to_sleep
    #define acc_sleep_exit_and_dma_arm                              lis2hh12_sleep_exit_and_dma_arm
    #define acc_manual_acc_data_read                                lis2hh12_manual_acc_data_read
    #define acc_get_temperature                                     lis2hh12_get_temperature
    #define acc_send_command                                        lis2hh12_send_command
    #define acc_dma_arm                                             lis2hh12_dma_arm
    #define acc_reset                                               lis2hh12_reset
#else
    #include "lis2dh12.h"
    
    #define acc_check_data_received_flag_and_arm_other_transfer     lis2dh12_check_data_received_flag_and_arm_other_transfer
    #define acc_check_presence_and_configure                        lis2dh12_check_presence_and_configure
    #define acc_deassert_ncs_and_go_to_sleep                        lis2dh12_deassert_ncs_and_go_to_sleep
    #define acc_sleep_exit_and_dma_arm                              lis2dh12_sleep_exit_and_dma_arm
    #define acc_manual_acc_data_read                                lis2dh12_manual_acc_data_read
    #define acc_get_temperature                                     lis2dh12_get_temperature
    #define acc_send_command                                        lis2dh12_send_command
    #define acc_dma_arm                                             lis2dh12_dma_arm
    #define acc_reset                                               lis2dh12_reset    
#endif





#endif /* ACC_WRAPPER_H_ */