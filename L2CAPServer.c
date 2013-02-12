
#include "Main.h"                /* Application Interface Abstraction.        */
#include "SS1BTPS.h"             /* Main SS1 Bluetooth Stack Header.          */
#include "L2CAPServer.h"             /* Application Header.                       */
#include "BTPSKRNL.h"            /* BTPS Kernel Header.                       */


#include "protocol.h"

#include <stdio.h>
#include <string.h>

#define MAX_SUPPORTED_COMMANDS                     (36)  /* Denotes the       */
                                                         /* maximum number of */
                                                         /* User Commands that*/
                                                         /* are supported by  */
                                                         /* this application. */

#define MAX_NUM_OF_PARAMETERS                       (5)  /* Denotes the max   */
                                                         /* number of         */
                                                         /* parameters a      */
                                                         /* command can have. */

#define MAX_INQUIRY_RESULTS                         (5)  /* Denotes the max   */
                                                         /* number of inquiry */
                                                         /* results.          */

#define MAX_SUPPORTED_LINK_KEYS                    (1)   /* Max supported Link*/
                                                         /* keys.             */

#define DEFAULT_IO_CAPABILITY          (icDisplayYesNo)  /* Denotes the       */
                                                         /* default I/O       */
                                                         /* Capability that is*/
                                                         /* used with Secure  */
                                                         /* Simple Pairing.   */

#define DEFAULT_MITM_PROTECTION                  (TRUE)  /* Denotes the       */
                                                         /* default value used*/
                                                         /* for Man in the    */
                                                         /* Middle (MITM)     */
                                                         /* protection used   */
                                                         /* with Secure Simple*/
                                                         /* Pairing.          */

#define TEST_DATA              "This is a test string."  /* Denotes the data  */
                                                         /* that is sent via  */
                                                         /* SPP when calling  */
                                                         /* SPP_Data_Write(). */

#define NO_COMMAND_ERROR                           (-1)  /* Denotes that no   */
                                                         /* command was       */
                                                         /* specified to the  */
                                                         /* parser.           */

#define INVALID_COMMAND_ERROR                      (-2)  /* Denotes that the  */
                                                         /* Command does not  */
                                                         /* exist for         */
                                                         /* processing.       */

#define EXIT_CODE                                  (-3)  /* Denotes that the  */
                                                         /* Command specified */
                                                         /* was the Exit      */
                                                         /* Command.          */

#define FUNCTION_ERROR                             (-4)  /* Denotes that an   */
                                                         /* error occurred in */
                                                         /* execution of the  */
                                                         /* Command Function. */

#define TO_MANY_PARAMS                             (-5)  /* Denotes that there*/
                                                         /* are more          */
                                                         /* parameters then   */
                                                         /* will fit in the   */
                                                         /* UserCommand.      */

#define INVALID_PARAMETERS_ERROR                   (-6)  /* Denotes that an   */
                                                         /* error occurred due*/
                                                         /* to the fact that  */
                                                         /* one or more of the*/
                                                         /* required          */
                                                         /* parameters were   */
                                                         /* invalid.          */

#define UNABLE_TO_INITIALIZE_STACK                 (-7)  /* Denotes that an   */
                                                         /* error occurred    */
                                                         /* while Initializing*/
                                                         /* the Bluetooth     */
                                                         /* Protocol Stack.   */

#define INVALID_STACK_ID_ERROR                     (-8)  /* Denotes that an   */
                                                         /* occurred due to   */
                                                         /* attempted         */
                                                         /* execution of a    */
                                                         /* Command when a    */
                                                         /* Bluetooth Protocol*/
                                                         /* Stack has not been*/
                                                         /* opened.           */

#define UNABLE_TO_REGISTER_SERVER                  (-9)  /* Denotes that an   */
                                                         /* error occurred    */
                                                         /* when trying to    */
                                                         /* create a Serial   */
                                                         /* Port Server.      */

#define EXIT_MODE                                  (-10) /* Flags exit from   */
                                                         /* any Mode.         */

   /* The following represent the possible values of UI_Mode variable.  */
#define UI_MODE_IS_CLIENT      (2)
#define UI_MODE_IS_SERVER      (1)
#define UI_MODE_SELECT         (0)
#define UI_MODE_IS_INVALID     (-1)

   /* Following converts a Sniff Parameter in Milliseconds to frames.   */
#define MILLISECONDS_TO_BASEBAND_SLOTS(_x)   ((_x) / (0.625))

   /* The following is used as a printf replacement.                    */
