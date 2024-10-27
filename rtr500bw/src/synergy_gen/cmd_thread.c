/* generated thread source file - do not edit */
#include "cmd_thread.h"

TX_THREAD cmd_thread;
void cmd_thread_create(void);
static void cmd_thread_func(ULONG thread_input);
static uint8_t cmd_thread_stack[8192] BSP_PLACE_IN_SECTION_V2(".stack.cmd_thread") BSP_ALIGN_VARIABLE_V2(BSP_STACK_ALIGNMENT);
void tx_startup_err_callback(void *p_instance, void *p_data);
void tx_startup_common_init(void);
TX_EVENT_FLAGS_GROUP g_command_event_flags;
TX_MUTEX mutex_cmd;
TX_MUTEX mutex_tcp_cmd;
TX_QUEUE cmd_queue;
static uint8_t queue_memory_cmd_queue[16];
extern bool g_ssp_common_initialized;
extern uint32_t g_ssp_common_thread_count;
extern TX_SEMAPHORE g_ssp_common_initialized_semaphore;

void cmd_thread_create(void)
{
    /* Increment count so we will know the number of ISDE created threads. */
    g_ssp_common_thread_count++;

    /* Initialize each kernel object. */
    UINT err_g_command_event_flags;
    err_g_command_event_flags = tx_event_flags_create (&g_command_event_flags, (CHAR *) "Command Event Flags");
    if (TX_SUCCESS != err_g_command_event_flags)
    {
        tx_startup_err_callback (&g_command_event_flags, 0);
    }
    UINT err_mutex_cmd;
    err_mutex_cmd = tx_mutex_create (&mutex_cmd, (CHAR *) "Command Task Mutex", TX_NO_INHERIT);
    if (TX_SUCCESS != err_mutex_cmd)
    {
        tx_startup_err_callback (&mutex_cmd, 0);
    }
    UINT err_mutex_tcp_cmd;
    err_mutex_tcp_cmd = tx_mutex_create (&mutex_tcp_cmd, (CHAR *) "TCP Cmd Mutex", TX_NO_INHERIT);
    if (TX_SUCCESS != err_mutex_tcp_cmd)
    {
        tx_startup_err_callback (&mutex_tcp_cmd, 0);
    }
    UINT err_cmd_queue;
    err_cmd_queue = tx_queue_create (&cmd_queue, (CHAR *) "CMD Queue", 4, &queue_memory_cmd_queue,
                                     sizeof(queue_memory_cmd_queue));
    if (TX_SUCCESS != err_cmd_queue)
    {
        tx_startup_err_callback (&cmd_queue, 0);
    }

    UINT err;
    err = tx_thread_create (&cmd_thread, (CHAR *) "Cmd Thread", cmd_thread_func, (ULONG) NULL, &cmd_thread_stack, 8192,
                            5, 5, 1, TX_DONT_START);
    if (TX_SUCCESS != err)
    {
        tx_startup_err_callback (&cmd_thread, 0);
    }
}

static void cmd_thread_func(ULONG thread_input)
{
    /* Not currently using thread_input. */
    SSP_PARAMETER_NOT_USED (thread_input);

    /* Initialize common components */
    tx_startup_common_init ();

    /* Initialize each module instance. */

    /* Enter user code for this thread. */
    cmd_thread_entry ();
}
