#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>


void    ftoa(float, char *, int);
int     intToStr(int x, char str[], int d);
void    reverse(char *str, int len);
void    init_UART(void);


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
	char result[] = {0,0,0};
	float cm;

	P1DIR  |=  BIT4;
	P1SEL  &= ~BIT4;
	P1SEL2 &= ~BIT4;


    while(1) {
        int j;
        for (j = 1000; j >= 0; --j) {

            if (j == 5){

                P1OUT |= 16;
            }
            else if (j == 0) {
                P1OUT &= ~16;
            }
        }

        //  do we need between trigger low and echo high?
        asm("    nop\n \t nop");


        // count microseconds of echo
        while (P1IN & 8) {
            ++longUS;
        }

        //   do calculations
        cm = longUS / 58.0;  // need to compensate for delays in while loop
        ftoa(cm, result, 0);

        // send data to RX
        /* must send ascii device ctrl 1 char first
         to cursor home RX
         */
        unsigned int i;
        for (i = 0; i < 8; i++) {
            while (1) {
                if (UCA0TXIFG & IFG2) {
                    if (i == 0) {
                        UCA0TXBUF = cursorHome;
                        break;
                    }
                    else if (i < 4) {
                        UCA0TXBUF = result[i-1];
                        break;
                    }
                    else {
                        UCA0TXBUF = units[i-4];
                        break;
                    }
                }
            }
        }
    }
	
	return 0;
}