#define Display(_x)                 do { BTPS_OutputMessage _x; } while(0)

   /* The following type definition represents the container type which */
   /* holds the mapping between Bluetooth devices (based on the BD_ADDR)*/
   /* and the Link Key (BD_ADDR <-> Link Key Mapping).                  */
typedef struct _tagLinkKeyInfo_t
{
   BD_ADDR_t  BD_ADDR;
   Link_Key_t LinkKey;
} LinkKeyInfo_t;

   /* The following type definition represents the structure which holds*/
   /* all information about the parameter, in particular the parameter  */
   /* as a string and the parameter as an unsigned int.                 */
typedef struct _tagParameter_t
{
   char     *strParam;
   SDWord_t  intParam;
} Parameter_t;

   /* The following type definition represents the structure which holds*/
   /* a list of parameters that are to be associated with a command The */
   /* NumberofParameters variable holds the value of the number of      */
   /* parameters in the list.                                           */
typedef struct _tagParameterList_t
{
   int         NumberofParameters;
   Parameter_t Params[MAX_NUM_OF_PARAMETERS];
} ParameterList_t;

   /* The following type definition represents the structure which holds*/
   /* the command and parameters to be executed.                        */
typedef struct _tagUserCommand_t
{
   char            *Command;
   ParameterList_t  Parameters;
} UserCommand_t;

   /* The following type definition represents the generic function     */
   /* pointer to be used by all commands that can be executed by the    */
   /* test program.                                                     */
typedef int (*CommandFunction_t)(ParameterList_t *TempParam);

   /* The following type definition represents the structure which holds*/
   /* information used in the interpretation and execution of Commands. */
typedef struct _tagCommandTable_t
{
   char              *CommandName;
   CommandFunction_t  CommandFunction;
} CommandTable_t;

   /* User to represent a structure to hold a BD_ADDR return from       */
   /* BD_ADDRToStr.                                                     */
typedef char BoardStr_t[15];

   /* The following structure holds status information about a send     */
   /* process.                                                          */
typedef struct _tagSend_Info_t
{
   DWord_t BytesToSend;
   DWord_t BytesSent;
} Send_Info_t;

   /* Internal Variables to this Module (Remember that all variables    */
   /* declared static are initialized to 0 automatically by the         */
   /* compiler as part of standard C/C++).                              */

static int                 UI_Mode;                 /* Holds the UI Mode.              */

static unsigned int        BluetoothStackID;        /* Variable which holds the Handle */
                                                    /* of the opened Bluetooth Protocol*/
                                                    /* Stack.                          */

static int                 SerialPortID;            /* Variable which contains the     */
                                                    /* Handle of the most recent       */
                                                    /* SPP Port that was opened.       */

static BD_ADDR_t           InquiryResultList[MAX_INQUIRY_RESULTS]; /* Variable which   */
                                                    /* contains the inquiry result     */
                                                    /* received from the most recently */
                                                    /* preformed inquiry.              */

static unsigned int        NumberofValidResponses;  /* Variable which holds the number */
                                                    /* of valid inquiry results within */
                                                    /* the inquiry results array.      */

static LinkKeyInfo_t       LinkKeyInfo[MAX_SUPPORTED_LINK_KEYS]; /* Variable holds     */
                                                    /* BD_ADDR <-> Link Keys for       */
                                                    /* pairing.                        */

static BD_ADDR_t           CurrentRemoteBD_ADDR;    /* Variable which holds the        */
                                                    /* current BD_ADDR of the device   */
                                                    /* which is currently pairing or   */
                                                    /* authenticating.                 */

static GAP_IO_Capability_t IOCapability;            /* Variable which holds the        */
                                                    /* current I/O Capabilities that   */
                                                    /* are to be used for Secure Simple*/
                                                    /* Pairing.                        */

static Boolean_t           OOBSupport;              /* Variable which flags whether    */
                                                    /* or not Out of Band Secure Simple*/
                                                    /* Pairing exchange is supported.  */

static Boolean_t           MITMProtection;          /* Variable which flags whether or */
                                                    /* not Man in the Middle (MITM)    */
                                                    /* protection is to be requested   */
                                                    /* during a Secure Simple Pairing  */
                                                    /* procedure.                      */

static BD_ADDR_t           SelectedBD_ADDR;         /* Holds address of selected Device*/

static BD_ADDR_t           NullADDR;                /* Holds a NULL BD_ADDR for comp.  */
                                                    /* purposes.                       */

static Boolean_t           LoopbackActive;          /* Variable which flags whether or */
                                                    /* not the application is currently*/
                                                    /* operating in Loopback Mode      */
                                                    /* (TRUE) or not (FALSE).          */

