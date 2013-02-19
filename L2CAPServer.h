#ifndef L2CAPSERVER_H_
#define L2CAPSERVER_H_

#include "BTPSKRNL.h"

   /* The following is used as a printf replacement.                    */
#define Display(_x)                 do { BTPS_OutputMessage _x; } while(0)

#define LOG_ERROR(_x)	Display(_x)
#define LOG_DEBUG(_x)	Display(_x)
#define LOG_INFO(_x)	Display(_x)

   /* The following function is used to initialize the application      */
   /* instance.  This function should open the stack and prepare to     */
   /* execute commands based on user input.  The first parameter passed */
   /* to this function is the HCI Driver Information that will be used  */
   /* when opening the stack and the second parameter is used to pass   */
   /* parameters to BTPS_Init.  This function returns the               */
   /* BluetoothStackID returned from BSC_Initialize on success or a     */
   /* negative error code (of the form APPLICATION_ERROR_XXX).          */
int InitializeApplication(HCI_DriverInformation_t *HCI_DriverInformation, BTPS_Initialization_t *BTPS_Initialization);

#endif /* L2CAPSERVER_H_ */
