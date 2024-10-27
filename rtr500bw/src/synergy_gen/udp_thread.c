/* generated thread source file - do not edit */
#include "udp_thread.h"

TX_THREAD udp_thread;
void udp_thread_create(void);
static void udp_thread_func(ULONG thread_input);
static uint8_t udp_thread_stack[2048] BSP_PLACE_IN_SECTION_V2(".stack.udp_thread") BSP_ALIGN_VARIABLE_V2(BSP_STACK_ALIGNMENT);
void tx_startup_err_callback(void *p_instance, void *p_data);
void tx_startup_common_init(void);
TX_QUEUE g_msg_udp_queue;
static uint8_t queue_memory_g_msg_udp_queue[4];
extern bool g_ssp_common_initialized;
extern uint32_t g_ssp_common_thread_count;
extern TX_SEMAPHORE g_ssp_common_initialized_semaphore;

void udp_thread_create(void)
{
    /* Increment count so we will know the number of ISDE created threads. */
    g_ssp_common_thread_count++;

    /* Initialize each kernel object. */
    UINT err_g_msg_udp_queue;
    err_g_msg_udp_queue = tx_queue_create (&g_msg_udp_queue, (CHAR *) "Udp Msg Queue", 1, &queue_memory_g_msg_udp_queue,
                                           sizeof(queue_memory_g_msg_udp_queue));
    if (TX_SUCCESS != err_g_msg_udp_queue)
    {
        tx_startup_err_callback (&g_msg_udp_queue, 0);
    }

    UINT err;
    err = tx_thread_create (&udp_thread, (CHAR *) "Udp Thread", udp_thread_func, (ULONG) NULL, &udp_thread_stack, 2048,
                            7, 7, 1, TX_DONT_START);
    if (TX_SUCCESS != err)
    {
        tx_startup_err_callback (&udp_thread, 0);
    }
}

static void udp_thread_func(ULONG thread_input)
{
    /* Not currently using thread_input. */
    SSP_PARAMETER_NOT_USED (thread_input);

    /* Initialize common components */
    tx_startup_common_init ();

    /* Initialize each module instance. */

    /* Enter user code for this thread. */
    udp_thread_entry ();
}
