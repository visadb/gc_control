#ifndef print_h__
#define print_h__

#include <avr/pgmspace.h>
#include "usb_rawhid.h"

// this macro allows you to write print("some text") and
// the string is automatically placed into flash memory :)
#define print(s) print_P(PSTR(s))

void pchar(char c);
void print_P(const char *s);
void phex4(uint8_t c);
void phex8(uint8_t c);
void phex16(uint16_t i);
void phex32(uint32_t i);
void flush_print_buffer(void);

#endif
