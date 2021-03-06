/* dtPulseWidthhLCD.c
 * Demonstration of configuring the MSP430G2553 (20-pin DIP)
 * counter/timer for capture mode to "measure" the width
 * of a signal pulse seen on both TA0.0 and TA0.1 (pins 3 and 4).
 * Timing of this software expects a 1 MHz MCLK on the MSP430.
 * Compile with Code Composer Studio
 * Jeffrey Anderson (Modified from LAB07, Jerry Hamann, S17)
 * EE 4390 Spring 2019
 */

#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>


/* LCD Function DECLARATIIONS *******************************************
 * Declarations for uwLCD functions, driving an HD44780 16x2 LCD in 4-bit
 * parallel mode from port 2 of an MSP430G2553 running at 1MHz MCLK.
 * Using low-level C to initialize and utilize an HD44780 16x2 LCD display
 * via digital I/O port 2.  Expected wiring between MSP430G2553 and display:
 * ============== P2_5  P2_4  P2_3  P2_2  P2_1  P2_0 =======================
 * --------------- D7    D6    D5    D4    EN    RS ------------------------
 * Timing of software expects a 1 MHz MCLK on the MSP430.
 *
 *
* Delay count achieves US as... 10*count+30    */

#define UWDELAY_30US     0
#define UWDELAY_40US     1
#define UWDELAY_100US    7
#define UWDELAY_1000US   97
#define UWDELAY_4100US   407
#define UWDELAY_15000US  1497
#define UWDELAY_0P5S     59997U
#define UWDELAY_LONG     0xFFFF
#define CLEAR_EN_BIT    (P2OUT &= ~BIT1)
#define SET_EN_BIT      (P2OUT |= BIT1)

void    ftoa(float, char *, int);
int     intToStr(int x, char str[], int d);
void    reverse(char *str, int len);
void    init_UART(void);
__interrupt void button(void);                      // is this line needed?


// Global variables to hold capture state information
uint16_t count0, count1;
uint16_t rollOvers0, rollOvers1, rollOvers;
uint16_t theIV;
bool     newValueReady;

// Global constant for display purposes
//char bmsg[]="           m/s^2";

// ISR ************************************************************
// Record the time at TA0.0 interrupt
void __attribute__((interrupt(TIMER0_A0_VECTOR))) Timer0A0ISR(void) {
    rollOvers0=rollOvers;
    count0=TA1CCR0; // Reading TA0CCR0 should clear the interrupt
    P2OUT ^= BIT3;
}

// Record the time at TA0.1 interrupt, and account for rollovers
void __attribute__((interrupt(TIMER0_A1_VECTOR))) Timer0A1ISR(void) {

    theIV=TA1IV;    // This identifies what interrupted.
    switch(theIV) {
        case 2:     // Cause was a TA0.1 interrupt
            count1=TA1CCR1;
            rollOvers1=rollOvers;
            newValueReady=true;

            break;
        case 10:    // Cause was a TAR0 roll-over interrupt
            rollOvers++;
            break;
        default:
            break;
    }
    TA1CTL &= ~CCIFG;  // Clear the interrupt condition

}

void init_CounterTimer(void) {
    // P1DIR &= ~BIT3; // Make P1.3 an input to provide pull-up on SW2.
    // P1REN |=  BIT3; // This is a trick to use the LaunchPad SW2 here.
    // P1OUT |=  BIT3; // Should now have internal pull-up for switch.
    // Note:  we externally wire this pin over to P1.1 and P1.2 to measure.
    P2DIR &= ~(BIT0 | BIT1); // P1.1(pin 3) is TA0.0, P1.2(pin 4) is TA0.1
    P2SEL |=  (BIT0 | BIT1);
    P2SEL2 &= ~(BIT0 | BIT1); // TA0.0 and TA0.1 now appearing at pins 3 and 4.
//    // Configure TA0 for capture on falling edge at TA0.0, rising edge at TA0.1
//    // SMCLK is used for counting 1us resolution (no division down from MCLK).
    TA1CCTL0 = (0x4000 | CAP | CCIE);
    TA1CCTL1 = (0x8000 | CAP | CCIE); // Beware, User's Guide CM1/CM0 don't match!
    TA1CTL   = (TASSEL_2 | MC_2 | TAIE);
//    // Initialize state of counter capture process
    count0=count1=rollOvers0=rollOvers1=rollOvers=0;
    newValueReady=false;
//    __enable_interrupt();
    return;
}

//void __attribute__((interrupt(PORT1_VECTOR))) button(void) {
//   // P1.3 button interrupt
//
//   uint8_t IV = P1IFG;
//
//   switch(IV) {
//       case 8:
//           P1IFG &= ~8;
//           // send at least 10 us pulse
//           P1OUT |= 16;
//           asm("    nop\n \t nop\n \t nop\n \t nop\n \t nop\n \t nop\n \t nop\n \t nop\n \t nop\n \t nop");
//           P1OUT &= ~16;
//           break;
//       default:
//           P1IFG &= ~IV;
//           break;
//   }
//
//}

