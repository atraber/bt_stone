#ifndef L2CAPSERVER_H_
#define L2CAPSERVER_H_

#include "BTPSKRNL.h"

   /* The following is used as a printf replacement.                    */
#define Display(_x)                 do { BTPS_OutputMessage _x; } while(0)

#define LOG_ERROR(_x)	Display(_x)
#define LOG_DEBUG(_x)	Display(_x)
#define LOG_INFO(_x)	Display(_x)

#endif /* L2CAPSERVER_H_ */
