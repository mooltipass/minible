/*!  \file     logic_encryption.h
*    \brief    Encryption related functions, calling low level stuff
*    Created:  20/03/2019
*    Author:   Mathieu Stephan
*/


#ifndef LOGIC_ENCRYPTION_H_
#define LOGIC_ENCRYPTION_H_

#include "defines.h"

/* Defines */
#define CTR_FLASH_MIN_INCR  64

/* Prototypes */
void logic_encryption_pre_ctr_task(void);
void logic_encryption_init_context(void);



#endif /* LOGIC_ENCRYPTION_H_ */