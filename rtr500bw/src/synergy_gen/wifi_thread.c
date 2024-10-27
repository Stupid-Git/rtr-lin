/* generated thread source file - do not edit */
#include "wifi_thread.h"

TX_THREAD wifi_thread;
void wifi_thread_create(void);
static void wifi_thread_func(ULONG thread_input);
static uint8_t wifi_thread_stack[8000] BSP_PLACE_IN_SECTION_V2(".stack.wifi_thread") BSP_ALIGN_VARIABLE_V2(BSP_STACK_ALIGNMENT);
void tx_startup_err_callback(void *p_instance, void *p_data);
void tx_startup_common_init(void);
NX_SNTP_CLIENT g_sntp0;
#if defined(__ICCARM__)
#define g_sntp0_err_callback_WEAK_ATTRIBUTE
#pragma weak g_sntp0_err_callback  = g_sntp0_err_callback_internal
#elif defined(__GNUC__)
#define g_sntp0_err_callback_WEAK_ATTRIBUTE   __attribute__ ((weak, alias("g_sntp0_err_callback_internal")))
#endif
void g_sntp0_err_callback(void *p_instance, void *p_data)
g_sntp0_err_callback_WEAK_ATTRIBUTE;
/*******************************************************************************************************************//**
 * @brief      This is a weak example initialization error function.  It should be overridden by defining a user  function
 *             with the prototype below.
 *             - void g_sntp0_err_callback(void * p_instance, void * p_data)
 *
 * @param[in]  p_instance arguments used to identify which instance caused the error and p_data Callback arguments used to identify what error caused the callback.
 **********************************************************************************************************************/
void g_sntp0_err_callback_internal(void *p_instance, void *p_data);
void g_sntp0_err_callback_internal(void *p_instance, void *p_data)
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
 *            - void sntp_client_init0(void)
 **********************************************************************************************************************/
void sntp_client_init0(void)
{
    UINT g_sntp0_err;
    /* Create SNTP Client. */
    g_sntp0_err = nx_sntp_client_create (&g_sntp0, &g_ip0, 0, &g_packet_pool0, leap_second_handler,
                                         kiss_of_death_handler, NULL);
    if (NX_SUCCESS != g_sntp0_err)
    {
        g_sntp0_err_callback ((void *) &g_sntp0, &g_sntp0_err);
    }
}
NX_DNS g_dns0;

#ifdef NX_DNS_CLIENT_USER_CREATE_PACKET_POOL
NX_PACKET_POOL g_dns0_packet_pool;
uint8_t g_dns0_pool_memory[NX_DNS_PACKET_POOL_SIZE];
#endif

#if defined(__ICCARM__)
#define g_dns0_err_callback_WEAK_ATTRIBUTE
#pragma weak g_dns0_err_callback  = g_dns0_err_callback_internal
#elif defined(__GNUC__)
#define g_dns0_err_callback_WEAK_ATTRIBUTE   __attribute__ ((weak, alias("g_dns0_err_callback_internal")))
#endif
void g_dns0_err_callback(void *p_instance, void *p_data)
g_dns0_err_callback_WEAK_ATTRIBUTE;
/*******************************************************************************************************************//**
 * @brief      This is a weak example initialization error function.  It should be overridden by defining a user  function
 *             with the prototype below.
 *             - void g_dns0_err_callback(void * p_instance, void * p_data)
 *
 * @param[in]  p_instance arguments used to identify which instance caused the error and p_data Callback arguments used to identify what error caused the callback.
 **********************************************************************************************************************/
void g_dns0_err_callback_internal(void *p_instance, void *p_data);
void g_dns0_err_callback_internal(void *p_instance, void *p_data)
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
 *            - void dns_client_init0(void)
 **********************************************************************************************************************/
void dns_client_init0(void)
{
    UINT g_dns0_err;

#ifdef NX_DNS_CLIENT_USER_CREATE_PACKET_POOL
    g_dns0_err = nx_packet_pool_create(&g_dns0_packet_pool, "g_dns0 Packet Pool", NX_DNS_PACKET_PAYLOAD, &g_dns0_pool_memory[0], NX_DNS_PACKET_POOL_SIZE);

    if (NX_SUCCESS != g_dns0_err)
    {
        g_dns0_err_callback((void *)&g_dns0,&g_dns0_err);
    }

    g_dns0.nx_dns_packet_pool_ptr = &g_dns0_packet_pool;
#endif

    /* Create DNS Client. */
    g_dns0_err = nx_dns_create (&g_dns0, &g_ip0, (UCHAR *) "g_dns0 DNS Client");
    if (NX_SUCCESS != g_dns0_err)
    {
        g_dns0_err_callback ((void *) &g_dns0, &g_dns0_err);
    }
}
NX_DHCP g_dhcp_client0;
#if defined(__ICCARM__)
#define g_dhcp_client0_err_callback_WEAK_ATTRIBUTE
#pragma weak g_dhcp_client0_err_callback  = g_dhcp_client0_err_callback_internal
#elif defined(__GNUC__)
#define g_dhcp_client0_err_callback_WEAK_ATTRIBUTE   __attribute__ ((weak, alias("g_dhcp_client0_err_callback_internal")))
#endif
void g_dhcp_client0_err_callback(void *p_instance, void *p_data)
g_dhcp_client0_err_callback_WEAK_ATTRIBUTE;
/*******************************************************************************************************************//**
 * @brief      This is a weak example initialization error function.  It should be overridden by defining a user  function
 *             with the prototype below.
 *             - void g_dhcp_client0_err_callback(void * p_instance, void * p_data)
 *
 * @param[in]  p_instance arguments used to identify which instance caused the error and p_data Callback arguments used to identify what error caused the callback.
 **********************************************************************************************************************/
