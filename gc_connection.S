; based on http://www.raphnet.net/electronique/x2wii/index_en.php

.data

.global gc_rx_buf ; defined in C
.global id_status ; defined in C

.text

#include <avr/io.h>

#define IO_SREG		_SFR_IO_ADDR(SREG)
#define IO_PIND		_SFR_IO_ADDR(PIND)
#define IO_DDRD		_SFR_IO_ADDR(DDRD)
#define IO_EIFR		_SFR_IO_ADDR(EIFR)
#define IO_EIMSK	_SFR_IO_ADDR(EIMSK)
#define IO_GPIOR0	_SFR_IO_ADDR(GPIOR0)
#define IO_GPIOR1	_SFR_IO_ADDR(GPIOR1)
#define IO_GPIOR2	_SFR_IO_ADDR(GPIOR2)

#define GC_DATA_MASK		0x01
#define GC_DATA_BIT		0
#define DATA_FALL_TIMEOUT	0x10
#define DATA_RAISE_TIMEOUT	0x15

#define zl r30
#define zh r31

; Registers:
;  r1: GCC's zero reg, use as tmp, clr on return
; r16: tmp
; r23: number of bits left to send/receive


.global INT0_vect
INT0_vect:
	in r1, IO_SREG
	out IO_GPIOR0, r1
	out IO_GPIOR1, r16
	out IO_GPIOR2, r23

	ldi zl, lo8(gc_rx_buf)
	ldi zh, hi8(gc_rx_buf)
	ldi r23, 24 ; try to receive 24 bits

	rjmp dataSignalFell

interruptDone:
	ldi r16, 1
	sts data_from_interrupt_available, r16
	in r23, IO_GPIOR2
	in r16, IO_GPIOR1
	in r1,  IO_GPIOR0
	out IO_SREG, r1
	clr r1
	; cbi IO_EIMSK,0 ; disable INT0
	sbi IO_EIFR,0 ; clear INT0 flag
	reti


readNextBit:
	ldi r16, DATA_FALL_TIMEOUT
waitFall:
	dec r16
	breq recvTimeout
	in r1, IO_PIND
	lsr r1
	brcs waitFall ; while data=1

	; When we first jump to 'dataSignalFell' (when the interrupt
	; occurs), we have already wasted many cycles. Those
	; delays are there to compensate and make sure we
	; always sample the data line where we want.
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
	; I check the pin on the 50th cycle.

	rcall alignDelay	; waste enough cycles to be aligned on cycle 32
	in r16, IO_PIND		; read the input port
	andi r16, GC_DATA_MASK	; isolate the data bit
	st z+, r16		; store the value

	dec r23			; One less bit to receive...
	breq gotCommand

	ldi r16, DATA_RAISE_TIMEOUT
waitHigh:
	dec r16
	breq recvTimeout
	in r1, IO_PIND
	lsr r1
	brcc waitHigh		; Still low? Keep looping

	rjmp readNextBit

; 3 + 1 + 9*3-1 + 1 + 4 = 48
alignDelay:
	ldi r16, 9
lp634534:
	dec r16
	brne lp634534
	ret

recvTimeout:
	;  If we had received only 9 bits, the command is getID/getOrigins.
	; We _must_ respond to this if we want the console to detect us.
	;
	; r23 is a count-down from 24 during the receive phase so it should
	; contain a value of 15..
	ldi r16, 15
	cp r16, r23
	brne readError

	ldi zl, lo8(gc_rx_buf)
	ldi zh, hi8(gc_rx_buf)

	; Commands: 0x00 = Get ID + Status, 0x40 = Get origins?
	adiw zl, 1
	ld r16, z
	tst r16
	breq not_40
	rjmp sendOriginsPacket
not_40: ; probably 0x00
	rjmp sendIdPacket

readError:
	ldi r16, 0x55
	sts data_from_interrupt, r16
	rjmp interruptDone

gotCommand:
	ldi r16, 0x40
	sts data_from_interrupt, r16
	rjmp interruptDone

sendOriginsPacket:
	ldi r16, 0x41
	sts data_from_interrupt, r16
	rjmp interruptDone

sendIdPacket:
	ldi r16, 0x1d
	sts data_from_interrupt, r16

	ldi zl, lo8(id_status)
	ldi zh, hi8(id_status)
	ldi r23, 24
	rcall sendGCData

	rjmp interruptDone



#define PULL_BUS_LOW sbi IO_DDRD, GC_DATA_BIT
#define RELEASE_BUS  cbi IO_DDRD, GC_DATA_BIT

sendGCData:

lp_sendGCData_bits:
	ld r16, z+
	tst r16
	brne send1
	nop
send0:
	PULL_BUS_LOW
	ldi r16, 18
lp_send0_3.6us:
	dec r16
	brne lp_send0_3.6us
	nop
	nop

	RELEASE_BUS
	ldi r16, 3
lp_send0_1.4us:
	dec r16
	brne lp_send0_1.4us
	nop
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
	ldi r16, 17
lp_send1_3.8us:
	dec r16
	brne lp_send1_3.8us

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
lp_stop_bit_delay:
	dec r16
	brne lp_stop_bit_delay
	nop
	nop
	RELEASE_BUS
	ret
