/* Very basic print functions, intended to be used with usb_rawhid.h
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2008 PJRC.COM, LLC
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// Version 1.0: Initial Release

#include <avr/io.h>
#include <avr/pgmspace.h>

#include "print.h"

#include "usb_rawhid_settings.h"

static uint8_t printbuf[RAWHID_TX_SIZE];
static uint8_t printbuf_cursor = 0;

void pchar(char c) {
  if (printbuf_cursor < RAWHID_TX_SIZE)
    printbuf[printbuf_cursor++] = c;
}
void flush_print_buffer(void) {
  pchar(0);
  usb_rawhid_send(printbuf, 100);
  printbuf_cursor = 0;
}

void print_P(const char *s)
{
	char c;

	while (1) {
		c = pgm_read_byte(s++);
		if (!c) break;
		if (c == '\n') pchar('\r');
		pchar(c);
	}
}

void phex4(uint8_t c)
{
  c &= 0x0f;
	pchar(c + ((c < 10) ? '0' : 'A' - 10));
}

void phex8(uint8_t c)
{
	phex4(c >> 4);
	phex4(c & 15);
}

void phex16(uint16_t i)
{
	phex8(i >> 8);
	phex8(i);
}

void phex32(uint32_t i)
{
	phex16(i >> 16);
	phex16(i);
}




