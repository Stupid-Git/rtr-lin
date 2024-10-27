/* generated thread source file - do not edit */
#include "tcp_thread.h"

TX_THREAD tcp_thread;
void tcp_thread_create(void);
static void tcp_thread_func(ULONG thread_input);
static uint8_t tcp_thread_stack[2048] BSP_PLACE_IN_SECTION_V2(".stack.tcp_thread") BSP_ALIGN_VARIABLE_V2(BSP_STACK_ALIGNMENT);
void tx_startup_err_callback(void *p_instance, void *p_data);
void tx_startup_common_init(void);
TX_EVENT_FLAGS_GROUP g_tcp_event_flags;
TX_MUTEX mutex_tcp_recv;
extern bool g_ssp_common_initialized;
extern uint32_t g_ssp_common_thread_count;
extern TX_SEMAPHORE g_ssp_common_initialized_semaphore;

void tcp_thread_create(void)
{
    /* Increment count so we will know the number of ISDE created threads. */
    g_ssp_common_thread_count++;

    /* Initialize each kernel object. */
    UINT err_g_tcp_event_flags;
    err_g_tcp_event_flags = tx_event_flags_create (&g_tcp_event_flags, (CHAR *) "TCP Event Flags");
    if (TX_SUCCESS != err_g_tcp_event_flags)
    {
        tx_startup_err_callback (&g_tcp_event_flags, 0);
    }
    UINT err_mutex_tcp_recv;
    err_mutex_tcp_recv = tx_mutex_create (&mutex_tcp_recv, (CHAR *) "Tcp Recv Mutex", TX_NO_INHERIT);
    if (TX_SUCCESS != err_mutex_tcp_recv)
    {
        tx_startup_err_callback (&mutex_tcp_recv, 0);
    }

    UINT err;
    err = tx_thread_create (&tcp_thread, (CHAR *) "Socket Thread", tcp_thread_func, (ULONG) NULL, &tcp_thread_stack,
                            2048, 7, 7, 1, TX_DONT_START);
    if (TX_SUCCESS != err)
    {
        tx_startup_err_callback (&tcp_thread, 0);
    }
}

static void tcp_thread_func(ULONG thread_input)
{
    /* Not currently using thread_input. */
    SSP_PARAMETER_NOT_USED (thread_input);

    /* Initialize common components */
    tx_startup_common_init ();

    /* Initialize each module instance. */

    /* Enter user code for this thread. */
    tcp_thread_entry ();
}
