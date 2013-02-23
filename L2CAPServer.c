
#include "Main.h"                /* Application Interface Abstraction.        */
#include "SS1BTPS.h"             /* Main SS1 Bluetooth Stack Header.          */
#include "BTPSKRNL.h"            /* BTPS Kernel Header.                       */
#include "L2CAPServer.h"             /* Application Header.                       */
#include "BTAPITyp.h"


#include "protocol.h"

#include <stdio.h>
#include <string.h>


#define MAX_SUPPORTED_LINK_KEYS                    (1)   /* Max supported Link*/
                                                         /* keys.             */

#define FUNCTION_ERROR                             (-4)  /* Denotes that an   */
                                                         /* error occurred in */
                                                         /* execution of the  */
                                                         /* Command Function. */

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

   /* The following type definition represents the container type which */
   /* holds the mapping between Bluetooth devices (based on the BD_ADDR)*/
   /* and the Link Key (BD_ADDR <-> Link Key Mapping).                  */
typedef struct _tagLinkKeyInfo_t
{
   BD_ADDR_t  BD_ADDR;
   Link_Key_t LinkKey;
} LinkKeyInfo_t;

   /* User to represent a structure to hold a BD_ADDR return from       */
   /* BD_ADDRToStr.                                                     */
typedef char BoardStr_t[15];


   /* Internal Variables to this Module (Remember that all variables    */
   /* declared static are initialized to 0 automatically by the         */
   /* compiler as part of standard C/C++).                              */


static unsigned int        BluetoothStackID;        /* Variable which holds the Handle */
                                                    /* of the opened Bluetooth Protocol*/
                                                    /* Stack.                          */


static LinkKeyInfo_t       LinkKeyInfo[MAX_SUPPORTED_LINK_KEYS]; /* Variable holds     */
                                                    /* BD_ADDR <-> Link Keys for       */
                                                    /* pairing.                        */


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

   /* Internal function prototypes.                                     */
static void BD_ADDRToStr(BD_ADDR_t Board_Address, BoardStr_t BoardStr);
static void DisplayFunctionError(char *Function,int Status);

static int OpenStack(HCI_DriverInformation_t *HCI_DriverInformation, BTPS_Initialization_t *BTPS_Initialization);
static int CloseStack(void);

static int SetConnect(void);
static int DeleteLinkKey(BD_ADDR_t BD_ADDR);

static int SetLocalName(char* name);

static int SetBaudRate(SDWord_t baudrate);

   /* BTPS Callback function prototypes.                                */
static void BTPSAPI L2CAP_Event_Callback(unsigned int BluetoothStackID, L2CA_Event_Data_t *L2CA_Event_Data, unsigned long CallbackParameter);

   /* The following function is responsible for converting data of type */
   /* BD_ADDR to a string.  The first parameter of this function is the */
   /* BD_ADDR to be converted to a string.  The second parameter of this*/
   /* function is a pointer to the string in which the converted BD_ADDR*/
   /* is to be stored.                                                  */
static void BD_ADDRToStr(BD_ADDR_t Board_Address, BoardStr_t BoardStr)
{
   BTPS_SprintF((char *)BoardStr, "0x%02X%02X%02X%02X%02X%02X", Board_Address.BD_ADDR5, Board_Address.BD_ADDR4, Board_Address.BD_ADDR3, Board_Address.BD_ADDR2, Board_Address.BD_ADDR1, Board_Address.BD_ADDR0);
}

   /* Displays a function error.                                        */
