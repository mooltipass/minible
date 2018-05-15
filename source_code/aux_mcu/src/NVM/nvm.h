/*
 * nvm.h
 *
 * Created: 5/4/2018 9:02:15 PM
 *  Author: Borrego
 */

#ifndef NVM_H_
#define NVM_H_

#include <asf.h>

typedef enum nvm_err{
    NVM_NO_ERR = 0,
    NVM_BUSY,
    NVM_BAD_ADDR,
    NVM_BAD_LEN
}T_nvm_err;

T_nvm_err nvm_write_buffer(uint32_t* dst, uint32_t* src, uint32_t len);

#endif /* NVM_H_ */