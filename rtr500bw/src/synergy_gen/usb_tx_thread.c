/* generated thread source file - do not edit */
#include "usb_tx_thread.h"

TX_THREAD usb_tx_thread;
void usb_tx_thread_create(void);
static void usb_tx_thread_func(ULONG thread_input);
static uint8_t usb_tx_thread_stack[1024] BSP_PLACE_IN_SECTION_V2(".stack.usb_tx_thread") BSP_ALIGN_VARIABLE_V2(BSP_STACK_ALIGNMENT);
void tx_startup_err_callback(void *p_instance, void *p_data);
void tx_startup_common_init(void);
TX_QUEUE g_usb_tx_queue;
static uint8_t queue_memory_g_usb_tx_queue[16];
extern bool g_ssp_common_initialized;
extern uint32_t g_ssp_common_thread_count;
extern TX_SEMAPHORE g_ssp_common_initialized_semaphore;

void usb_tx_thread_create(void)
{
    /* Increment count so we will know the number of ISDE created threads. */
    g_ssp_common_thread_count++;

    /* Initialize each kernel object. */
    UINT err_g_usb_tx_queue;
    err_g_usb_tx_queue = tx_queue_create (&g_usb_tx_queue, (CHAR *) "USB TX Queue", 2, &queue_memory_g_usb_tx_queue,
                                          sizeof(queue_memory_g_usb_tx_queue));
    if (TX_SUCCESS != err_g_usb_tx_queue)
    {
        tx_startup_err_callback (&g_usb_tx_queue, 0);
    }

    UINT err;
    err = tx_thread_create (&usb_tx_thread, (CHAR *) "USB TX Thread", usb_tx_thread_func, (ULONG) NULL,
                            &usb_tx_thread_stack, 1024, 4, 4, 1, TX_AUTO_START);
    if (TX_SUCCESS != err)
    {
        tx_startup_err_callback (&usb_tx_thread, 0);
    }
}

static void usb_tx_thread_func(ULONG thread_input)
{
    /* Not currently using thread_input. */
    SSP_PARAMETER_NOT_USED (thread_input);

    /* Initialize common components */
    tx_startup_common_init ();

    /* Initialize each module instance. */

    /* Enter user code for this thread. */
    usb_tx_thread_entry ();
}
