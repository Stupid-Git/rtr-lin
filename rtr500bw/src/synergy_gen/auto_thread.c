/* generated thread source file - do not edit */
#include "auto_thread.h"

TX_THREAD auto_thread;
void auto_thread_create(void);
static void auto_thread_func(ULONG thread_input);
static uint8_t auto_thread_stack[2048] BSP_PLACE_IN_SECTION_V2(".stack.auto_thread") BSP_ALIGN_VARIABLE_V2(BSP_STACK_ALIGNMENT);
void tx_startup_err_callback(void *p_instance, void *p_data);
void tx_startup_common_init(void);
extern bool g_ssp_common_initialized;
extern uint32_t g_ssp_common_thread_count;
extern TX_SEMAPHORE g_ssp_common_initialized_semaphore;

void auto_thread_create(void)
{
    /* Increment count so we will know the number of ISDE created threads. */
    g_ssp_common_thread_count++;

    /* Initialize each kernel object. */

    UINT err;
    err = tx_thread_create (&auto_thread, (CHAR *) "Auto Thread", auto_thread_func, (ULONG) NULL, &auto_thread_stack,
                            2048, 9, 9, 1, TX_DONT_START);
    if (TX_SUCCESS != err)
    {
        tx_startup_err_callback (&auto_thread, 0);
    }
}

static void auto_thread_func(ULONG thread_input)
{
    /* Not currently using thread_input. */
    SSP_PARAMETER_NOT_USED (thread_input);

    /* Initialize common components */
    tx_startup_common_init ();

    /* Initialize each module instance. */

    /* Enter user code for this thread. */
    auto_thread_entry ();
}
