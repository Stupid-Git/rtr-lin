/* generated thread source file - do not edit */
#include "sntp_thread.h"

TX_THREAD sntp_thread;
void sntp_thread_create(void);
static void sntp_thread_func(ULONG thread_input);
static uint8_t sntp_thread_stack[2048] BSP_PLACE_IN_SECTION_V2(".stack.sntp_thread") BSP_ALIGN_VARIABLE_V2(BSP_STACK_ALIGNMENT);
void tx_startup_err_callback(void *p_instance, void *p_data);
void tx_startup_common_init(void);
TX_EVENT_FLAGS_GROUP event_flags_sntp;
extern bool g_ssp_common_initialized;
extern uint32_t g_ssp_common_thread_count;
extern TX_SEMAPHORE g_ssp_common_initialized_semaphore;

void sntp_thread_create(void)
{
    /* Increment count so we will know the number of ISDE created threads. */
    g_ssp_common_thread_count++;

    /* Initialize each kernel object. */
    UINT err_event_flags_sntp;
    err_event_flags_sntp = tx_event_flags_create (&event_flags_sntp, (CHAR *) "SNTP Event Flags");
    if (TX_SUCCESS != err_event_flags_sntp)
    {
        tx_startup_err_callback (&event_flags_sntp, 0);
    }

    UINT err;
    err = tx_thread_create (&sntp_thread, (CHAR *) "SNTP Thread", sntp_thread_func, (ULONG) NULL, &sntp_thread_stack,
                            2048, 8, 8, 1, TX_DONT_START);
    if (TX_SUCCESS != err)
    {
        tx_startup_err_callback (&sntp_thread, 0);
    }
}

static void sntp_thread_func(ULONG thread_input)
{
    /* Not currently using thread_input. */
    SSP_PARAMETER_NOT_USED (thread_input);

    /* Initialize common components */
    tx_startup_common_init ();

    /* Initialize each module instance. */

    /* Enter user code for this thread. */
    sntp_thread_entry ();
}
