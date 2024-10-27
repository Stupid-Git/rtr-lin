/* generated thread source file - do not edit */
#include "ftp_thread.h"

TX_THREAD ftp_thread;
void ftp_thread_create(void);
static void ftp_thread_func(ULONG thread_input);
static uint8_t ftp_thread_stack[4000] BSP_PLACE_IN_SECTION_V2(".stack.ftp_thread") BSP_ALIGN_VARIABLE_V2(BSP_STACK_ALIGNMENT);
void tx_startup_err_callback(void *p_instance, void *p_data);
void tx_startup_common_init(void);
NX_FTP_CLIENT g_ftp_client0;
#if defined(__ICCARM__)
#define g_ftp_client0_err_callback_WEAK_ATTRIBUTE
#pragma weak g_ftp_client0_err_callback  = g_ftp_client0_err_callback_internal
#elif defined(__GNUC__)
#define g_ftp_client0_err_callback_WEAK_ATTRIBUTE   __attribute__ ((weak, alias("g_ftp_client0_err_callback_internal")))
#endif
void g_ftp_client0_err_callback(void *p_instance, void *p_data)
g_ftp_client0_err_callback_WEAK_ATTRIBUTE;
/*******************************************************************************************************************//**
 * @brief      This is a weak example initialization error function.  It should be overridden by defining a user  function
 *             with the prototype below.
 *             - void g_ftp_client0_err_callback(void * p_instance, void * p_data)
 *
 * @param[in]  p_instance arguments used to identify which instance caused the error and p_data Callback arguments used to identify what error caused the callback.
 **********************************************************************************************************************/
void g_ftp_client0_err_callback_internal(void *p_instance, void *p_data);
void g_ftp_client0_err_callback_internal(void *p_instance, void *p_data)
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
 *            - void ftp_client_init0(void)
 **********************************************************************************************************************/
void ftp_client_init0(void)
{
    UINT g_ftp_client0_err;
    /* Create FTP Client. */
    g_ftp_client0_err = nx_ftp_client_create (&g_ftp_client0, "g_ftp_client0 FTP Client", &g_ip0, 2048,
                                              &g_packet_pool0);
    if (NX_SUCCESS != g_ftp_client0_err)
    {
        g_ftp_client0_err_callback ((void *) &g_ftp_client0, &g_ftp_client0_err);
    }
}
TX_EVENT_FLAGS_GROUP event_flags_ftp;
TX_MUTEX mutex_ftp;
extern bool g_ssp_common_initialized;
extern uint32_t g_ssp_common_thread_count;
extern TX_SEMAPHORE g_ssp_common_initialized_semaphore;

void ftp_thread_create(void)
{
    /* Increment count so we will know the number of ISDE created threads. */
    g_ssp_common_thread_count++;

    /* Initialize each kernel object. */
    UINT err_event_flags_ftp;
    err_event_flags_ftp = tx_event_flags_create (&event_flags_ftp, (CHAR *) "FTP Event Flags");
    if (TX_SUCCESS != err_event_flags_ftp)
    {
        tx_startup_err_callback (&event_flags_ftp, 0);
    }
    UINT err_mutex_ftp;
    err_mutex_ftp = tx_mutex_create (&mutex_ftp, (CHAR *) "Ftp Mutex", TX_NO_INHERIT);
    if (TX_SUCCESS != err_mutex_ftp)
    {
        tx_startup_err_callback (&mutex_ftp, 0);
    }

    UINT err;
    err = tx_thread_create (&ftp_thread, (CHAR *) "FTP Thread", ftp_thread_func, (ULONG) NULL, &ftp_thread_stack, 4000,
                            6, 6, 1, TX_DONT_START);
    if (TX_SUCCESS != err)
    {
        tx_startup_err_callback (&ftp_thread, 0);
    }
}

static void ftp_thread_func(ULONG thread_input)
{
    /* Not currently using thread_input. */
    SSP_PARAMETER_NOT_USED (thread_input);

    /* Initialize common components */
    tx_startup_common_init ();

    /* Initialize each module instance. */
    /**  Call initialization function if user has selected to do so */
#if (1)
    ftp_client_init0 ();
#endif

    /* Enter user code for this thread. */
    ftp_thread_entry ();
}
