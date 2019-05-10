#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>


void    ftoa(float, char *, int);
int     intToStr(int x, char str[], int d);
void    reverse(char *str, int len);
void    init_UART(void);
void    delay(void);
//char[]  intToChars(int, int);
//
//char[]  intToChars(int x, int len) {
//    //    while (x)
//    //    {
//    //        str[i++] = (x%10) + '0';
//    //        x = x/10;
//    //    }
//}

void delay(){
    int k;
    for (k=1000; k >= 0; --k){
    }
}

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
//    str[i] = '\0';
    return i;
}
//
//void ftoa(float n, char *res, int afterpoint)
//{
//    // From geeksforgeeks
//    // https://www.geeksforgeeks.org/convert-floating-point-number-string/
//    // Extract integer part
//    int ipart = (int)n;
//
//    // Extract floating part
//    float fpart = n - (float)ipart;
//
//    // convert integer part to string
//    int i = intToStr(ipart, res, 0);
//
//    // check for display option after point
//    if (afterpoint != 0)
//    {
//        res[i] = '.';  // add dot
//
//        // Get the value of fraction part upto given no.
//        // of points after dot. The third parameter is needed
//        // to handle cases like 233.007
//        fpart = fpart * pow(10, afterpoint);
//
//        intToStr((int)fpart, res + i + 1, afterpoint);
//    }
//}

void init_UART() {
    // setup uart pins
//    P1SEL |= (BIT2 | BIT1);
    asm("    mov.b    #00000110b, &P1SEL");
//    P1SEL2 |= (BIT2 | BIT1);
    asm("    mov.b    #00000110b, &P1SEL2");

    // UART config sequence
//    UCA0CTL1 |= UCSWRST;
    asm("    bis.b   #1, &0x61");

//    UCA0CTL1 = UCSSEL_2;
    asm("    mov.b   #0x80, &0x61");
//    UCA0BR0 = 104;
    asm("    mov.b   #104, &0x62");
//    UCA0BR1 = 0;
    asm("    mov.b   #0,   &0x63");
//    UCA0MCTL = UCBRS0;
    asm("    mov.b   #2, &0x64");

//    UCA0CTL1 &= ~UCA0CTL1;
    asm("    bic.b   #1, &0x61");
}


/**
 * main.c
 */
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	BCSCTL1 = CALBC1_1MHZ;
	DCOCTL  = CALDCO_1MHZ;
	uint32_t longUS;
	const char cursorHome = 17;
	const char units[] = " cm";
	char result[3];// = {0,0,0};
	int cm;

	init_UART();
	int k;

	P1DIR  |=  (BIT4 | BIT2);
	P1SEL  &= ~BIT4;
	P1SEL2 &= ~BIT4;

	P2DIR |= BIT5;
	P2OUT = 0;



    while(1) {
        int j;  // j = 1000
        for (j = 1000; j >= 0; --j) {

            if (j == 2){

                P1OUT |= 16;
            }
            else if (j == 0) {
                P1OUT &= ~16;
            }
        }

        //  do we need delay between trigger low and echo high?
        //  Yes. Yes we do. A big one.

        //  big delay
        while (!(P1IN & 8)) {}



        // count microseconds of echo
        longUS = 0;
//        P2OUT |= BIT5;
        while (P1IN & 8) {
            ++longUS;
//            P2OUT &= ~BIT5;
        }

        //   do calculations
        cm = (longUS * 10) * 0.034 / 2;
            // 8us to check loop and 3us for ++var
            // * 11us to compensate for loop delay
            // / 58 to convert to cm

//        ftoa(cm, result, 0);
        intToStr(cm, result, 3);

        // send data to RX
        /* must send ascii device ctrl 1 char first
           to cursor home RX
        */

        unsigned int i;
        for (i = 0; i < 7; ++i) {
            while (1) {
                for (k=1000; k >= 0; --k){}
                if (UCA0TXIFG & IFG2) {
                    if (i == 0) {
                        UCA0TXBUF = cursorHome;

                        for (k=1000; k >= 0; --k){}

                        break;
                    }
                    else if (i < 4) {
                        UCA0TXBUF = result[i-1];
                        for (k=700; k >= 0; --k){}
                        break;
                    }
                    else {
                        UCA0TXBUF = units[i-4];
                        for (k=700; k >= 0; --k){}
                        break;
                    }
                }
            }
        }
    }
	
	return 0;
}
