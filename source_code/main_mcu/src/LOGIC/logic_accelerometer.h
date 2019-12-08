/* 
 * This file is part of the Mooltipass Project (https://github.com/mooltipass).
 * Copyright (c) 2019 Stephan Mathieu
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
/*!  \file     logic_accelerometer.h
*    \brief    Accelerometer logic
*    Created:  08/12/2019
*    Author:   Mathieu Stephan
*/


#ifndef LOGIC_ACCELEROMETER_H_
#define LOGIC_ACCELEROMETER_H_

#include "platform_defines.h"
#include "defines.h"

/* Defines */
// After how many z samples we compute the z axis average
#define ACC_Z_AVG_NB_SAMPLES        256
// Minimum sum of the y access to reverse the display (accumulated over ACC_Z_AVG_NB_SAMPLES)
#define ACC_Y_TOTAL_NREVERSE        58*ACC_Z_AVG_NB_SAMPLES
// Same thing, but the other way around ;)
#define ACC_Y_TOTAL_REVERSE         -58*ACC_Z_AVG_NB_SAMPLES
// The maximum sum of the difference between samples & avg to run the detection algo
#define ACC_Z_MAX_AVG_SUM_DIFF      2*ACC_Z_AVG_NB_SAMPLES
// The minimum sum of the difference between samples & avg to notify there's movement
#define ACC_Z_MOVEMENT_AVG_SUM_DIFF ACC_Z_AVG_NB_SAMPLES*2
// Nb samples to timeout a second knock detection
#define ACC_Z_SECOND_KNOCK_MAX_NBS  300
// Nb samples of silence for a second knock detection
#define ACC_Z_SECOND_KNOCK_MIN_NBS  40
// Nb samples to wait before retriggering another detection
#define ACC_Z_KNOCK_REARM_WAIT      400
// Maximum width of a knock
#define ACC_Z_MAX_KNOCK_PULSE_WIDTH 20

/* Prototypes */
acc_detection_te logic_accelerometer_scan_for_action_in_acc_read(void);
acc_detection_te logic_accelerometer_routine(void);


#endif /* LOGIC_ACCELEROMETER_H_ */