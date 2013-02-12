/*****< main.c >***************************************************************/
/*      Copyright 2001 - 2012 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  MAIN - Main application implementation.                                   */
/*                                                                            */
/*  Author:  Tim Cook                                                         */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   10/28/11  T. Cook        Initial creation.                               */
/******************************************************************************/
#include "HAL.h"                 /* Function for Hardware Abstraction.        */
#include "Main.h"                /* Main application header.                  */
#include "EHCILL.h"              /* eHCILL Implementation Header.             */

#include "i2c_lib.h"

#define MAX_COMMAND_LENGTH                         (64)  /* Denotes the max   */
                                                         /* buffer size used  */
                                                         /* for user commands */
                                                         /* input via the     */
                                                         /* User Interface.   */

#define LED_TOGGLE_RATE_SUCCESS                    (500) /* The LED Toggle    */
                                                         /* rate when the demo*/
                                                         /* successfully      */
                                                         /* starts up.        */

   /* The following parameters are used when configuring HCILL Mode.    */
#define HCILL_MODE_INACTIVITY_TIMEOUT              (500)
#define HCILL_MODE_RETRANSMIT_TIMEOUT              (100)

   /* The following is used as a printf replacement.                    */
#define Display(_x)                 do { BTPS_OutputMessage _x; } while(0)

   /* Internal Variables to this Module (Remember that all variables    */
   /* declared static are initialized to 0 automatically by the         */
   /* compiler as part of standard C/C++).                              */
static unsigned int BluetoothStackID;
static unsigned int InputIndex;
static char         Input[MAX_COMMAND_LENGTH];

   /* Internal function prototypes.                                     */
static int GetInput(void);

   /* Application Tasks.                                                */
static void DisplayCallback(char Character);
static unsigned long GetTickCallback(void);
static void ProcessCharacters(void *UserParameter);
static void IdleFunction(void *UserParameter);
static void MainThread(void);

   /* The following function is responsible for retrieving the Commands */
   /* from the Serial Input routines and copying this Command into the  */
   /* specified Buffer.  This function blocks until a Command (defined  */
   /* to be a NULL terminated ASCII string).  The Serial Data Callback  */
   /* is responsible for building the Command and dispatching the Signal*/
   /* that this function waits for.                                     */
static int GetInput(void)
{
   char Char;
   int  Done;

   /* Initialize the Flag indicating a complete line has been parsed.   */
   Done = 0;

   /* Attempt to read data from the Console.                            */
   if(HAL_ConsoleRead(1, &Char))
   {
      switch(Char)
      {
         case '\r':
         case '\n':
            /* This character is a new-line or a line-feed character    */
            /* NULL terminate the Input Buffer.                         */
            Input[InputIndex] = '\0';

            /* Set Done to the number of bytes that are to be returned. */
            /* ** NOTE ** In the current implementation any data after a*/
            /*            new-line or line-feed character will be lost. */
            /*            This is fine for how this function is used is */
            /*            no more data will be entered after a new-line */
            /*            or line-feed.                                 */
            Done       = (InputIndex-1);
            InputIndex = 0;
            break;
         case 0x08:
            /* Backspace has been pressed, so now decrement the number  */
            /* of bytes in the buffer (if there are bytes in the        */
            /* buffer).                                                 */
            if(InputIndex)
            {
               InputIndex--;
               HAL_ConsoleWrite(3, "\b \b");
            }
            break;
         default:
            /* Accept any other printable characters.                   */
            if((Char >= ' ') && (Char <= '~'))
            {
               /* Add the Data Byte to the Input Buffer, and make sure  */
               /* that we do not overwrite the Input Buffer.            */
               Input[InputIndex++] = Char;
               HAL_ConsoleWrite(1, &Char);

               /* Check to see if we have reached the end of the buffer.*/
               if(InputIndex == (MAX_COMMAND_LENGTH-1))
               {
                  Input[InputIndex] = 0;
                  Done              = (InputIndex-1);
                  InputIndex        = 0;
               }
            }
            break;
      }
   }

   return(Done);
}

   /* The following function is registered with the application so that */
   /* it can display strings to the debug UART.                         */
