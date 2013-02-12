#include <msp430f5438a.h>
#include "i2c_lib.h"

#define lib

#ifdef lib
unsigned char *PTxData;                     // Pointer to TX data
unsigned char TXByteCtr;

unsigned char *PRxData;                     // Pointer to RX data
unsigned char RXByteCtr;

int err;



void initi2c()
{
	P10SEL |= 0x06;                            // Assign I2C pins to USCI_B0
	UCB3CTL1 |= UCSWRST;                      // Enable SW reset
	UCB3CTL0 = UCMST + UCMODE_3 + UCSYNC;     // I2C Master, synchronous mode
	UCB3CTL1 = UCSSEL_2 + UCSWRST;            // Use SMCLK, keep SW reset
	UCB3BR0 = 160;                             // fSCL = SMCLK/12 = ~100kHz
	UCB3BR1 = 0;
	UCB3CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
	UCB3IE |= UCTXIE + UCNACKIE + UCRXIE;     // Enable TX interrupt, enable NACK interrupt; Enable RX interrupt
}

int sendi2c(unsigned char addr, unsigned char TxData[], unsigned char len)
{
	if(len == 0)
		return 0;

	//while (UCB3CTL1 & UCTXSTP);             // Ensure stop condition got sent

	err = 0;
	UCB3I2CSA = addr;                         // Slave Address is 048h
	PTxData = (unsigned char *)TxData;      // TX array start address
											// Place breakpoint here to see each
											// transmit operation.
	TXByteCtr = len;              // Load TX byte counter

	UCB3CTL1 |= UCTR + UCTXSTT;             // I2C TX, start condition

	__bis_SR_register(LPM0_bits + GIE);     // Enter LPM0, enable interrupts
	__no_operation();                       // Remain in LPM0 until all data
											// is TX'd

	return err;
}

int geti2c(unsigned char addr, unsigned char RxData[], unsigned char len)		//Assume a always a previous send command to same device in order to specify read register
{
	if(len == 0)
		return 0;

	err = 0;

	PRxData = RxData;
	if(len == 1)
	{
		RXByteCtr = len;
		//while (UCB3CTL1 & UCTXSTP);             // Ensure stop condition got sent

		UCB3I2CSA = addr;                         // Slave Address is 048h


		UCB3CTL1 &= ~UCTR;             			// I2C RX
		UCB3CTL1 |= UCTXSTT;                    // I2C start condition
		while(UCB3CTL1 & UCTXSTT);              // Start condition sent?
		UCB3CTL1 |= UCTXSTP;                    // I2C stop condition

		__bis_SR_register(LPM0_bits + GIE);     // Enter LPM0, enable interrupts
		__no_operation();                       // For debugger
	}
	else
	{
		RXByteCtr = len;                          // Load RX byte counter
		//while (UCB3CTL1 & UCTXSTP);             // Ensure stop condition got sent

		UCB3I2CSA = addr;                         // Slave Address is 048h

		UCB3CTL1 &= ~UCTR;             			// I2C RX
		UCB3CTL1 |= UCTXSTT;                    // I2C start condition

		__bis_SR_register(LPM0_bits + GIE);     // Enter LPM0, enable interrupts
												// Remain in LPM0 until all data
												// is RX'd
		__no_operation();                       // Set breakpoint >>here<< and
	}

	/*
	if( len == 1 )
	{
		RXByteCtr = 0;
		__disable_interrupt();
		UCB3CTL1 |= UCTXSTT;                      // I2C start condition
		while(UCB3CTL1 & UCTXSTT);               // Start condition sent?
		UCB3CTL1 |= UCTXSTP;                      // I2C stop condition
		__enable_interrupt();
	}
	else
	{
		RXByteCtr = len;
		UCB3CTL1 |= UCTXSTT;                      // I2C start condition
	}

	//__bis_SR_register(LPM0_bits + GIE);     // Enter LPM0, enable interrupts
	                                        // Remain in LPM0 until all data is RX'd
	__no_operation();                       // Set breakpoint >>here<< and read out the RxBuffer buffer

	while (UCB3CTL1 & UCTXSTP);             // Ensure stop condition got sent*/
	return err;
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
  case  0: break;                           // Vector  0: No interrupts
  case  2: break;                           // Vector  2: ALIFG
  case  4: 									// Vector  4: NACKIFG
	  err = 1;
  	  UCB3CTL1 |= UCTXSTP;                  // I2C stop condition
  	  UCB3STAT &= ~UCNACKIFG;
  	  __bic_SR_register_on_exit(LPM0_bits); // Exit LPM0
  	  break;
  case  6: break;                           // Vector  6: STTIFG
  case  8:                           // Vector  8: STPIFG
	  //__bic_SR_register_on_exit(LPM0_bits); // Exit active CPU
	  break;
  case 10:                                  // Vector 10: RXIFG

	  /*
	  if(RXByteCtr == 1)
		  UCB3CTL1 |= UCTXSTP;                    // I2C stop condition

	  if ( RXByteCtr == 0 )
	  {
		  *PRxData = UCB3RXBUF;
		  PRxData++;
		  //__bic_SR_register_on_exit(LPM0_bits); // Exit LPM0
	  }
	  else
	  {
		  *PRxData = UCB3RXBUF;
		  PRxData++;
		  RXByteCtr--;
	  }*/
	  RXByteCtr--;                            // Decrement RX byte counter
	  if (RXByteCtr)
	  {
		  *PRxData++ = UCB3RXBUF;               // Move RX data to address PRxData
		  if (RXByteCtr == 1)                   // Only one byte left?
			  UCB3CTL1 |= UCTXSTP;                // Generate I2C stop condition
	  }
	  else
	  {
		  *PRxData = UCB3RXBUF;                 // Move final RX data to PRxData
		  __bic_SR_register_on_exit(LPM0_bits); // Exit active CPU
	  }
    break;
  case 12:                                  // Vector 12: TXIFG
	  /*
	  if (TXByteCtr)                          // Check TX byte counter
	  {
		  UCB3TXBUF = *PTxData++;               // Load TX buffer
		  TXByteCtr--;                          // Decrement TX byte counter
	  }
	  else
	  {
		  UCB3CTL1 |= UCTXSTP;                  // I2C stop condition
		  //UCB3IFG &= ~UCTXIFG;                  // Clear USCI_B0 TX int flag
		  //__bic_SR_register_on_exit(LPM0_bits); // Exit LPM0
	  }
	  */
	  if (TXByteCtr)                          // Check TX byte counter
	  {
		UCB3TXBUF = *PTxData++;               // Load TX buffer
		TXByteCtr--;                          // Decrement TX byte counter
	  }
	  else
	  {
		UCB3CTL1 |= UCTXSTP;                  // I2C stop condition
		UCB3IFG &= ~UCTXIFG;                  // Clear USCI_B0 TX int flag
		__bic_SR_register_on_exit(LPM0_bits); // Exit LPM0
	  }
  default: break;
  }
}

#endif
