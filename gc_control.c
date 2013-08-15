#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "usb_rawhid.h"
#include "usb_rawhid_settings.h"
#include "print.h"

#define ENABLE_INT0  (EIMSK |=  (1<<0))
#define DISABLE_INT0 (EIMSK &= ~(1<<0))
#define DISABLE_INT0_ASM "cbi 0x1d,0\n\t"
#define CLEAR_INT0_FLAG (EIFR |= (1<<0))

// Receive buffer. One byte per bit. Bit is considered 1 if any bit of byte is set
volatile uint8_t gc_rx_buf[24];

// Send buffers
uint8_t const id_status[] = {0x09, 0x00, 0x20};
uint8_t const origins_buf[] = {
  0x00, 0x80, // No buttons pressed
  0x80, 0x80, // Joystick neutral
  0x80, 0x80, // C-stick neutral
  0x00, 0x00, // Shoulder L & R not pressed
  0x02, 0x02  // unknown
};
uint8_t controller_status_buf[] = {
  0x00, // 0 0 0 START Y X B A
  0x80, // 1 LSHOULDER RSHOULDER Z UP DOWN RIGHT LEFT
  0x80, 0x80, // Joystick X & Y
  0x80, 0x80, // C-stick X & Y
  0x00, 0x00  // Shoulder button positions
};

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

  usb_init();
  while(!usb_configured());

  EICRA = 0x02; // Falling edge of GC_DATA(INT0) causes interrupt
  CLEAR_INT0_FLAG;
  ENABLE_INT0;
  sei();

  while(1) {
    uint8_t recv_bytes;
    uint8_t recv_buf[RAWHID_RX_SIZE];
    recv_bytes = usb_rawhid_recv(recv_buf, 0);
    if (recv_bytes > 0) {
      cli();
      LED_TOGGLE;
      controller_status_buf[0] = recv_buf[0];
      controller_status_buf[1] = recv_buf[1];
      sei();
    }
  }
}
