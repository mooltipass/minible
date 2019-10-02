#pragma once
#include <inttypes.h>

typedef struct {} Sercom;
typedef struct {} RTC_MODE2_CLOCK_Type;

#define SERCOM0 (0)
#define SERCOM1 (0)
#define SERCOM2 (0)
#define SERCOM3 (0)
#define SERCOM4 (0)
#define SERCOM5 (0)
#define FLASH_ADDR (0)
#define FLASH_SIZE (1024*1024)
#define NVMCTRL_ROW_SIZE (256)

void cpu_irq_enter_critical(void);
void cpu_irq_leave_critical(void);

static inline uint16_t swap16(uint16_t v) {
	return (v<<8)|(v>>8);
}

extern struct {
	struct {
		struct {
			uint32_t reg;
		} OUTCLR, OUTSET;
	} Group[1];
} *PORT;
