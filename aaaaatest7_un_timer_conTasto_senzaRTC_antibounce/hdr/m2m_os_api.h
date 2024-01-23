/* $version: 201929  */ 


#ifndef   __M2M_OS_API_H
#define   __M2M_OS_API_H


#include "m2mb_os_api.h"

/* ======================================================================= */
/* Sends AT command to the modem. */
INT32 m2m_os_iat_send_at_command(CHAR *atCmd, UINT16 logPort);

/* Sends AT data to the modem. */
INT32 m2m_os_iat_send_atdata_command(CHAR *atCmd, INT32 atCmdLength, UINT16 logPort);

/* Links AT application logPort and AT command parser instance . */
INT32 m2m_os_iat_set_at_command_instance(UINT16 logPort, UINT16 atInstance);

#if 0
/* Gets the M2M SW version. */
CHAR *m2m_os_get_version(void);

/* Sets the M2M SW version (application version), cannot be larger than M2M_OS_MAX_SW_VERSION_STR_LENGTH. */
INT32 m2m_os_set_version(CHAR *sw_version);

/* Gets the current task (process) id (the one being executed on). */
UINT8 m2m_os_get_current_task_id(void);

/* Sends trace and user messages on the USIF1 serial port. */
void m2m_os_trace_out (char *msg);


#if defined(AZ_PPP_ASC_BRIDGE)
/*  . */
INT32 m2m_os_iat_set_csd_data_on( INT32 hwPort);
#endif

/* Sends a message to one of the user processes (1-32). */
INT32 m2m_os_send_message_to_task( INT8 procId, INT32 type, INT32 param1, INT32 param2);

/* Puts the current process to sleep. */
void m2m_os_sleep_ms(UINT32 ms);

/* Retreives the system tick.  1 tick = 10 ms. */
INT32 m2m_os_retrieve_clock(void);

/* get enqueued messagges on user process with id=procId */
INT32 m2m_os_task_get_enqueued_msg(INT8 procId);

/* ======================================================================= */


/* Dynamic memory pool space reservation. */
INT32 m2m_os_mem_pool(UINT32 pool_size);

/* Dynamic memory allocation. */
void *m2m_os_mem_alloc(UINT32 size);

/* Dynamic memory reallocation */
void *m2m_os_mem_realloc (void *ptr, UINT32 size);

/* Free an already allocated memory. */
void m2m_os_mem_free(void *mem);

/* Gets dynamic memory pool space (HEAP) information. */
UINT32 m2m_os_get_mem_info ( UINT32* pool_frags );



/* Reset the entire system (module). */
void m2m_os_sys_reset(INT32 id);


/* ======================================================================= */


/* Gets the module manufacturer. */
void m2m_info_get_manufacturer(CHAR *buf);

/* Gets the module model. */
void m2m_info_get_model(CHAR *buf);

/* Gets the module serial number. */
void m2m_info_get_serial_num(CHAR *buf);

/* Gets the module factory SN. */
void m2m_info_get_factory_SN(CHAR *buf);

/* Gets the module software version. */
void m2m_info_get_sw_version(CHAR *buf);

/* Gets the module firmware version. */
void m2m_info_get_fw_version(CHAR *buf);

/* Gets the module IMSI. */
void m2m_info_get_IMSI(CHAR *buf);

/* Gets the module MSISDN. */
void m2m_info_get_MSISDN(CHAR *buf);

typedef INT32 (*M2M_CB_MSG_PROC)(INT32, INT32, INT32);

typedef enum
{
  M2M_OS_TASK_STACK_S,    /* 2K */
  M2M_OS_TASK_STACK_M,    /* 4K */
  M2M_OS_TASK_STACK_L,    /* 8K */
  M2M_OS_TASK_STACK_XL,   /* 16K */
  
  M2M_OS_TASK_STACK_LIMIT
  
} M2M_OS_TASK_STACK_SIZE;

#define M2M_OS_TASK_PRIORITY_MAX   1
#define M2M_OS_TASK_PRIORITY_MIN  32

typedef enum
{
  M2M_OS_TASK_MBOX_S,
  M2M_OS_TASK_MBOX_M,
  M2M_OS_TASK_MBOX_L,
  
  M2M_OS_TASK_MBOX_LIMIT
  
} M2M_OS_TASK_MBOX_SIZE;

/* Create a user task, returns process id */
INT32 m2m_os_create_task ( M2M_OS_TASK_STACK_SIZE stackSize, UINT8 priority, 
                           M2M_OS_TASK_MBOX_SIZE mboxSize, M2M_CB_MSG_PROC msg_cb );

/* change dinamically the priority of the already created task */
INT32 m2m_os_change_task_priority ( INT8 procId, UINT16 new_priority, UINT16 *old_priority );

/* change dinamically the preemption threshold of the already created task */
INT32 m2m_os_change_task_preemption ( INT8 procId, UINT16 new_preempt, UINT16 *old_preempt );

/* Delete the user process id task */
INT32 m2m_os_destroy_task ( INT8 procId );

/* Relinquish process control to other ready-to-tun tasks ats the same or higher priority */
void m2m_os_cooperate_task ( void );

/* M2M_main argc parameter write to internal parameter table */
INT8 m2m_os_set_argc ( INT8 argc );

/* M2M_main argc parameter read from internal parameter table */
INT8 m2m_os_get_argc ( void );

/* M2M_main argv[index] parameter write to internal parameter table */
INT8 m2m_os_set_argv ( UINT8 index, CHAR* arg );

/* M2M_main argv[index] parameter read frome internal parameter table */
CHAR* m2m_os_get_argv ( UINT8 index );
#endif
#endif  /*  __M2M_OS_API_H */

