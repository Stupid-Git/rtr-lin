/* generated thread source file - do not edit */
#include "event_thread.h"

TX_THREAD event_thread;
void event_thread_create(void);
static void event_thread_func(ULONG thread_input);
static uint8_t event_thread_stack[1024] BSP_PLACE_IN_SECTION_V2(".stack.event_thread") BSP_ALIGN_VARIABLE_V2(BSP_STACK_ALIGNMENT);
void tx_startup_err_callback(void *p_instance, void *p_data);
void tx_startup_common_init(void);
TX_EVENT_FLAGS_GROUP event_flags_cycle;
extern bool g_ssp_common_initialized;
extern uint32_t g_ssp_common_thread_count;
extern TX_SEMAPHORE g_ssp_common_initialized_semaphore;

void event_thread_create(void)
{
    /* Increment count so we will know the number of ISDE created threads. */
    g_ssp_common_thread_count++;

    /* Initialize each kernel object. */
    UINT err_event_flags_cycle;
    err_event_flags_cycle = tx_event_flags_create (&event_flags_cycle, (CHAR *) "Event Control  Event Flags");
    if (TX_SUCCESS != err_event_flags_cycle)
    {
        tx_startup_err_callback (&event_flags_cycle, 0);
    }

    UINT err;
    err = tx_thread_create (&event_thread, (CHAR *) "Event Thread", event_thread_func, (ULONG) NULL,
                            &event_thread_stack, 1024, 7, 7, 1, TX_DONT_START);
    if (TX_SUCCESS != err)
    {
        tx_startup_err_callback (&event_thread, 0);
    }
}

static void event_thread_func(ULONG thread_input)
{
    /* Not currently using thread_input. */
    SSP_PARAMETER_NOT_USED (thread_input);

    /* Initialize common components */
    tx_startup_common_init ();

    /* Initialize each module instance. */

    /* Enter user code for this thread. */
    event_thread_entry ();
}
