; based on http://www.raphnet.net/electronique/x2wii/index_en.php

.data

.global gc_rx_buf ; defined in C
.global id_status ; defined in C

.text

#include <avr/io.h>

#define IO_SREG    _SFR_IO_ADDR(SREG)
#define IO_PORTC   _SFR_IO_ADDR(PORTC)
#define IO_PIND    _SFR_IO_ADDR(PIND)
#define IO_PORTD   _SFR_IO_ADDR(PORTD)
#define IO_DDRD    _SFR_IO_ADDR(DDRD)
#define IO_EIFR    _SFR_IO_ADDR(EIFR)
#define IO_EIMSK   _SFR_IO_ADDR(EIMSK)
#define IO_GPIOR0  _SFR_IO_ADDR(GPIOR0)
#define IO_GPIOR1  _SFR_IO_ADDR(GPIOR1)
#define IO_GPIOR2  _SFR_IO_ADDR(GPIOR2)

#define GC_DATA_BIT        0
#define DATA_FALL_TIMEOUT  0x10
#define DATA_RAISE_TIMEOUT 0x15

#define zl r30
#define zh r31

; Registers:
;  r1: GCC's zero reg, use as tmp, clr on return
; r16: tmp
; r23: number of bits left to send/receive
; r24: number of bits left to send/receive in current byte


.global INT0_vect
INT0_vect:
	; NOTE: this might be triggered in the middle of an incoming request
	; due to a) booting in middle of one, b) usb_rawhid_recv call (10-20
	; cycles) or c) ongoing USB interrupt handler (may take hundreds of
	; cycles).
	in r1, IO_SREG
	out IO_GPIOR0, r1
	out IO_GPIOR1, r16
	out IO_GPIOR2, r23
	push r24

	ldi zl, lo8(gc_rx_buf)
	ldi zh, hi8(gc_rx_buf)
	ldi r23, 24 ; try to receive 24 bits
	ldi r24,  8 ; 8 bits left in first byte

	rjmp dataSignalFell


readNextBit:
	tst r24
	brne waitFall
	adiw zl, 1 ; move to next byte of buffer
	ldi r24, 8
waitFall:
	ldi r16, DATA_FALL_TIMEOUT
waitFallLoop:
	dec r16
	breq recvTimeout
	in r1, IO_PIND
	lsr r1 ; this assumes that data pin is D0!
	brcs waitFallLoop ; while data=1

	; When we first jump to 'dataSignalFell' (when the interrupt
	; occurs), we have already wasted many cycles:
	;  * We may have been in an USB-related interrupt: 1 - 43 cycles
	;  * 3-6 cycles until jump to interrupt vector
	;  * 6 cycles of interrupt vector prologue
	;  * 5 cycles until control is in dataSignalFell
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
dataSignalFell:
	; Check the pin on the ~50th cycle.

	rcall alignDelay	; waste enough cycles to be aligned
	ld r1, Z
	lsl r1
	in r16, IO_PIND		; read the input port
	andi r16, 0x01		; separate PIND0
	or r1, r16
	st Z, r1		; store the value

	dec r24			; One less bit to receive in current byte...
	dec r23			; One less bit to receive...
	breq sendControllerStatus

	ldi r16, DATA_RAISE_TIMEOUT
waitHigh:
	dec r16
	breq recvTimeout
	in r1, IO_PIND
	lsr r1
	brcc waitHigh		; Still low? Keep looping

	rjmp readNextBit

; 3 + 1 + 7*3-1 + 1 + 4
alignDelay:
	ldi r16, 8
alignDelayLoop:
	dec r16
	brne alignDelayLoop
	ret

recvTimeout:
	; If we had received only 9 bits, the command is getID/getOrigins.
	; We must respond to this if we want the console to detect us.
	; r23 is a count-down from 24 during the receive phase so it should
	; contain a value of 15.

	ldi r16, 15
	cp r16, r23
	brne readError

	ldi zl, lo8(gc_rx_buf)
	ldi zh, hi8(gc_rx_buf)

	; Commands: 0x00 = Get ID, 0x41 = Get origins
	ld r16, Z
	ldi r24, 0x00
	cp r16, r24
	breq sendIdPacket
	ldi r24, 0x41
	cp r16, r24
	breq sendOriginsPacket
	rjmp readError

readError:
	; Count read errors in PORTC
	in r16, IO_PORTC
	inc r16
	out IO_PORTC, r16
	rjmp interruptDone

sendControllerStatus:
	ldi r16, 50
lp_delay_before_status_response:
	dec r16
	brne lp_delay_before_status_response

	ldi zl, lo8(controller_status_buf)
	ldi zh, hi8(controller_status_buf)
	ldi r23, 64
	rcall sendGCData

	rjmp interruptDone

sendIdPacket:
	ldi zl, lo8(id_status)
	ldi zh, hi8(id_status)
	ldi r23, 24
	rcall sendGCData

	rjmp interruptDone

sendOriginsPacket:
	ldi zl, lo8(origins_buf)
	ldi zh, hi8(origins_buf)
	ldi r23, 80
	rcall sendGCData

	rjmp interruptDone

interruptDone:
	pop r24
	in r23, IO_GPIOR2
	in r16, IO_GPIOR1
	in r1,  IO_GPIOR0
	out IO_SREG, r1
	clr r1
	sbi IO_EIFR,0 ; clear INT0 flag
	reti


#define PULL_BUS_LOW sbi IO_DDRD, GC_DATA_BIT
#define RELEASE_BUS  cbi IO_DDRD, GC_DATA_BIT

sendGCData:
	ldi r24, 0 ; bits left to send in current byte

lp_sendGCData_bits:
	tst r24
	breq read_next_byte
	nop
	nop
	rjmp read_next_bit_from_current_byte
read_next_byte:
	ld r1, z+
	ldi r24, 8
read_next_bit_from_current_byte:
	dec r24
	lsl r1
	brcs send1

	nop ; compensate for cycle that would have been used on branch to send1
send0:
	PULL_BUS_LOW
	ldi r16, 18
lp_send0_3.6us:
	dec r16
	brne lp_send0_3.6us
	nop
	nop
	RELEASE_BUS
	ldi r16, 2
lp_send0_1.4us:
	dec r16
	brne lp_send0_1.4us
	nop
	dec r23
	brne lp_sendGCData_bits
	rjmp send_stop_bit

send1:
	PULL_BUS_LOW
	ldi r16, 5
lp_send1_1.2us:
	dec r16
	brne lp_send1_1.2us
	nop
	nop
	RELEASE_BUS
	ldi r16, 15
lp_send1_3.8us:
	dec r16
	brne lp_send1_3.8us
	nop
	dec r23
	brne lp_sendGCData_bits
	rjmp send_stop_bit

send_stop_bit:
	nop
	nop
	nop
	nop
	PULL_BUS_LOW
	ldi r16,5
lp_stop_bit_delay_1.2us:
	dec r16
	brne lp_stop_bit_delay_1.2us
	nop
	nop
	RELEASE_BUS
	ret
