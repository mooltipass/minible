/*
 * debug.h
 *
 * Created: 25/05/2019 22:53:26
 *  Author: limpkin
 */ 


#ifndef DEBUG_H_
#define DEBUG_H_

void debug_tx_band_send(uint16_t frequency_index, uint16_t payload_type, uint16_t payload_length, BOOL continuous_tone);
void debug_tx_stop_continuous_tone(void);

#endif /* DEBUG_H_ */