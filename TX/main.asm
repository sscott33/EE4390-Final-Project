;-------------------------------------------------------------------------------
;                             UART TRANSMITTER
; This program allows you to input data (ASCII characters) in binary via 2 push
; button switches. Push the "0" and "1" buttons quickly. There is only a very
; short delay built in to accomodate for switch bounce. Long pushes will cause
; multiple unintentional "0"s and "1"s to be inputted. The individual LEDs in the
; RGB LED are used as indicators (P2.1 RED = a "1" was entered)(P2.3 GREEN = a "0"
; was entered)(P2.5 BLUE = 8 Bits have been entered and are ready to send).
; Pushing the "SEND" button transmits the data to the RECEIVER going out on pin
; P1.2 labeled "TXD" on the LaunchPad. At this point the BLUE LED goes off and
; the TRANSMITTER is ready to have another 8 Bits of data inputted.
;
;     James Kretzschmar
;     University of Wyoming
;     5 January 2019
;
;-------------------------------------------------------------------------------
            .cdecls C,LIST,"msp430.h"       ; Include device header file
;-------------------------------------------------------------------------------
            .def    RESET                   ; Export program entry-point to
                                            ; make it known to linker.
;-------------------------------------------------------------------------------
            .text                           ; Assemble into program memory.
            .retain                         ; Override ELF conditional linking
                                            ; and retain current section.
            .retainrefs                     ; And retain any sections that have
                                            ; references to current section.
;-------------------------------------------------------------------------------
RESET       mov.w   #__STACK_END,SP         ; Initialize stackpointer
StopWDT     mov.w   #WDTPW|WDTHOLD,&WDTCTL  ; Stop watchdog timer
;-------------------------------------------------------------------------------
;                            SET UP CLOCK INPUTS AND OUTPUTS
;-------------------------------------------------------------------------------
     mov.b   &CALBC1_1MHZ, &BCSCTL1     ; Calibrated 1 MHz clock selected
     mov.b   &CALDCO_1MHZ, &DCOCTL      ; Calibrated 1 MHz clock selected

     mov.b   #01110100b, &P1DIR         ; P1.2 will be TX output
                                        ; Selected by P1SEL and P1SEL2
                                        ; P1.3, P1.4, P1.5 are pushbutton inputs
;     mov.b   #00000000b, &P1REN         ; Enables Pull-up/Pull-down resistors
                                        ; on P1.3, P1.4 and P1.5
;     mov.b   #00111000b, &P1OUT         ; Selects the Pull-up resistors for
                                        ; P1.3, P1.4 and P1.5

     mov.b   #11111111b, &P2DIR         ; All pins are outputs
                                        ; Using the RGB LED
                                        ; P2.3 (GREEN) a "0" was entered
                                        ; P2.1 (RED) a "1" was entered
                                        ; P2.5 (BLUE) 8 Bits have been entered and
                                        ; are ready to "SEND"
;     bic.b   #00101010b, &P2OUT         ; Makes sure LEDs are off
;---------------------------------------------------------------------------------
;                             SET UP UART TRANSMITTER
;---------------------------------------------------------------------------------
     mov.b   #00000110b, &P1SEL         ; UART RX and TX pins
     mov.b   #00000110b, &P1SEL2        ; UART RX and TX pins
;----------------------------------------------------------------------------------
;                        Register UCA0CTL0 is all "0"s
;   (BIT 7 = 0 Parity disabled)(BIT 6 = 0 Do not need to select a parity)
;   (BIT 5 = 0 LSB sent out first)(BIT 4 = 0 8 Bit data mode)
;   (BIT 3 = 0 One stop Bit)(BIT 1&2 = 0 UART Mode)(Bit 0 = 0 Asynchronous mode)
;----------------------------------------------------------------------------------

     bis.b   #UCSWRST, &UCA0CTL1        ; Reset UART ... ON
                                        ; Reset has to be ON to configure parameters
     mov.b   #UCSSEL_2, &UCA0CTL1       ; SMCLK selected as the clock source
     mov.b   #104, &UCA0BR0             ; 9600 Baud selected (See Table 15-4, p.424)
     mov.b   #0,   &UCA0BR1             ; 9600 Baud rate selected
     mov.b   #UCBRS0, &UCA0MCTL         ; (See Table 15-2 p. 420) and p. 431

     bic.b   #UCSWRST, &UCA0CTL1        ; Reset  UART ... OFF
                                        ; Reset has to be OFF for UART to operate