static Boolean_t           DisplayRawData;          /* Variable which flags whether or */
                                                    /* not the application is to       */
                                                    /* simply display the Raw Data     */
                                                    /* when it is received (when not   */
                                                    /* operating in Loopback Mode).    */

static Boolean_t           AutomaticReadActive;     /* Variable which flags whether or */
                                                    /* not the application is to       */
                                                    /* automatically read all data     */
                                                    /* as it is received.              */

static unsigned int        NumberCommands;          /* Variable which is used to hold  */
                                                    /* the number of Commands that are */
                                                    /* supported by this application.  */
                                                    /* Commands are added individually.*/

static BoardStr_t          Callback_BoardStr;       /* Holds a BD_ADDR string in the   */
                                                    /* Callbacks.                      */


   /* The following string table is used to map HCI Version information */
   /* to an easily displayable version string.                          */
static BTPSCONST char *HCIVersionStrings[] =
{
   "1.0b",
   "1.1",
   "1.2",
   "2.0",
   "2.1",
   "3.0",
   "4.0",
   "Unknown (greater 4.0)"
} ;

#define NUM_SUPPORTED_HCI_VERSIONS              (sizeof(HCIVersionStrings)/sizeof(char *) - 1)

   /* The following string table is used to map the API I/O Capabilities*/
   /* values to an easily displayable string.                           */
static BTPSCONST char *IOCapabilitiesStrings[] =
{
   "Display Only",
   "Display Yes/No",
   "Keyboard Only",
   "No Input/Output"
} ;

   /* Internal function prototypes.                                     */
static void BD_ADDRToStr(BD_ADDR_t Board_Address, BoardStr_t BoardStr);
static void DisplayPrompt(void);
static void DisplayUsage(char *UsageString);
static void DisplayFunctionError(char *Function,int Status);
static void DisplayFunctionSuccess(char *Function);

static int OpenStack(HCI_DriverInformation_t *HCI_DriverInformation, BTPS_Initialization_t *BTPS_Initialization);
static int CloseStack(void);

static int SetConnect(void);
static int SetPairable(void);
static int DeleteLinkKey(BD_ADDR_t BD_ADDR);

static int SetLocalName(char* name);

static int SetBaudRate(ParameterList_t *TempParam);

   /* BTPS Callback function prototypes.                                */
