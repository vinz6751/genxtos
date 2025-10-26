#ifndef VICKY2_TXT_A_LOGGER
#define VICKY2_TXT_A_LOGGER

#include <stdint.h>

#if defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)

void channel_A_logger_init(void);
void channel_A_cls(void);
void channel_A_write(const char *c);
void channel_A_set_fg_color(uint8_t c);
void channel_A_set_bg_color(uint8_t c);
void channel_A_set_cursor_pos(uint16_t x, uint16_t y);

#endif

#endif
