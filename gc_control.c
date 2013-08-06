#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "usb_debug_only.h"
#include "print.h"

#define NOP0 ""
#define NOP1 "nop\n\t"
#define NOP2  NOP1  NOP1
#define NOP3  NOP2  NOP1
#define NOP4  NOP3  NOP1
#define NOP5  NOP4  NOP1
#define NOP6  NOP5  NOP1
#define NOP7  NOP6  NOP1
#define NOP8  NOP7  NOP1
#define NOP9  NOP8  NOP1
#define NOP10 NOP9  NOP1
#define NOP11 NOP10 NOP1
#define NOP12 NOP11 NOP1
#define NOP13 NOP12 NOP1
#define NOP14 NOP13 NOP1
#define NOP15 NOP14 NOP1
#define NOP16 NOP8 NOP8
#define NOP32 NOP16 NOP16
#define NOP64 NOP32 NOP32

#define ENABLE_INT0  (EIMSK |=  (1<<0))
#define DISABLE_INT0 (EIMSK &= ~(1<<0))
#define DISABLE_INT0_ASM "cbi 0x1d,0\n\t"
#define CLEAR_INT0_FLAG (EIFR |= (1<<0))

volatile uint8_t data_available;
volatile register uint8_t data_at_int asm("r16");

#define LED_ON      (PORTD |=  (1<<6))
#define LED_OFF     (PORTD &= ~(1<<6))
#define LED_TOGGLE  (PORTD ^=  (1<<6))

#define CPU_PRESCALE(n) (CLKPR=0x80, CLKPR=(n))

//led=output, data=input w/o pull-up:
#define CONFIG_DATA_AND_LED (DDRD=(1<<6), PORTD=(0))

#define print_uintN_var(n, x) print_P(PSTR(#x "="));phex##n(x);pchar('\n');
#define print_uint8_var(x) print_uintN_var(8, x);
#define print_uint16_var(x) print_uintN_var(16, x);


// PD0(INT0)=GC_DATA  PD6=LED
int main(void)
{
  CPU_PRESCALE(0);
  CONFIG_DATA_AND_LED;
  LED_OFF;
  data_at_int = 0;
  data_available = 0;

  usb_init();
  while(!usb_configured());

  EICRA = 0x02; // Falling edge of GC_DATA(INT0) causes interrupt
  CLEAR_INT0_FLAG;
  ENABLE_INT0;

  //volatile uint16_t delay=0;
  while(1) {
    if (data_available) {
      phex4(data_at_int);
      usb_debug_flush_output();
      _delay_ms(10);
      data_available = 0;
      CLEAR_INT0_FLAG;
      ENABLE_INT0;
    }
  }
}
