/* generated thread source file - do not edit */
#include "test_thread.h"

TX_THREAD test_thread;
void test_thread_create(void);
static void test_thread_func(ULONG thread_input);
static uint8_t test_thread_stack[1024] BSP_PLACE_IN_SECTION_V2(".stack.test_thread") BSP_ALIGN_VARIABLE_V2(BSP_STACK_ALIGNMENT);
void tx_startup_err_callback(void *p_instance, void *p_data);
void tx_startup_common_init(void);
extern bool g_ssp_common_initialized;
extern uint32_t g_ssp_common_thread_count;
extern TX_SEMAPHORE g_ssp_common_initialized_semaphore;

void test_thread_create(void)
{
    /* Increment count so we will know the number of ISDE created threads. */
    g_ssp_common_thread_count++;

    /* Initialize each kernel object. */

    UINT err;
    err = tx_thread_create (&test_thread, (CHAR *) "Test Thread", test_thread_func, (ULONG) NULL, &test_thread_stack,
                            1024, 10, 10, 1, TX_DONT_START);
    if (TX_SUCCESS != err)
    {
        tx_startup_err_callback (&test_thread, 0);
    }
}

static void test_thread_func(ULONG thread_input)
{
    /* Not currently using thread_input. */
    SSP_PARAMETER_NOT_USED (thread_input);

    /* Initialize common components */
    tx_startup_common_init ();

    /* Initialize each module instance. */

    /* Enter user code for this thread. */
    test_thread_entry ();
}
