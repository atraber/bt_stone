
#include <msp430f5438a.h>
#include "I2C.h"

unsigned char* g_pI2CData;
unsigned char g_I2CCount;

int g_I2CError;


void I2C_init()
{
	P10SEL = BIT1 + BIT2;                     // Assign I2C pins to USCI_B0
	UCB3CTL1 |= UCSWRST;                      // Enable SW reset
	UCB3CTL0 = UCMST + UCMODE_3 + UCSYNC;     // I2C Master, synchronous mode
	UCB3CTL1 = UCSSEL_2 + UCSWRST;            // Use SMCLK, keep SW reset
	UCB3BR0 = 160;                            // fSCL = SMCLK/160 = ~100kHz
	UCB3BR1 = 0;
	UCB3CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
	UCB3IE |= UCTXIE + UCNACKIE + UCRXIE;     // Enable TX interrupt, enable NACK interrupt; Enable RX interrupt
}

int I2C_write(unsigned char addr, unsigned char* TxData, unsigned char len)
{
	if(len == 0)
		return 0;

	// load data into globals
	g_I2CCount = len;
	g_pI2CData = TxData;
	g_I2CError = 0;

	// set slave address
	UCB3I2CSA = addr;

	UCB3CTL1 |= UCTR + UCTXSTT;             // I2C TX, start condition

	__bis_SR_register(LPM0_bits + GIE);     // Enter LPM0, enable interrupts
	__no_operation();                       // Remain in LPM0 until all data
											// is TX'd

	return g_I2CError;
}

int I2C_read(unsigned char addr, unsigned char* RxData, unsigned char len)
{
	// check for invalid parameters
	if(len == 0)
		return 0;

	// load data into globals
	g_I2CCount = len;
	g_pI2CData = RxData;
	g_I2CError = 0;

	// set slave address
	UCB3I2CSA = addr;

	if(len == 1)
	{
		UCB3CTL1 &= ~UCTR;             			// I2C RX
		UCB3CTL1 |= UCTXSTT;                    // I2C start condition
		while(UCB3CTL1 & UCTXSTT);              // Start condition sent?
		UCB3CTL1 |= UCTXSTP;                    // I2C stop condition

		__bis_SR_register(LPM0_bits + GIE);     // Enter LPM0, enable interrupts

		__no_operation();                       // For debugger
	}
	else
	{
		UCB3CTL1 &= ~UCTR;             			// I2C RX
		UCB3CTL1 |= UCTXSTT;                    // I2C start condition

		__bis_SR_register(LPM0_bits + GIE);     // Enter LPM0, enable interrupts

		__no_operation();                       // Set breakpoint >>here<< and
	}

	return g_I2CError;
}

//------------------------------------------------------------------------------
// The USCIAB0TX_ISR is structured such that it can be used to transmit any
// number of bytes by pre-loading TXByteCtr with the byte count. Also, TXData
// points to the next byte to transmit.
//------------------------------------------------------------------------------
#pragma vector = USCI_B3_VECTOR
__interrupt void USCI_B3_ISR(void)
{
	switch(__even_in_range(UCB3IV,12))
	{
		case  0:                                  // Vector  0: No interrupts
			break;
		case  2:                                  // Vector  2: ALIFG
			break;
		case  4: 							      // Vector  4: NACKIFG
			// NACK means the device did not respond => set error flag
			g_I2CError = 1;

			UCB3CTL1 |= UCTXSTP;                  // I2C stop condition
			UCB3STAT &= ~UCNACKIFG;

			__bic_SR_register_on_exit(LPM0_bits); // Exit LPM0
			break;
		case  6:                                  // Vector  6: STTIFG
			break;
		case  8:                                  // Vector  8: STPIFG
			break;
		case 10:                                  // Vector 10: RXIFG
			g_I2CCount--;                         // Decrement RX byte counter
			if (g_I2CCount)
			{
				*g_pI2CData++ = UCB3RXBUF;               // Move RX data to address PRxData

				// generate stop condition if only one byte is left
				if(g_I2CCount == 1)
					UCB3CTL1 |= UCTXSTP;                // Generate I2C stop condition
			}
			else
			{
				*g_pI2CData = UCB3RXBUF;              // Move final RX data to PRxData
				__bic_SR_register_on_exit(LPM0_bits); // Exit active CPU
			}
			break;
		case 12:                                    // Vector 12: TXIFG
			if(g_I2CCount)                          // Check TX byte counter
			{
				UCB3TXBUF = *g_pI2CData++;             // Load TX buffer
				g_I2CCount--;                          // Decrement TX byte counter
			}
			else
			{
				UCB3CTL1 |= UCTXSTP;                  // I2C stop condition
				UCB3IFG &= ~UCTXIFG;                  // Clear USCI_B0 TX int flag
				__bic_SR_register_on_exit(LPM0_bits); // Exit LPM0
			}
			break;
		default:
			break;
	}
}
