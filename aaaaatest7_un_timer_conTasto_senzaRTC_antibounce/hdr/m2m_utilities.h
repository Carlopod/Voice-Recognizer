
#ifndef	_M2M_UTILITIES_H
#define _M2M_UTILITIES_H


#include "m2mb_types.h"
#include "azx_utils.h"


#define m2m_os_sleep_ms azx_sleep_ms

/* This file provides utility interfaces available to the M2M application. */

/* PUBLIC functions*/

UINT32 getUptime(void);
UINT32 getHWUptime(void);
int uptimeAsString(UINT32 uptime, char * string);


int waitRegistration(UINT32 timeout);
int waitGPRSRegistration(UINT32 timeout);


/*Returns the module name as a string*/
char * m2m_GetModuleName(void);


UINT16 m2m_get_AcT(void);

UINT32 m2m_parse_IPv4_dotted(const char *s);

INT32 m2m_check_PDP_status_cid(UINT32 timeout, UINT8 cid);

//char *NetEventString(M2M_NETWORK_EVENT event);
#endif  /* _M2M_FS_API_H */

void getSlMacAddress( unsigned char *mac );