static void DisplayCallback(char Character)
{
   HAL_ConsoleWrite(1, &Character);
}

   /* The following function is registered with the application so that */
   /* it can get the current System Tick Count.                         */
static unsigned long GetTickCallback(void)
{
   return(HAL_GetTickCount());
}

   /* The following function processes terminal input.                  */
static void ProcessCharacters(void *UserParameter)
{
   /* Check to see if we have a command to process.                     */
   if(GetInput() > 0)
   {
      /* Attempt to process a character.                                */
      //ProcessCommandLine(Input);
   }
}
   /* The following function is responsible for checking the idle state */
   /* and possibly entering LPM3 mode.                                  */
static void IdleFunction(void *UserParameter)
{
   HCILL_State_t HCILL_State;

   /* Determine the HCILL State.                                        */
   HCILL_State = HCILL_GetState();

   /* If the stack is Idle and we are in HCILL Sleep, then we may enter */
   /* LPM3 mode (with Timer Interrupts disabled).                       */
   if((BSC_QueryStackIdle(BluetoothStackID)) && (HCILL_State == hsSleep) && (!HCILL_Get_Power_Lock_Count()))
   {
      /* Enter MSP430 LPM3 with Timer Interrupts disabled (we will      */
      /* require an interrupt to wake us up from this state).           */
      HAL_LowPowerMode((unsigned char)TRUE);
   }
   else
      HAL_LedToggle(0);
}

   /* The following function is the main user interface thread.  It     */
   /* opens the Bluetooth Stack and then drives the main user interface.*/
static void MainThread(void)
{
   int                     Result;
   BTPS_Initialization_t   BTPS_Initialization;
   HCI_DriverInformation_t HCI_DriverInformation;

   /* Configure the UART Parameters.                                    */
   HCI_DRIVER_SET_COMM_INFORMATION(&HCI_DriverInformation, 1, 115200, cpUART);
   HCI_DriverInformation.DriverInformation.COMMDriverInformation.InitializationDelay = 100;

   /* Set up the application callbacks.                                 */
   BTPS_Initialization.GetTickCountCallback  = GetTickCallback;
   BTPS_Initialization.MessageOutputCallback = DisplayCallback;

   /* Initialize the application.                                       */
   if((Result = InitializeApplication(&HCI_DriverInformation, &BTPS_Initialization)) > 0)
   {
      /* Save the Bluetooth Stack ID.                                   */
      BluetoothStackID = (unsigned int)Result;

      /* Go ahead an enable HCILL Mode.                                 */
      HCILL_Init();
      HCILL_Configure(BluetoothStackID, HCILL_MODE_INACTIVITY_TIMEOUT, HCILL_MODE_RETRANSMIT_TIMEOUT, TRUE);

      /* We need to execute Add a function to process the command line  */
      /* to the BTPS Scheduler.                                         */
      if(BTPS_AddFunctionToScheduler(ProcessCharacters, NULL, 200))
      {
         /* Add the idle function (which determines if LPM3 may be      */
         /* entered) to the scheduler.                                  */
         if(BTPS_AddFunctionToScheduler(IdleFunction, NULL, HCILL_MODE_INACTIVITY_TIMEOUT))
         {
            /* Loop forever and execute the scheduler.                  */
            while(1)
               BTPS_ExecuteScheduler();
         }
      }
   }
}

   /* The following is the Main application entry point.  This function */
   /* will configure the hardware and initialize the OS Abstraction     */
   /* layer, create the Main application thread and start the scheduler.*/
int main(void)
{
   /* Turn off the watchdog timer                                       */
   WDTCTL = WDTPW | WDTHOLD;

   /* Configure the hardware for its intended use.                      */
   HAL_ConfigureHardware();

   initi2c();

   /* Enable interrupts and call the main application thread.           */
   __enable_interrupt();
   MainThread();

   /* MainThread should run continously, if it exits an error occured.  */
   while(1)
   {
      HAL_LedToggle(0);
      BTPS_Delay(100);
   }
}

/*
// for printf support
#include <stdio.h>
#include <stdlib.h>

int fputc(int _c, register FILE *_fp)
{
	MessageOutputCallback(_c);

	return((unsigned char)_c);
}*/
