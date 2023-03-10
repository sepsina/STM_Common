/**************************************************************************************************
  Filename:       test_printf.h
  Revised:
  Revision:
    
  Description:
    

**************************************************************************************************/

#ifndef DBG_PRINTF_H
#define DBG_PRINTF_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include <stdbool.h>
#include <stdint.h>
/*********************************************************************
 * CONSTANTS
 */
//#define TX_BUF_SIZE    256
#define TX_MAX         200
/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * FUNCTIONS
 */

extern void dbgPrintf(char* format,...);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* #ifndef DBG_PRINTF_H */