void g_dhcp_client0_err_callback_internal(void *p_instance, void *p_data);
void g_dhcp_client0_err_callback_internal(void *p_instance, void *p_data)
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
 *            - void dhcp_client_init0(void)
 **********************************************************************************************************************/
void dhcp_client_init0(void)
{
    UINT g_dhcp_client0_err;
    /* Create DHCP client. */
    g_dhcp_client0_err = nx_dhcp_create (&g_dhcp_client0, &g_ip0, "g_dhcp_client0_DHCPv4");
    if (NX_SUCCESS != g_dhcp_client0_err)
    {
        g_dhcp_client0_err_callback ((void *) &g_dhcp_client0, &g_dhcp_client0_err);
    }

#if DHCP_USR_OPT_ADD_ENABLE_g_dhcp_client0
    /* Set callback function to add user options.  */
    g_dhcp_client0_err = nx_dhcp_user_option_add_callback_set(&g_dhcp_client0,
            dhcp_user_option_add_client1);

    if (NX_SUCCESS != g_dhcp_client0_err)
    {
        g_dhcp_client0_err_callback((void *)&g_dhcp_client0,&g_dhcp_client0_err);
    }
#endif
}

#if DHCP_USR_OPT_ADD_ENABLE_g_dhcp_client0 && DHCP_USR_OPT_ADD_FUNCTION_ENABLE_g_dhcp_client0
UCHAR g_dhcp_client0_opt_num = 60;
CHAR *g_dhcp_client0_opt_val = "REA";

/*******************************************************************************************************************//**
 * @brief     This DHCP user options add function adds Vendor Class ID. User can change the option number and/or option
 *            value in the user defined code by simply overriding the values. This function work fine to add any option
 *            which takes string values for option. Like below
 *            g_dhcp_client0_opt_num = 43;
 *            g_dhcp_client0_opt_val = "OPT43VAL";
 *            If user wants to chain the options, should disable this function generation and provide their own definition.
 **********************************************************************************************************************/
UINT dhcp_user_option_add_client1(NX_DHCP *dhcp_ptr, UINT iface_index, UINT message_type, UCHAR *user_option_ptr, UINT *user_option_length)
{
    NX_PARAMETER_NOT_USED(dhcp_ptr);
    NX_PARAMETER_NOT_USED(iface_index);
    NX_PARAMETER_NOT_USED(message_type);

    /* Application can check if add options into the packet by iface_index and message_type.
     message_type are defined in header file, such as: NX_DHCP_TYPE_DHCPDISCOVER.  */
    /* Add Vendor Class ID option refer to RFC2132.  */

    /* Check if have enough space for this option.  */
    if (*user_option_length < (strlen(g_dhcp_client0_opt_val) + 2))
    {
        return(NX_FALSE);
    }

    /* Set the option code.  */
    *user_option_ptr = g_dhcp_client0_opt_num;

    /* Set the option length.  */
    *(user_option_ptr + 1) = (UCHAR)strlen(g_dhcp_client0_opt_val);

    /* Set the option value (Vendor class id).  */
    memcpy((user_option_ptr + 2), g_dhcp_client0_opt_val, strlen(g_dhcp_client0_opt_val));

    /* Update the option length.  */
    *user_option_length = (strlen(g_dhcp_client0_opt_val) + 2);

    return(NX_TRUE);
}
#endif
TX_MUTEX mutex_sntp;
TX_MUTEX mutex_dns;
extern bool g_ssp_common_initialized;
extern uint32_t g_ssp_common_thread_count;
extern TX_SEMAPHORE g_ssp_common_initialized_semaphore;

void wifi_thread_create(void)
{
    /* Increment count so we will know the number of ISDE created threads. */
    g_ssp_common_thread_count++;

    /* Initialize each kernel object. */
    UINT err_mutex_sntp;
    err_mutex_sntp = tx_mutex_create (&mutex_sntp, (CHAR *) "SNTP Mutex", TX_NO_INHERIT);
    if (TX_SUCCESS != err_mutex_sntp)
    {
        tx_startup_err_callback (&mutex_sntp, 0);
    }
    UINT err_mutex_dns;
    err_mutex_dns = tx_mutex_create (&mutex_dns, (CHAR *) "DNS Mutex", TX_NO_INHERIT);
    if (TX_SUCCESS != err_mutex_dns)
    {
        tx_startup_err_callback (&mutex_dns, 0);
    }

    UINT err;
    err = tx_thread_create (&wifi_thread, (CHAR *) "WIFI Thread", wifi_thread_func, (ULONG) NULL, &wifi_thread_stack,
                            8000, 6, 6, 1, TX_DONT_START);
    if (TX_SUCCESS != err)
    {
        tx_startup_err_callback (&wifi_thread, 0);
    }
}

static void wifi_thread_func(ULONG thread_input)
{
    /* Not currently using thread_input. */
    SSP_PARAMETER_NOT_USED (thread_input);

    /* Initialize common components */
    tx_startup_common_init ();

    /* Initialize each module instance. */
    /** Call initialization function if user has selected to do so. */
#if (0)
    sntp_client_init0();
#endif
    /** Call initialization function if user has selected to do so. */
#if (0)
    dns_client_init0();
#endif
    /** Call initialization function if user has selected to do so. */
#if (0)
    dhcp_client_init0();
#endif

    /* Enter user code for this thread. */
    wifi_thread_entry ();
}