static void BTPSAPI L2CAP_Event_Callback(unsigned int BluetoothStackID, L2CA_Event_Data_t *L2CA_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI GAP_Event_Callback(unsigned int BluetoothStackID, GAP_Event_Data_t *GAP_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI HCI_Event_Callback(unsigned int BluetoothStackID, HCI_Event_Data_t *HCI_Event_Data, unsigned long CallbackParameter);

   /* The following function is responsible for converting data of type */
   /* BD_ADDR to a string.  The first parameter of this function is the */
   /* BD_ADDR to be converted to a string.  The second parameter of this*/
   /* function is a pointer to the string in which the converted BD_ADDR*/
   /* is to be stored.                                                  */
static void BD_ADDRToStr(BD_ADDR_t Board_Address, BoardStr_t BoardStr)
{
   BTPS_SprintF((char *)BoardStr, "0x%02X%02X%02X%02X%02X%02X", Board_Address.BD_ADDR5, Board_Address.BD_ADDR4, Board_Address.BD_ADDR3, Board_Address.BD_ADDR2, Board_Address.BD_ADDR1, Board_Address.BD_ADDR0);
}

   /* Displays the correct prompt depening on the Server/Client Mode.   */
static void DisplayPrompt(void)
{
   if(UI_Mode == UI_MODE_IS_CLIENT)
      Display(("\r\nClient>"));
   else
   {
      if(UI_Mode == UI_MODE_IS_SERVER)
         Display(("\r\nServer>"));
      else
         Display(("\r\nChoose Mode>"));
   }
}

   /* Displays a usage string..                                         */
static void DisplayUsage(char *UsageString)
{
   Display(("\nUsage: %s.\r\n", UsageString));
}

   /* Displays a function error.                                        */
static void DisplayFunctionError(char *Function, int Status)
{
   Display(("\n%s Failed: %d.\r\n", Function, Status));
}

static void DisplayFunctionSuccess(char *Function)
{
   Display(("\n%s success.\r\n", Function));
}

   /* The following function is responsible for opening the SS1         */
   /* Bluetooth Protocol Stack.  This function accepts a pre-populated  */
   /* HCI Driver Information structure that contains the HCI Driver     */
   /* Transport Information.  This function returns zero on successful  */
   /* execution and a negative value on all errors.                     */
static int OpenStack(HCI_DriverInformation_t *HCI_DriverInformation, BTPS_Initialization_t *BTPS_Initialization)
{
   int                        Result;
   int                        ret_val = 0;
   char                       BluetoothAddress[15];
   Byte_t                     Status;
   BD_ADDR_t                  BD_ADDR;
   HCI_Version_t              HCIVersion;
   L2CA_Link_Connect_Params_t L2CA_Link_Connect_Params;

   /* First check to see if the Stack has already been opened.          */
   if(!BluetoothStackID)
   {
      /* Next, makes sure that the Driver Information passed appears to */
      /* be semi-valid.                                                 */
      if((HCI_DriverInformation) && (BTPS_Initialization))
      {
         Display(("\r\n"));

         /* Initialize BTPSKNRl.                                        */
         BTPS_Init(BTPS_Initialization);

         Display(("OpenStack().\r\n"));

         /* Initialize the Stack                                        */
         Result = BSC_Initialize(HCI_DriverInformation, 0);

         /* Next, check the return value of the initialization to see if*/
         /* it was successful.                                          */
         if(Result > 0)
         {
            /* The Stack was initialized successfully, inform the user  */
            /* and set the return value of the initialization function  */
            /* to the Bluetooth Stack ID.                               */
            BluetoothStackID = Result;
            Display(("Bluetooth Stack ID: %d\r\n", BluetoothStackID));

            /* Initialize the default Secure Simple Pairing parameters. */
            IOCapability     = DEFAULT_IO_CAPABILITY;
            OOBSupport       = FALSE;
            MITMProtection   = DEFAULT_MITM_PROTECTION;

            if(!HCI_Version_Supported(BluetoothStackID, &HCIVersion))
               Display(("Device Chipset: %s\r\n", (HCIVersion <= NUM_SUPPORTED_HCI_VERSIONS)?HCIVersionStrings[HCIVersion]:HCIVersionStrings[NUM_SUPPORTED_HCI_VERSIONS]));

            /* Let's output the Bluetooth Device Address so that the    */
            /* user knows what the Device Address is.                   */
            if(!GAP_Query_Local_BD_ADDR(BluetoothStackID, &BD_ADDR))
            {
               BD_ADDRToStr(BD_ADDR, BluetoothAddress);

               Display(("BD_ADDR: %s\r\n", BluetoothAddress));
            }

            /* Go ahead and allow Master/Slave Role Switch.             */
            L2CA_Link_Connect_Params.L2CA_Link_Connect_Request_Config  = cqAllowRoleSwitch;
            L2CA_Link_Connect_Params.L2CA_Link_Connect_Response_Config = csMaintainCurrentRole;

            L2CA_Set_Link_Connection_Configuration(BluetoothStackID, &L2CA_Link_Connect_Params);

            if(HCI_Command_Supported(BluetoothStackID, HCI_SUPPORTED_COMMAND_WRITE_DEFAULT_LINK_POLICY_BIT_NUMBER) > 0)
               HCI_Write_Default_Link_Policy_Settings(BluetoothStackID, (HCI_LINK_POLICY_SETTINGS_ENABLE_MASTER_SLAVE_SWITCH|HCI_LINK_POLICY_SETTINGS_ENABLE_SNIFF_MODE), &Status);

            /* Delete all Stored Link Keys.                             */
            ASSIGN_BD_ADDR(BD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

            DeleteLinkKey(BD_ADDR);
         }
         else
         {
            /* The Stack was NOT initialized successfully, inform the   */
            /* user and set the return value of the initialization      */
            /* function to an error.                                    */
            DisplayFunctionError("Stack Init", Result);

            BluetoothStackID = 0;

            ret_val          = UNABLE_TO_INITIALIZE_STACK;
         }
      }
      else
      {
         /* One or more of the necessary parameters are invalid.        */
         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }

   return(ret_val);
}

   /* The following function is responsible for closing the SS1         */
   /* Bluetooth Protocol Stack.  This function requires that the        */
   /* Bluetooth Protocol stack previously have been initialized via the */
   /* OpenStack() function.  This function returns zero on successful   */
   /* execution and a negative value on all errors.                     */
static int CloseStack(void)
{
   int ret_val = 0;

   /* First check to see if the Stack has been opened.                  */
   if(BluetoothStackID)
   {
      /* Simply close the Stack                                         */
      BSC_Shutdown(BluetoothStackID);

      /* Free BTPSKRNL allocated memory.                                */
      BTPS_DeInit();

      Display(("Stack Shutdown.\r\n"));

      /* Flag that the Stack is no longer initialized.                  */
      BluetoothStackID = 0;

      /* Flag success to the caller.                                    */
      ret_val          = 0;
   }
   else
   {
      /* A valid Stack ID does not exist, inform to user.               */
      ret_val = UNABLE_TO_INITIALIZE_STACK;
   }

   return(ret_val);
}

   /* The following function is responsible for placing the Local       */
   /* Bluetooth Device into General Discoverablity Mode.  Once in this  */
   /* mode the Device will respond to Inquiry Scans from other Bluetooth*/
   /* Devices.  This function requires that a valid Bluetooth Stack ID  */
   /* exists before running.  This function returns zero on successful  */
   /* execution and a negative value if an error occurred.              */
static int SetDiscoverable(void)
{
   int ret_val = 0;

   /* First, check that a valid Bluetooth Stack ID exists.              */
   if(BluetoothStackID)
   {
      /* A semi-valid Bluetooth Stack ID exists, now attempt to set the */
      /* attached Devices Discoverablity Mode to General.               */
      ret_val = GAP_Set_Discoverability_Mode(BluetoothStackID, dmGeneralDiscoverableMode, 0);

      /* Next, check the return value of the GAP Set Discoverability    */
      /* Mode command for successful execution.                         */
      if(ret_val)
      {
         /* An error occurred while trying to set the Discoverability   */
         /* Mode of the Device.                                         */
         DisplayFunctionError("Set Discoverable Mode", ret_val);
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for placing the Local       */
   /* Bluetooth Device into Connectable Mode.  Once in this mode the    */
   /* Device will respond to Page Scans from other Bluetooth Devices.   */
   /* This function requires that a valid Bluetooth Stack ID exists     */
   /* before running.  This function returns zero on success and a      */
   /* negative value if an error occurred.                              */
static int SetConnect(void)
{
   int ret_val = 0;

   /* First, check that a valid Bluetooth Stack ID exists.              */
   if(BluetoothStackID)
   {
      /* Attempt to set the attached Device to be Connectable.          */
      ret_val = GAP_Set_Connectability_Mode(BluetoothStackID, cmConnectableMode);

      /* Next, check the return value of the                            */
      /* GAP_Set_Connectability_Mode() function for successful          */
      /* execution.                                                     */
      if(ret_val)
      {
         /* An error occurred while trying to make the Device           */
         /* Connectable.                                                */
         DisplayFunctionError("Set Connectability Mode", ret_val);
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is a utility function that exists to delete*/
   /* the specified Link Key from the Local Bluetooth Device.  If a NULL*/
   /* Bluetooth Device Address is specified, then all Link Keys will be */
   /* deleted.                                                          */
static int DeleteLinkKey(BD_ADDR_t BD_ADDR)
{
   int       Result;
   Byte_t    Status_Result;
   Word_t    Num_Keys_Deleted = 0;
   BD_ADDR_t NULL_BD_ADDR;

   Result = HCI_Delete_Stored_Link_Key(BluetoothStackID, BD_ADDR, TRUE, &Status_Result, &Num_Keys_Deleted);

   /* Any stored link keys for the specified address (or all) have been */
   /* deleted from the chip.  Now, let's make sure that our stored Link */
   /* Key Array is in sync with these changes.                          */

   /* First check to see all Link Keys were deleted.                    */
   ASSIGN_BD_ADDR(NULL_BD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

   if(COMPARE_BD_ADDR(BD_ADDR, NULL_BD_ADDR))
      BTPS_MemInitialize(LinkKeyInfo, 0, sizeof(LinkKeyInfo));
   else
   {
      /* Individual Link Key.  Go ahead and see if know about the entry */
      /* in the list.                                                   */
      for(Result=0;(Result<sizeof(LinkKeyInfo)/sizeof(LinkKeyInfo_t));Result++)
      {
         if(COMPARE_BD_ADDR(BD_ADDR, LinkKeyInfo[Result].BD_ADDR))
         {
            LinkKeyInfo[Result].BD_ADDR = NULL_BD_ADDR;

            break;
         }
      }
   }

   return(Result);
}

   /* The following function is responsible for setting the name of the */
   /* local Bluetooth Device to a specified name.  This function returns*/
   /* zero on successful execution and a negative value on all errors.  */
static int SetLocalName(char* name)
{
	int Result;
	int ret_val = 0;

	/* First, check that valid Bluetooth Stack ID exists.                */
	if(BluetoothStackID)
	{
		/* Attempt to submit the command.                              */
		Result = GAP_Set_Local_Device_Name(BluetoothStackID, name);

		/* Check the return value of the submitted command for success.*/
		if(!Result)
		{
			/* Display a messsage indicating that the Device Name was   */
			/* successfully submitted.                                  */
			Display(("Local Device Name: %s.\r\n", name));

			/* Flag success to the caller.                              */
			ret_val = 0;
		}
		else
		{
			/* Display a message indicating that an error occured while */
			/* attempting to set the local Device Name.                 */
			DisplayFunctionError("GAP_Set_Local_Device_Name", Result);

			ret_val = FUNCTION_ERROR;
		}
	}
	else
	{
		/* No valid Bluetooth Stack ID exists.                            */
		ret_val = INVALID_STACK_ID_ERROR;
	}

	return ret_val;
}

   /* The following thread is responsible for checking changing the     */
   /* current Baud Rate used to talk to the Radio.                      */
   /* * NOTE * This function ONLY configures the Baud Rate for a TI     */
   /*          Bluetooth chipset.                                       */
static int SetBaudRate(ParameterList_t *TempParam)
{
   int                              ret_val;
   Byte_t                           Length;
   Byte_t                           Status;
   NonAlignedDWord_t                _BaudRate;

   union
   {
      Byte_t                        Buffer[16];
      HCI_Driver_Reconfigure_Data_t DriverReconfigureData;
   } Data;

   /* First check to see if the parameters required for the execution of*/
   /* this function appear to be semi-valid.                            */
   if(BluetoothStackID)
   {
      /* Next check to see if the parameters required for the execution */
      /* of this function appear to be semi-valid.                      */
      if((TempParam) && (TempParam->NumberofParameters > 0))
      {
         /* Verify that this is a valid taable index.                   */
         if(TempParam->Params[0].intParam)
         {
            /* Write the Baud Rate.                                     */
            ASSIGN_HOST_DWORD_TO_LITTLE_ENDIAN_UNALIGNED_DWORD(&_BaudRate, (TempParam->Params[0].intParam));

            /* Next, write the command to the device.                   */
            Length  = sizeof(Data.Buffer);
            ret_val = HCI_Send_Raw_Command(BluetoothStackID, 0x3F, 0x0336, sizeof(NonAlignedDWord_t), (Byte_t *)&_BaudRate, &Status, &Length, Data.Buffer, TRUE);
            if((!ret_val) && (!Status))
            {
               /* We were successful, now we need to change the baud    */
               /* rate of the driver.                                   */
               BTPS_MemInitialize(&(Data.DriverReconfigureData), 0, sizeof(HCI_Driver_Reconfigure_Data_t));

               Data.DriverReconfigureData.ReconfigureCommand = HCI_COMM_DRIVER_RECONFIGURE_DATA_COMMAND_CHANGE_PARAMETERS;
               Data.DriverReconfigureData.ReconfigureData    = (void *)&(TempParam->Params[0].intParam);

               ret_val = HCI_Reconfigure_Driver(BluetoothStackID, FALSE, &(Data.DriverReconfigureData));

               if(ret_val >= 0)
               {
                  Display(("HCI_Reconfigure_Driver(%lu): Success.\r\n", (TempParam->Params[0].intParam)));

                  /* Flag success.                                      */
                  ret_val = 0;
               }
               else
               {
                  Display(("HCI_Reconfigure_Driver(%lu): Failure %d.\r\n", (TempParam->Params[0].intParam), ret_val));

                  ret_val = FUNCTION_ERROR;
               }
            }
            else
            {
               /* Unable to write vendor specific command to chipset.   */
               Display(("HCI_Send_Raw_Command(%lu): Failure %d, %d.\r\n", (TempParam->Params[0].intParam), ret_val, Status));

               ret_val = FUNCTION_ERROR;
            }
         }
         else
         {
            DisplayUsage("SetBaudRate [BaudRate]");

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         DisplayUsage("SetBaudRate [BaudRate]");

         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      /* One or more of the necessary parameters are invalid.           */
      ret_val = INVALID_PARAMETERS_ERROR;
   }

   return(ret_val);
}

/*********************************************************************/
/*                         Event Callbacks                           */
/*********************************************************************/

static void BTPSAPI L2CAP_Event_Callback(unsigned int BluetoothStackID, L2CA_Event_Data_t *L2CA_Event_Data, unsigned long CallbackParameter)
{
	int retval;
	L2CA_Config_Request_t ConfigRequest;
	L2CA_Config_Response_t ConfigResponse;

	switch(L2CA_Event_Data->L2CA_Event_Type)
	{
	case etConnect_Indication:
		Display(("L2CAP: Received connection request\r\n"));
		// accept connection
		retval = L2CA_Connect_Response(BluetoothStackID,
				L2CA_Event_Data->Event_Data.L2CA_Connect_Indication->BD_ADDR,
				L2CA_Event_Data->Event_Data.L2CA_Connect_Indication->Identifier,
				L2CA_Event_Data->Event_Data.L2CA_Connect_Indication->LCID,
				L2CAP_CONNECT_RESPONSE_RESPONSE_SUCCESSFUL,
				0);

		if(!retval)
		{
			/* Connect Response was issued successfully, so let's    */
			/* clear the Config Request and fill the appropriate    */
			/* fields.                                               */
			memset(&ConfigRequest, 0, sizeof(L2CA_Config_Request_t));

			/* Set the desired MTU.  This will tell the remote device*/
			/* what the Maximum packet size that are capable if      */
			/* receiving.                                            */
			ConfigRequest.Option_Flags = L2CA_CONFIG_OPTION_FLAG_MTU;
			ConfigRequest.InMTU        = (Word_t)50;

			/* Send the Config Request to the Remote Device.         */
			retval = L2CA_Config_Request(BluetoothStackID, L2CA_Event_Data->Event_Data.L2CA_Connect_Indication->LCID, L2CAP_LINK_TIMEOUT_MAXIMUM_VALUE, &ConfigRequest);
			if(retval)
			{
				/* Config Request Error, so let's issue an error to   */
				/* the user and Delete the CID that we have already   */
				/* added to the List Box.                             */
				Display(("     Config Request: Function Error %d.\r\n", retval));
			}
		}
		else
		{
			Display(("Error occurred on Line %d, File %s\r\n", __LINE__, __FILE__));
		}
		break;

	case etConnect_Confirmation:
		Display(("L2CAP: Connect confirmation\r\n"));
		break;

	case etDisconnect_Indication:
		Display(("L2CAP: Disconnect indication\r\n"));

		L2CA_Disconnect_Response(BluetoothStackID, L2CA_Event_Data->Event_Data.L2CA_Disconnect_Indication->LCID);
		break;

	case etDisconnect_Confirmation:
		Display(("L2CAP: Successfully disconnected\r\n"));
		break;

	case etConfig_Indication:
		Display(("L2CAP: Config indication\r\n"));

		memset(&ConfigResponse, 0, sizeof(L2CA_Config_Response_t));

		if(L2CA_Event_Data->Event_Data.L2CA_Config_Indication->Option_Flags & L2CA_CONFIG_OPTION_FLAG_MTU)
		{
			ConfigResponse.Option_Flags |= L2CA_CONFIG_OPTION_FLAG_MTU;
		    ConfigResponse.OutMTU = L2CA_Event_Data->Event_Data.L2CA_Config_Indication->OutMTU;
		}

		retval = L2CA_Config_Response(BluetoothStackID,
				L2CA_Event_Data->Event_Data.L2CA_Config_Indication->LCID,
				L2CAP_CONFIGURE_RESPONSE_RESULT_SUCCESS,
				&ConfigResponse);
		if(retval)
		{
			Display(("Error occurred on Line %d, File %s\r\n", __LINE__, __FILE__));
		}

		break;

	case etConfig_Confirmation:
		Display(("L2CAP: Config confirmation\r\n"));
		break;

	case etData_Indication:
		Display(("L2CAP: Received data, length %d\r\n", L2CA_Event_Data->Event_Data.L2CA_Data_Indication->Data_Length));

		protocol(L2CA_Event_Data->Event_Data.L2CA_Data_Indication->Variable_Data,
				L2CA_Event_Data->Event_Data.L2CA_Data_Indication->Data_Length,
				L2CA_Event_Data->Event_Data.L2CA_Data_Indication->CID,
				BluetoothStackID);

		break;

	case etData_Error_Indication:
		Display(("L2CAP: Received data error indication\r\n"));
		break;

	case etTimeout_Indication:
		Display(("L2CAP: Timeout\r\n"));
		break;

	default:
		Display(("L2CAP: Received something\r\n"));
		break;
	}
}

   /* The following function is responsible for processing HCI Mode     */
   /* change events.                                                    */
static void BTPSAPI HCI_Event_Callback(unsigned int BluetoothStackID, HCI_Event_Data_t *HCI_Event_Data, unsigned long CallbackParameter)
{
   char *Mode;

   /* Make sure that the input parameters that were passed to us are    */
   /* semi-valid.                                                       */
   if((BluetoothStackID) && (HCI_Event_Data))
   {
      /* Process the Event Data.                                        */
      switch(HCI_Event_Data->Event_Data_Type)
      {
         case etMode_Change_Event:
            if(HCI_Event_Data->Event_Data.HCI_Mode_Change_Event_Data)
            {
               switch(HCI_Event_Data->Event_Data.HCI_Mode_Change_Event_Data->Current_Mode)
               {
                  case HCI_CURRENT_MODE_HOLD_MODE:
                     Mode = "Hold";
                     break;
                  case HCI_CURRENT_MODE_SNIFF_MODE:
                     Mode = "Sniff";
                     break;
                  case HCI_CURRENT_MODE_PARK_MODE:
                     Mode = "Park";
                     break;
                  case HCI_CURRENT_MODE_ACTIVE_MODE:
                  default:
                     Mode = "Active";
                     break;
               }

               Display(("\r\n"));
               Display(("HCI Mode Change Event, Status: 0x%02X, Connection Handle: %d, Mode: %s, Interval: %d\r\n", HCI_Event_Data->Event_Data.HCI_Mode_Change_Event_Data->Status,
                                                                                                                    HCI_Event_Data->Event_Data.HCI_Mode_Change_Event_Data->Connection_Handle,
                                                                                                                    Mode,
                                                                                                                    HCI_Event_Data->Event_Data.HCI_Mode_Change_Event_Data->Interval));
               DisplayPrompt();
            }
            break;
      }
   }
}

   /* The following function is used to initialize the application      */
   /* instance.  This function should open the stack and prepare to     */
   /* execute commands based on user input.  The first parameter passed */
   /* to this function is the HCI Driver Information that will be used  */
   /* when opening the stack and the second parameter is used to pass   */
   /* parameters to BTPS_Init.  This function returns the               */
   /* BluetoothStackID returned from BSC_Initialize on success or a     */
   /* negative error code (of the form APPLICATION_ERROR_XXX).          */
int InitializeApplication(HCI_DriverInformation_t *HCI_DriverInformation, BTPS_Initialization_t *BTPS_Initialization)
{
	int ret_val = APPLICATION_ERROR_UNABLE_TO_OPEN_STACK;

	/* Initiailize some defaults.                                        */
	SerialPortID           = 0;
	UI_Mode                = UI_MODE_SELECT;
	LoopbackActive         = FALSE;
	DisplayRawData         = FALSE;
	AutomaticReadActive    = FALSE;
	NumberofValidResponses = 0;

	/* Next, makes sure that the Driver Information passed appears to be */
	/* semi-valid.                                                       */
	if((HCI_DriverInformation) && (BTPS_Initialization))
	{
		/* Try to Open the stack and check if it was successful.          */
		if(!OpenStack(HCI_DriverInformation, BTPS_Initialization))
		{
			/* The stack was opened successfully.  Now set some defaults.  */

			/* First, attempt to set the Device to be Connectable.         */
			ret_val = SetConnect();

			/* Next, check to see if the Device was successfully made      */
			/* Connectable.                                                */
			if(!ret_val)
			{
				/* Now that the device is Connectable attempt to make it    */
				/* Discoverable.                                            */
				ret_val = SetDiscoverable();

				/* Next, check to see if the Device was successfully made   */
				/* Discoverable.                                            */
				if(!ret_val)
				{
					/* Attempt to register a HCI Event Callback.          */
					ret_val = HCI_Register_Event_Callback(BluetoothStackID, HCI_Event_Callback, (unsigned long)NULL);
					if(ret_val > 0)
					{
						SetLocalName("Stone BT");

						// NOW WE SHOULD INITIALIZE ALL L2CAP STUFF
						L2CA_Register_PSM(BluetoothStackID, 0x1001, L2CAP_Event_Callback, (unsigned long)NULL);

						/* Return success to the caller.                   */
						ret_val = (int)BluetoothStackID;
					}
					else
						DisplayFunctionError("HCI_Register_Event_Callback()", ret_val);
				}
				else
					DisplayFunctionError("SetDisc", ret_val);
			}
			else
				DisplayFunctionError("SetDisc", ret_val);

			/* In some error occurred then close the stack.                */
			if(ret_val < 0)
			{
				/* Close the Bluetooth Stack.                               */
				CloseStack();
			}
		}
		else
		{
			/* There was an error while attempting to open the Stack.      */
			Display(("Unable to open the stack.\r\n"));
		}
	}
	else
		ret_val = APPLICATION_ERROR_INVALID_PARAMETERS;

	return ret_val;
}
