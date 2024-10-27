/* generated thread source file - do not edit */
#include "usb_thread.h"

TX_THREAD usb_thread;
void usb_thread_create(void);
static void usb_thread_func(ULONG thread_input);
static uint8_t usb_thread_stack[2048] BSP_PLACE_IN_SECTION_V2(".stack.usb_thread") BSP_ALIGN_VARIABLE_V2(BSP_STACK_ALIGNMENT);
void tx_startup_err_callback(void *p_instance, void *p_data);
void tx_startup_common_init(void);
TX_SEMAPHORE g_cdc_activate_semaphore0;
TX_EVENT_FLAGS_GROUP event_flags_usb;
extern bool g_ssp_common_initialized;
extern uint32_t g_ssp_common_thread_count;
extern TX_SEMAPHORE g_ssp_common_initialized_semaphore;

void usb_thread_create(void)
{
    /* Increment count so we will know the number of ISDE created threads. */
    g_ssp_common_thread_count++;

    /* Initialize each kernel object. */
    UINT err_g_cdc_activate_semaphore0;
    err_g_cdc_activate_semaphore0 = tx_semaphore_create (&g_cdc_activate_semaphore0, (CHAR *) "New Semaphore", 0);
    if (TX_SUCCESS != err_g_cdc_activate_semaphore0)
    {
        tx_startup_err_callback (&g_cdc_activate_semaphore0, 0);
    }
    UINT err_event_flags_usb;
    err_event_flags_usb = tx_event_flags_create (&event_flags_usb, (CHAR *) "USB Event Flags");
    if (TX_SUCCESS != err_event_flags_usb)
    {
        tx_startup_err_callback (&event_flags_usb, 0);
    }

    UINT err;
    err = tx_thread_create (&usb_thread, (CHAR *) "USB Thread", usb_thread_func, (ULONG) NULL, &usb_thread_stack, 2048,
                            4, 4, 1, TX_DONT_START);
    if (TX_SUCCESS != err)
    {
        tx_startup_err_callback (&usb_thread, 0);
    }
}

static void usb_thread_func(ULONG thread_input)
{
    /* Not currently using thread_input. */
    SSP_PARAMETER_NOT_USED (thread_input);

    /* Initialize common components */
    tx_startup_common_init ();

    /* Initialize each module instance. */

    /* Enter user code for this thread. */
    usb_thread_entry ();
}
