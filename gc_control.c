#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "usb_debug_only.h"
#include "print.h"

volatile uint16_t int0_count = 0;
volatile uint8_t led_toggle_countdown = 100;
ISR(INT0_vect)
{
  ++int0_count;
  --led_toggle_countdown;
}

#define LED_ON      (PORTD |= (1<<6))
#define LED_OFF     (PORTD &= ~(1<<6))
#define LED_TOGGLE  (PORTD ^= (1<<6))

#define CPU_PRESCALE(n) (CLKPR=0x80, CLKPR=(n))

//led=output, data=input w/o pull-up:
#define CONFIG_DATA_AND_LED (DDRD=(1<<6), PORTD=(0))

#define phex8 phex
#define print_uintN_var(n, x) print_P(PSTR(#x "="));phex##n(x);pchar('\n');
#define print_uint8_var(x) print_uintN_var(8, x);
#define print_uint16_var(x) print_uintN_var(16, x);

// PD0(INT0)=GC_DATA  PD6=LED
int main(void)
{
  CPU_PRESCALE(0);
  CONFIG_DATA_AND_LED;
  LED_OFF;

  usb_init();
  //while(!usb_configured());

  sei();
  EICRA = 0x02; // Falling edge of GC_DATA(INT0) causes interrupt
  EIMSK = 0x01; // Enable INT0

  uint32_t print_interval = 200000;
  while (1) {
    if (led_toggle_countdown == 0) {
      LED_TOGGLE;
      led_toggle_countdown = 100;
    }
    if (--print_interval == 0) {
      print_uint16_var(int0_count);
      print_interval = 200000;
    }
  }
}
