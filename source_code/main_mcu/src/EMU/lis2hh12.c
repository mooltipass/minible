#include "lis2hh12.h"
#include <unistd.h>
#include <fcntl.h>

static int urandom = -1;

BOOL lis2hh12_check_data_received_flag_and_arm_other_transfer(accelerometer_descriptor_t* descriptor_pt, BOOL arm_other_transfer)
{
    (void)arm_other_transfer;
    if(urandom < 0)
        urandom = open("/dev/urandom", O_RDONLY);
    read(urandom, &descriptor_pt->fifo_read.acc_data_array, sizeof(descriptor_pt->fifo_read.acc_data_array));
    return TRUE; 
}

RET_TYPE lis2hh12_check_presence_and_configure(accelerometer_descriptor_t* descriptor_pt){ return RETURN_OK; }
/*
void lis2hh12_send_command(accelerometer_descriptor_t* descriptor_pt, uint8_t* data, uint32_t length){}
void lis2hh12_manual_acc_data_read(accelerometer_descriptor_t* descriptor_pt, acc_data_t* data_pt){}
void lis2hh12_deassert_ncs_and_go_to_sleep(accelerometer_descriptor_t* descriptor_pt){}
void lis2hh12_sleep_exit_and_dma_arm(accelerometer_descriptor_t* descriptor_pt){}
int16_t lis2hh12_get_temperature(accelerometer_descriptor_t* descriptor_pt){}
void lis2hh12_dma_arm(accelerometer_descriptor_t* descriptor_pt){}
void lis2hh12_reset(accelerometer_descriptor_t* descriptor_pt){}
*/