static void DisplayFunctionError(char *Function, int Status)
{
   Display(("\n%s Failed: %d.\r\n", Function, Status));
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

	return ret_val;
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

	return ret_val;
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

	return ret_val;
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
static int SetBaudRate(SDWord_t baudrate)
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
	 /* Verify that this is a valid taable index.                   */
	 if(baudrate)
	 {
		/* Write the Baud Rate.                                     */
		ASSIGN_HOST_DWORD_TO_LITTLE_ENDIAN_UNALIGNED_DWORD(&_BaudRate, (baudrate));

		/* Next, write the command to the device.                   */
		Length  = sizeof(Data.Buffer);
		ret_val = HCI_Send_Raw_Command(BluetoothStackID, 0x3F, 0x0336, sizeof(NonAlignedDWord_t), (Byte_t *)&_BaudRate, &Status, &Length, Data.Buffer, TRUE);
		if((!ret_val) && (!Status))
		{
		   /* We were successful, now we need to change the baud    */
		   /* rate of the driver.                                   */
		   BTPS_MemInitialize(&(Data.DriverReconfigureData), 0, sizeof(HCI_Driver_Reconfigure_Data_t));

		   Data.DriverReconfigureData.ReconfigureCommand = HCI_COMM_DRIVER_RECONFIGURE_DATA_COMMAND_CHANGE_PARAMETERS;
		   Data.DriverReconfigureData.ReconfigureData    = (void *)&baudrate;

		   ret_val = HCI_Reconfigure_Driver(BluetoothStackID, FALSE, &(Data.DriverReconfigureData));

		   if(ret_val >= 0)
		   {
			  Display(("HCI_Reconfigure_Driver(%lu): Success.\r\n", baudrate));

			  /* Flag success.                                      */
			  ret_val = 0;
		   }
		   else
		   {
			  Display(("HCI_Reconfigure_Driver(%lu): Failure %d.\r\n", baudrate, ret_val));

			  ret_val = FUNCTION_ERROR;
		   }
		}
		else
		{
		   /* Unable to write vendor specific command to chipset.   */
		   Display(("HCI_Send_Raw_Command(%lu): Failure %d, %d.\r\n", baudrate, ret_val, Status));

		   ret_val = FUNCTION_ERROR;
		}
	 }
	 else
	 {
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
		LOG_INFO(("L2CAP: Received connection request\r\n"));
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
				LOG_ERROR(("     Config Request: Function Error %d.\r\n", retval));
			}

			connectionOpened(BluetoothStackID, L2CA_Event_Data->Event_Data.L2CA_Connect_Indication->LCID);
		}
		else
		{
			LOG_ERROR(("L2CA_Connect_Response failed: Error code %d", retval));
		}
		break;

	case etConnect_Confirmation:
		LOG_INFO(("L2CAP: Connect confirmation\r\n"));
		break;

	case etDisconnect_Indication:
		LOG_INFO(("L2CAP: Disconnect indication\r\n"));

		retval = L2CA_Disconnect_Response(BluetoothStackID, L2CA_Event_Data->Event_Data.L2CA_Disconnect_Indication->LCID);
		if(retval)
		{
			LOG_ERROR(("L2CA_Disconnect_Indication failed: Error code %d", retval));
		}

		connectionClosed();
		break;

	case etDisconnect_Confirmation:
		LOG_INFO(("L2CAP: Successfully disconnected\r\n"));

		break;

	case etConfig_Indication:
		LOG_INFO(("L2CAP: Config indication\r\n"));

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
			LOG_ERROR(("L2CA_Config_Response failed: error code %d", retval));
		}

		break;

	case etConfig_Confirmation:
		LOG_INFO(("L2CAP: Config confirmation\r\n"));
		break;

	case etData_Indication:
		LOG_INFO(("L2CAP: Received data, length %d\r\n", L2CA_Event_Data->Event_Data.L2CA_Data_Indication->Data_Length));

		protocol(BluetoothStackID,
				L2CA_Event_Data->Event_Data.L2CA_Data_Indication->CID,
				L2CA_Event_Data->Event_Data.L2CA_Data_Indication->Variable_Data,
				L2CA_Event_Data->Event_Data.L2CA_Data_Indication->Data_Length);

		break;

	case etData_Error_Indication:
		LOG_DEBUG(("L2CAP: Received data error indication\r\n"));
		break;

	case etTimeout_Indication:
		LOG_DEBUG(("L2CAP: Timeout\r\n"));
		break;

	default:
		LOG_DEBUG(("L2CAP: Received some event which we do not handle\r\n"));
		break;
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
					SetLocalName("Stone BT");

					// NOW WE SHOULD INITIALIZE ALL L2CAP STUFF
					L2CA_Register_PSM(BluetoothStackID, 0x1001, L2CAP_Event_Callback, (unsigned long)NULL);

					/* Return success to the caller.                   */
					ret_val = (int)BluetoothStackID;
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
