.text

#include <avr/io.h>

#define IO_SREG	_SFR_IO_ADDR(SREG)
#define IO_PIND	_SFR_IO_ADDR(PIND)

.global INT0_vect

INT0_vect:
  in r0, IO_SREG
  in r16, IO_PIND
  andi r16, 0x01
  ldi r24, 1
  sts data_available, r24
  cbi 0x1d,0
  out IO_SREG, r0
  reti
