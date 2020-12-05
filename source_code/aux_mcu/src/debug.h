/*
 * debug.h
 *
 * Created: 25/05/2019 22:53:26
 *  Author: limpkin
 */ 


#ifndef DEBUG_H_
#define DEBUG_H_

void debug_tx_band_send(uint16_t frequency_index, uint16_t payload_type, uint16_t payload_length);
void debug_dtm_rx(uint16_t frequency_index);
void debug_tx_stop_continuous_tone(void);
void debug_init_trace_buffer(void);

#endif /* DEBUG_H_ */