;-----------------------------------------------------------------------------------
;     THIS SECTION INPUTS 8 BITS OF DATA INTO R10 BY ENTERING "1"s AND "0"s
;     BY PUSHBUTTONS ON P1.4 AND P1.5   EACH TIME A BUTTON IS PUSHED A LED
;     WILL COME ON FOR 65 MILLISECONDS INDICATING A BUTTON WAS PUSHED ... THEN
;     THE LED WILL GO OFF FOR 65 MILLISCEONDS BEFORE THE PROGRAM CONTINUES. WHEN
;     8 BITS HAVE BEEN ENTERED THE BLUE LED COMES ON AND THE 8 BITS ARE READY TO
;     BE SENT OUT ON P1.2
;-----------------------------------------------------------------------------------
;                THE INPUTTED "1"s AND "0"s ARE STORED IN R10
;-----------------------------------------------------------------------------------
RELOAD_R10:
	bis.b #BIT4, &P1OUT
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	bic.b #BIT4, &P1OUT

	nop
	nop

	mov #0, R6
	mov #0, R7

LOOP_PW:		; loop while echo is high
	bit #0xFFFF, R6
	jz NO_ROLLOVER
	inc R7

NO_ROLLOVER:
	inc R6

	bit.b #BIT3, P1IN
	jnz LOOP_PW

;------------------------------------------------------------------------------------
;   IF THE "SEND" BUTTON WAS PUSHED THE PROGRAM CONTINUES ON
;------------------------------------------------------------------------------------
     bic.b   #00100000b, &P2OUT  ; BLUE LED goes "OFF"
;------------------------------------------------------------------------------------
;    IFG2 is the Interrupt Flag Register 2 and controls the flow of data
;    within the UART module for both the Transmitter and the Receiver. On the
;    Transmitter side if the UCA0TXIFG Bit is not set (ie. = 0) then the transmit
;    buffer still has data in it and is not ready to accept new data. If UCA0TXIFG
;    is set (ie. = 1) then the transmit buffer is empty and ready to accept new data
;    to be sent out on P1.2.
;-------------------------------------------------------------------------------------
;	 mov #1, R12

CHECK_STATUS_OF_TRANSMIT_BUFFER:

     bit.b   #UCA0TXIFG, &IFG2               ; Test the UCA0TXIFG flag
     jz      CHECK_STATUS_OF_TRANSMIT_BUFFER ; If "0" keep checking
;------------------------------------------------------------------------------------
;                      IF "1" THE PROGRAM CONTINUES ON
;------------------------------------------------------------------------------------
;     bit #0, R12
;     jeq LSW
     mov     R7, &UCA0TXBUF   ; R7 moved into the Transmit Buffer and sent out
;     dec R12
;     jmp CHECK_STATUS_OF_TRANSMIT_BUFFER

;LSW:
;	 mov R6, &UCA0TXBUF
;	 bis.b #BIT5, &P1OUT
;     nop
;     nop
;     nop
;     bic.b #BIT5, &P1OUT




	 mov #0xFFFF, R12
;;;

DELAY:
	 dec R12
	 jnz DELAY
     jmp     RELOAD_R10        ; Goto the top and get new sensor reading
;-------------------------------------------------------------------------------
; Stack Pointer definition
;-------------------------------------------------------------------------------
            .global __STACK_END
            .sect   .stack
;-------------------------------------------------------------------------------
; Interrupt Vectors
;-------------------------------------------------------------------------------
            .sect   ".reset"                ; MSP430 RESET Vector
            .short  RESET
            
