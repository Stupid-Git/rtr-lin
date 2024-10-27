/* generated thread source file - do not edit */
#include "http_thread.h"

TX_THREAD http_thread;
void http_thread_create(void);
static void http_thread_func(ULONG thread_input);
static uint8_t http_thread_stack[16384] BSP_PLACE_IN_SECTION_V2(".stack.http_thread") BSP_ALIGN_VARIABLE_V2(BSP_STACK_ALIGNMENT);
void tx_startup_err_callback(void *p_instance, void *p_data);
void tx_startup_common_init(void);
NX_WEB_HTTP_CLIENT g_web_http_client0;
#if defined(__ICCARM__)
#define g_web_http_client0_err_callback_WEAK_ATTRIBUTE
#pragma weak g_web_http_client0_err_callback  = g_web_http_client0_err_callback_internal
#elif defined(__GNUC__)
#define g_web_http_client0_err_callback_WEAK_ATTRIBUTE   __attribute__ ((weak, alias("g_web_http_client0_err_callback_internal")))
#endif
void g_web_http_client0_err_callback(void *p_instance, void *p_data)
g_web_http_client0_err_callback_WEAK_ATTRIBUTE;
/*******************************************************************************************************************//**
 * @brief      This is a weak example initialization error function.  It should be overridden by defining a user  function
 *             with the prototype below.
 *             - void g_web_http_client0_err_callback(void * p_instance, void * p_data)
 *
 * @param[in]  p_instance arguments used to identify which instance caused the error and p_data Callback arguments used to identify what error caused the callback.
 **********************************************************************************************************************/
void g_web_http_client0_err_callback_internal(void *p_instance, void *p_data);
void g_web_http_client0_err_callback_internal(void *p_instance, void *p_data)
{
    /** Suppress compiler warning for not using parameters. */
    SSP_PARAMETER_NOT_USED (p_instance);
    SSP_PARAMETER_NOT_USED (p_data);

    /** An error has occurred. Please check function arguments for more information. */
    BSP_CFG_HANDLE_UNRECOVERABLE_ERROR (0);
}
/*******************************************************************************************************************//**
 * @brief     Initialization function that the user can choose to have called automatically during thread entry.
 *            The user can call this function at a later time if desired using the prototype below.
 *            - void web_http_client_init0(void)
 **********************************************************************************************************************/
void web_http_client_init0(void)
{
    UINT g_web_http_client0_err;
    /* Create HTTP Client. */
    g_web_http_client0_err = nx_web_http_client_create (&g_web_http_client0, "g_web_http_client0 HTTP Client", &g_ip0,
                                                        &g_packet_pool1, 4096);
    if (NX_SUCCESS != g_web_http_client0_err)
    {
        g_web_http_client0_err_callback ((void *) &g_web_http_client0, &g_web_http_client0_err);
    }
}
TX_EVENT_FLAGS_GROUP event_flags_http;
extern bool g_ssp_common_initialized;
extern uint32_t g_ssp_common_thread_count;
extern TX_SEMAPHORE g_ssp_common_initialized_semaphore;

void http_thread_create(void)
{
    /* Increment count so we will know the number of ISDE created threads. */
    g_ssp_common_thread_count++;

    /* Initialize each kernel object. */
    UINT err_event_flags_http;
    err_event_flags_http = tx_event_flags_create (&event_flags_http, (CHAR *) "Http Event Flags");
    if (TX_SUCCESS != err_event_flags_http)
    {
        tx_startup_err_callback (&event_flags_http, 0);
    }

    UINT err;
    err = tx_thread_create (&http_thread, (CHAR *) "HTTP Thread", http_thread_func, (ULONG) NULL, &http_thread_stack,
                            16384, 6, 6, 1, TX_DONT_START);
    if (TX_SUCCESS != err)
    {
        tx_startup_err_callback (&http_thread, 0);
    }
}

static void http_thread_func(ULONG thread_input)
{
    /* Not currently using thread_input. */
    SSP_PARAMETER_NOT_USED (thread_input);

    /* Initialize common components */
    tx_startup_common_init ();

    /* Initialize each module instance. */
    /** Call initialization function if user has selected to do so. */
#if (1)
    web_http_client_init0 ();
#endif

    /* Enter user code for this thread. */
    http_thread_entry ();
}
