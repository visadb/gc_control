#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "usb_debug_only.h"
#include "print.h"


#if defined(__AVR_ATmega32U4__) || defined(__AVR_AT90USB1286__)
 // Teensy 2.0: LED is active high
 #define LED_ON		(PORTD |= (1<<6))
 #define LED_OFF		(PORTD &= ~(1<<6))
#else
 // Teensy 1.0: LED is active low
 #define LED_ON	(PORTD &= ~(1<<6))
 #define LED_OFF	(PORTD |= (1<<6))
#endif

#define LED_CONFIG	(DDRD |= (1<<6))
#define CPU_PRESCALE(n)	(CLKPR = 0x80, CLKPR = (n))

int main(void)
{
	// set for 16 MHz clock, and make sure the LED is off
	CPU_PRESCALE(0);
	LED_CONFIG;
	LED_OFF;

	// initialize the USB, but don't want for the host to
	// configure.
	usb_init();

  uint8_t led_on = 0;
	// blink morse code messages!
	while (1) {
		print("Hello from Teensy++\n");
    if (led_on)
      LED_ON;
    else
      LED_OFF;
    led_on = !led_on;
		_delay_ms(1000);
	}
}

