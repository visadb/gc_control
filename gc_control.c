#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "usb_debug_only.h"
#include "print.h"

#define ENABLE_INT0  (EIMSK |=  (1<<0))
#define DISABLE_INT0 (EIMSK &= ~(1<<0))
#define DISABLE_INT0_ASM "cbi 0x1d,0\n\t"
#define CLEAR_INT0_FLAG (EIFR |= (1<<0))

volatile uint8_t data_from_interrupt;
volatile uint8_t data_from_interrupt_available;

// Buffers. One byte per bit. Bit is considered 1 if any bit of byte is set
volatile uint8_t gc_rx_buf[24];
uint8_t id_status[24] = {0,0,0,0,1,0,0,1, 0,0,0,0,0,0,0,0, 0,0,1,0,0,0,0,0};

#define LED_ON      (PORTD |=  (1<<6))
#define LED_OFF     (PORTD &= ~(1<<6))
#define LED_TOGGLE  (PORTD ^=  (1<<6))

#define CPU_PRESCALE(n) (CLKPR=0x80, CLKPR=(n))

//led=output, data=input w/o pull-up:
#define CONFIG_DATA_AND_LED (DDRD=(1<<6), PORTD=0x00)

// PD0(INT0)=GC_DATA  PD6=LED
int main(void)
{
  CPU_PRESCALE(0);
  CONFIG_DATA_AND_LED;
  LED_OFF;
  data_from_interrupt_available = 0;

  usb_init();
  while(!usb_configured());

  EICRA = 0x02; // Falling edge of GC_DATA(INT0) causes interrupt
  CLEAR_INT0_FLAG;
  ENABLE_INT0;
  sei();

  while(1) {
    if (data_from_interrupt_available) {
      cli();
      LED_TOGGLE;
      print("data:"); phex8(data_from_interrupt); pchar('\n');
      usb_debug_flush_output();
      data_from_interrupt_available = 0;
      data_from_interrupt = 0;
      sei();
      _delay_ms(50);
    }
  }
}
