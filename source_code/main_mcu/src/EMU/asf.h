#pragma once
#include <inttypes.h>

typedef struct {} Sercom;
typedef struct {} RTC_MODE2_CLOCK_Type;

extern Sercom sercom_array[6];

#define SERCOM0 (&sercom_array[0])
#define SERCOM1 (&sercom_array[1])
#define SERCOM2 (&sercom_array[2])
#define SERCOM3 (&sercom_array[3])
#define SERCOM4 (&sercom_array[4])
#define SERCOM5 (&sercom_array[5])
#define NVMCTRL_ROW_SIZE (256)

static inline void cpu_irq_enter_critical(void) {}
static inline void cpu_irq_leave_critical(void) {}

static inline uint16_t swap16(uint16_t v) {
	return (v<<8)|(v>>8);
}

extern struct emu_port_t {
	struct {
		struct {
			uint32_t reg;
		} OUTCLR, OUTSET;
	} Group[2];
} *PORT;
