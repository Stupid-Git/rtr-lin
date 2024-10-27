/* generated thread source file - do not edit */
#include "led_thread.h"

TX_THREAD led_thread;
void led_thread_create(void);
static void led_thread_func(ULONG thread_input);
static uint8_t led_thread_stack[1024] BSP_PLACE_IN_SECTION_V2(".stack.led_thread") BSP_ALIGN_VARIABLE_V2(BSP_STACK_ALIGNMENT);
void tx_startup_err_callback(void *p_instance, void *p_data);
void tx_startup_common_init(void);
TX_QUEUE led_queue;
static uint8_t queue_memory_led_queue[20];
extern bool g_ssp_common_initialized;
extern uint32_t g_ssp_common_thread_count;
extern TX_SEMAPHORE g_ssp_common_initialized_semaphore;

void led_thread_create(void)
{
    /* Increment count so we will know the number of ISDE created threads. */
    g_ssp_common_thread_count++;

    /* Initialize each kernel object. */
    UINT err_led_queue;
    err_led_queue = tx_queue_create (&led_queue, (CHAR *) "Led Queue", 1, &queue_memory_led_queue,
                                     sizeof(queue_memory_led_queue));
    if (TX_SUCCESS != err_led_queue)
    {
        tx_startup_err_callback (&led_queue, 0);
    }

    UINT err;
    err = tx_thread_create (&led_thread, (CHAR *) "Led Thread", led_thread_func, (ULONG) NULL, &led_thread_stack, 1024,
                            3, 3, 1, TX_DONT_START);
    if (TX_SUCCESS != err)
    {
        tx_startup_err_callback (&led_thread, 0);
    }
}

static void led_thread_func(ULONG thread_input)
{
    /* Not currently using thread_input. */
    SSP_PARAMETER_NOT_USED (thread_input);

    /* Initialize common components */
    tx_startup_common_init ();

    /* Initialize each module instance. */

    /* Enter user code for this thread. */
    led_thread_entry ();
}
