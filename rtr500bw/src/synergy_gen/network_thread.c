/* generated thread source file - do not edit */
#include "network_thread.h"

TX_THREAD network_thread;
void network_thread_create(void);
static void network_thread_func(ULONG thread_input);
static uint8_t network_thread_stack[2048] BSP_PLACE_IN_SECTION_V2(".stack.network_thread") BSP_ALIGN_VARIABLE_V2(BSP_STACK_ALIGNMENT);
void tx_startup_err_callback(void *p_instance, void *p_data);
void tx_startup_common_init(void);
TX_EVENT_FLAGS_GROUP event_flags_network;
TX_EVENT_FLAGS_GROUP event_flags_net_status;
TX_EVENT_FLAGS_GROUP event_flags_link;
TX_EVENT_FLAGS_GROUP event_flags_wifi;
extern bool g_ssp_common_initialized;
extern uint32_t g_ssp_common_thread_count;
extern TX_SEMAPHORE g_ssp_common_initialized_semaphore;

void network_thread_create(void)
{
    /* Increment count so we will know the number of ISDE created threads. */
    g_ssp_common_thread_count++;

    /* Initialize each kernel object. */
    UINT err_event_flags_network;
    err_event_flags_network = tx_event_flags_create (&event_flags_network, (CHAR *) "Network Event Flags");
    if (TX_SUCCESS != err_event_flags_network)
    {
        tx_startup_err_callback (&event_flags_network, 0);
    }
    UINT err_event_flags_net_status;
    err_event_flags_net_status = tx_event_flags_create (&event_flags_net_status, (CHAR *) "Net Status Event Flags");
    if (TX_SUCCESS != err_event_flags_net_status)
    {
        tx_startup_err_callback (&event_flags_net_status, 0);
    }
    UINT err_event_flags_link;
    err_event_flags_link = tx_event_flags_create (&event_flags_link, (CHAR *) "Link Event Flags");
    if (TX_SUCCESS != err_event_flags_link)
    {
        tx_startup_err_callback (&event_flags_link, 0);
    }
    UINT err_event_flags_wifi;
    err_event_flags_wifi = tx_event_flags_create (&event_flags_wifi, (CHAR *) "Wifi Event Flags");
    if (TX_SUCCESS != err_event_flags_wifi)
    {
        tx_startup_err_callback (&event_flags_wifi, 0);
    }

    UINT err;
    err = tx_thread_create (&network_thread, (CHAR *) "Network Thread", network_thread_func, (ULONG) NULL,
                            &network_thread_stack, 2048, 6, 6, 1, TX_DONT_START);
    if (TX_SUCCESS != err)
    {
        tx_startup_err_callback (&network_thread, 0);
    }
}

static void network_thread_func(ULONG thread_input)
{
    /* Not currently using thread_input. */
    SSP_PARAMETER_NOT_USED (thread_input);

    /* Initialize common components */
    tx_startup_common_init ();

    /* Initialize each module instance. */

    /* Enter user code for this thread. */
    network_thread_entry ();
}