// void init_CounterTimer(void) {
//    P1DIR &= ~BIT3; // Make P1.3 an input to provide pull-up on SW2.
//    P1REN |=  BIT3; // This is a trick to use the LaunchPad SW2 here.
//    P1OUT |=  BIT3; // Should now have internal pull-up for switch.
//    // Note:  we externally wire this pin over to P1.1 and P1.2 to measure.
//    P1DIR |= (BIT5); // P1.5(pin 7) is TA0.0, P2.6(pin 19) is TA0.1
//    P1SEL2 &=  ~(BIT5);
//    P1SEL |= (BIT5); // TA0.0 and TA0.1 now appearing at pins 7 and 19.
//    P2DIR |= (BIT6);
//    P2SEL2 &= ~(BIT7 | BIT6);
//    P2SEL &= ~BIT7;
//    P2SEL |= BIT6;
//    // Configure TA0 for capture on falling edge at TA0.0, rising edge at TA0.1
//    // SMCLK is used for counting 1us resolution (no division down from MCLK).
//    TA0CCTL0 = (0x4000 | CAP | CCIE);
//    TA0CCTL1 = (0x8000 | CAP | CCIE); // Beware, User's Guide CM1/CM0 don't match!
//    TA0CTL   = (TASSEL_2 | MC_2 | TAIE);
//    // Initialize state of counter capture process
//    count0=count1=rollOvers0=rollOvers1=rollOvers=0;
//    newValueReady=false;
//    __enable_interrupt();
//    return;
// }

void reverse(char *str, int len)
{
    // From geeksforgeeks
    // https://www.geeksforgeeks.org/convert-floating-point-number-string/
    int i=0, j=len-1, temp;
    while (i<j)
    {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++; j--;
    }
}

int intToStr(int x, char str[], int d)
{
    // From geeksforgeeks
    // https://www.geeksforgeeks.org/convert-floating-point-number-string/
    int i = 0;
    while (x)
    {
        str[i++] = (x%10) + '0';
        x = x/10;
    }

    // If number of digits required is more, then
    // add 0s at the beginning
    while (i < d)
        str[i++] = '0';

    reverse(str, i);
    str[i] = '\0';
    return i;
}

void ftoa(float n, char *res, int afterpoint)
{
    // From geeksforgeeks
    // https://www.geeksforgeeks.org/convert-floating-point-number-string/
    // Extract integer part
    int ipart = (int)n;

    // Extract floating part
    float fpart = n - (float)ipart;

    // convert integer part to string
    int i = intToStr(ipart, res, 0);

    // check for display option after point
    if (afterpoint != 0)
    {
        res[i] = '.';  // add dot

        // Get the value of fraction part upto given no.
        // of points after dot. The third parameter is needed
        // to handle cases like 233.007
        fpart = fpart * pow(10, afterpoint);

        intToStr((int)fpart, res + i + 1, afterpoint);
    }
}

void init_UART() {
    // setup uart pins
    asm("    mov.b    #00000110b, &P1SEL");
    asm("    mov.b    #00000110b, &P1SEL2");

    // UART config sequence
    asm("    bis.b   #1, &0x61");

    asm("    mov.b   #0x80, &0x61");
    asm("    mov.b   #104, &0x62");
    asm("    mov.b   #0,   &0x63");
    asm("    mov.b   #2, &0x64");

    asm("    bic.b   #1, &0x61");
}

/*  MAIN FUNCTION *****************************************************/
int main(void) {
    int32_t  longResult;
    uint32_t tmp0, tmp1;
    WDTCTL  = WDTPW | WDTHOLD;
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL  = CALDCO_1MHZ;
    // set falling edge interrupt for P1.3

    float cm;  // distance measurement
    char result[] = {0,0,0,0,0};
    const char cursorHome = 17; // ask Anderson about cursor home
        // command in ASCII format

    // setup timer system
    init_CounterTimer();

    // setup uart
    init_UART();

    // setup button P1.3 for interrupt on falling edge
//    P1IE |= BIT3;
//    P1IES |= BIT3;
    // P1DIR &= ~BIT3; // Make P1.3 an input to provide pull-up on SW2.
    // P1REN |=  BIT3; // This is a trick to use the LaunchPad SW2 here.
    // P1OUT |=  BIT3; // Should now have internal pull-up for switch.

    P2DIR |= BIT3;
    P2SEL &= ~BIT3;
    P2SEL2 &= ~BIT3;

    // configure P1.4 for triggering to sensor
    P1DIR |= BIT4;
    P1SEL &= ~BIT4;
    P1SEL2 &= ~BIT4;

    while(1) {
//        P1OUT ^= BIT4;
//        asm("    nop\n \t nop\n \t nop\n \t nop");
        int j;
        for (j = 1000; j >= 0; --j) {

            if (j == 5){

                P1OUT |= 16;
            }
            else if (j == 0) {
                P1OUT &= ~16;
            }
        }
//            int k = 3200;
//            for (; k >= 0; --k){
////
//                if (k == 0 && newValueReady) {
//
//                    if(rollOvers1>=rollOvers0) {
//                        tmp0=rollOvers0; tmp1=rollOvers1;
//                        longResult=((tmp1*65536L)+count1)-((tmp0*65536L)+count0);
//
//                        if(longResult>0) {
//
//                            // do calculations
//                            cm = longResult / 58.0;
//                            ftoa(cm, result, 1);
//
//                            // send data to RX
//                            /* must send ascii device ctrl 1 char first
//                             to cursor home RX
//                             */
//                            unsigned int i;
//                            for (i = 0; i < 6; i++) {
//                                while (UCA0TXIFG & IFG2 == 0) {
//                                    if (i == 0)
//                                        UCA0TXBUF = cursorHome;
//                                    else
//                                        UCA0TXBUF = result[i-1];
//                                }
//                            }
//                        }
//                    }
//                    newValueReady=false;
//                }
//            }
//
//        }

}


    return 0;
}
