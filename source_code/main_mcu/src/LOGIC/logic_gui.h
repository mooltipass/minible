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
/*!  \file     logic_gui.h
*    \brief    General logic for GUI
*    Created:  11/05/2019
*    Author:   Mathieu Stephan
*/ 


#ifndef LOGIC_GUI_H_
#define LOGIC_GUI_H_

#include "nodemgmt.h"
#include "defines.h"


/* Prototypes */
void logic_gui_display_login_password_TOTP(child_cred_node_t* child_node);
void logic_gui_disable_bluetooth(void);
void logic_gui_enable_bluetooth(void);
void logic_gui_clone_card(void);
void logic_gui_change_pin(void);
void logic_gui_erase_user(void);

/* Defines */
#define LOGIC_GUI_NUM_LINES (3)           // Number of lines to display for credentials
#define LOGIC_GUI_TOTP_STR_LEN (30)       // Length of TOTP string generated
#define LOGIC_GUI_TOTP_MAX_TIMESTEP (100) // Maximum number for the timestep
#endif /* LOGIC_GUI_H_ */
