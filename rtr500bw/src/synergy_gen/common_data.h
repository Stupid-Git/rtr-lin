/* generated common header file - do not edit */
#ifndef COMMON_DATA_H_
#define COMMON_DATA_H_
#include <stdint.h>
#include "bsp_api.h"
#include "sf_power_profiles_v2.h"
#include "r_lpmv2_s5d9.h"
#include "r_lpmv2_api.h"
#include "fx_api.h"
#include "sf_block_media_ram.h"
#include "sf_block_media_api.h"
#include "sf_el_fx.h"
#include "fx_api.h"
#include "nx_api.h"
#include "sf_el_nx_cfg.h"
#include "../src/framework/sf_el_nx/nx_renesas_synergy.h"
#include "r_crypto_api.h"
#include "r_aes_api.h"
#include "r_rsa_api.h"
#include "r_ecc_api.h"
#include "r_hash_api.h"
#include "r_trng_api.h"
#include "r_sce.h"
#include "nx_crypto_sce_config.h"
#include "nx_secure_tls.h"
#include "nx_secure_tls_api.h"
#include "nx_api.h"
#include "nx_md5.h"
#include "r_icu.h"
#include "r_external_irq_api.h"
#include "r_dtc.h"
#include "r_transfer_api.h"
#include "r_sci_spi.h"
#include "r_spi_api.h"
#include "sf_wifi_api.h"
#include "sf_wifi_cc31xx.h"
#if !SF_WIFI_CC31XX_SDK6_CFG_ONCHIP_STACK_SUPPORT
#include "sf_wifi_nsal_api.h"
#endif
#include "nx_api.h"
#include "sf_wifi_nsal_nx.h"
#include "nx_api.h"

#include "nx_api.h"

#include "r_dmac.h"
#include "r_transfer_api.h"
#include "r_elc.h"
#include "r_elc_api.h"
#include "r_cgc.h"
#include "r_cgc_api.h"
#include "r_fmi.h"
#include "r_fmi_api.h"
#include "r_ioport.h"
#include "r_ioport_api.h"
#ifdef __cplusplus
extern "C"
{
#endif
/** Power Profiles on Power Profiles instance */
extern sf_power_profiles_v2_instance_t g_sf_power_profiles_v2_common;

void sf_power_profiles_v2_init0(void);
/** lpmv2 Instance */
extern const lpmv2_instance_t g_lpmv2_deep_standby;
void fx_common_init0(void);
/** Block Media on RAM Instance */
extern sf_block_media_instance_t g_sf_block_media_ram0;
extern sf_el_fx_t g_sf_el_fx0_cfg;
extern FX_MEDIA g_fx_media0;

void g_fx_media0_err_callback(void *p_instance, void *p_data);
ssp_err_t fx_media_init0_format(void);
uint32_t fx_media_init0_open(void);
void fx_media_init0(void);
#ifndef set_mac_callback
void set_mac_callback(nx_mac_address_t *p_mac_config);
#endif
#ifndef NULL
void NULL(NX_PACKET *packet_ptr, USHORT packet_type);
#endif
VOID nx_ether_driver_eth0(NX_IP_DRIVER *driver_req_ptr);
extern VOID (*g_sf_el_nx_eth)(NX_IP_DRIVER *driver_req_ptr);
extern const crypto_instance_t g_sce_0;
#define R_SCE_SERVICES_AES_PLAIN_TEXT_128_ECB   (1)
#define R_SCE_SERVICES_AES_PLAIN_TEXT_128_CBC   (1)
#define R_SCE_SERVICES_AES_PLAIN_TEXT_128_CTR   (1)
#define R_SCE_SERVICES_AES_PLAIN_TEXT_128_GCM   (1)
#define R_SCE_SERVICES_AES_PLAIN_TEXT_128_XTS   (1)
#define R_SCE_SERVICES_AES_PLAIN_TEXT_192_ECB   (1)
#define R_SCE_SERVICES_AES_PLAIN_TEXT_192_CBC   (1)
#define R_SCE_SERVICES_AES_PLAIN_TEXT_192_CTR   (1)
#define R_SCE_SERVICES_AES_PLAIN_TEXT_192_GCM   (1)
#define R_SCE_SERVICES_AES_PLAIN_TEXT_256_ECB   (1)
#define R_SCE_SERVICES_AES_PLAIN_TEXT_256_CBC   (1)
#define R_SCE_SERVICES_AES_PLAIN_TEXT_256_CTR   (1)
#define R_SCE_SERVICES_AES_PLAIN_TEXT_256_GCM   (1)
#define R_SCE_SERVICES_AES_PLAIN_TEXT_256_XTS   (1)
#define R_SCE_SERVICES_AES_WRAPPED_128_ECB      (1)
#define R_SCE_SERVICES_AES_WRAPPED_128_CBC      (1)
#define R_SCE_SERVICES_AES_WRAPPED_128_CTR      (1)
#define R_SCE_SERVICES_AES_WRAPPED_128_GCM      (1)
#define R_SCE_SERVICES_AES_WRAPPED_128_XTS      (1)
#define R_SCE_SERVICES_AES_WRAPPED_192_ECB      (1)
#define R_SCE_SERVICES_AES_WRAPPED_192_CBC      (1)
#define R_SCE_SERVICES_AES_WRAPPED_192_CTR      (1)
#define R_SCE_SERVICES_AES_WRAPPED_192_GCM      (1)
#define R_SCE_SERVICES_AES_WRAPPED_256_ECB      (1)
#define R_SCE_SERVICES_AES_WRAPPED_256_CBC      (1)
#define R_SCE_SERVICES_AES_WRAPPED_256_CTR      (1)
#define R_SCE_SERVICES_AES_WRAPPED_256_GCM      (1)
#define R_SCE_SERVICES_AES_WRAPPED_256_XTS      (1)
#define R_SCE_SERVICES_RSA_PLAIN_TEXT_1024      (1)
#define R_SCE_SERVICES_RSA_PLAIN_TEXT_2048      (1)
#define R_SCE_SERVICES_RSA_WRAPPED_1024         (1)
#define R_SCE_SERVICES_RSA_WRAPPED_2048         (1)
#define R_SCE_SERVICES_ECC_PLAIN_TEXT_192       (1)
#define R_SCE_SERVICES_ECC_PLAIN_TEXT_256       (1)
#define R_SCE_SERVICES_ECC_WRAPPED_192          (1)
#define R_SCE_SERVICES_ECC_WRAPPED_256          (1)
#define R_SCE_SERVICES_HASH_SHA1                (1)
#define R_SCE_SERVICES_HASH_SHA256              ((1) || (1))
#define R_SCE_SERVICES_HASH_MD5                 (1)
#define R_SCE_SERVICES_TRNG                     (1)
/** Software based crypto ciphers for use with nx_secure_tls_session_create. */
extern const NX_SECURE_TLS_CRYPTO nx_crypto_tls_ciphers;
/* External IRQ on ICU Instance. */
extern const external_irq_instance_t g_external_irq13;
#ifndef custom_hw_irq_isr
void custom_hw_irq_isr(external_irq_callback_args_t *p_args);
#endif
/* Transfer on DTC Instance. */
extern const transfer_instance_t g_transfer_s8r;
#ifndef NULL
void NULL(transfer_callback_args_t *p_args);
#endif
/* Transfer on DTC Instance. */
extern const transfer_instance_t g_transfer_s8t;
#ifndef NULL
void NULL(transfer_callback_args_t *p_args);
#endif
extern const spi_cfg_t g_spi8_cfg;
/** SPI on SCI Instance. */
extern const spi_instance_t g_spi8;
extern sci_spi_instance_ctrl_t g_spi8_ctrl;
extern const sci_spi_extended_cfg g_spi8_cfg_extend;

#ifndef NULL
void NULL(spi_callback_args_t *p_args);
#endif

#define SYNERGY_NOT_DEFINED (1)            
#if (SYNERGY_NOT_DEFINED == g_transfer_s8t)
#define g_spi8_P_TRANSFER_TX (NULL)
#else
#define g_spi8_P_TRANSFER_TX (&g_transfer_s8t)
#endif
#if (SYNERGY_NOT_DEFINED == g_transfer_s8r)
#define g_spi8_P_TRANSFER_RX (NULL)
#else
#define g_spi8_P_TRANSFER_RX (&g_transfer_s8r)
#endif
#undef SYNERGY_NOT_DEFINED

#define g_spi8_P_EXTEND (&g_spi8_cfg_extend)
/** sf_wifi_v2 on CC31XX_SDK6 Wi-Fi Driver instance */
extern sf_wifi_instance_t g_sf_wifi0;
#ifdef NULL
#define SF_WIFI_ON_WIFI_CC31XX_SDK6_CALLBACK_USED_g_sf_wifi0 (0)
#else
#define SF_WIFI_ON_WIFI_CC31XX_SDK6_CALLBACK_USED_g_sf_wifi0 (1)
#endif
#if SF_WIFI_ON_WIFI_CC31XX_SDK6_CALLBACK_USED_g_sf_wifi0
/** Declaration of user callback function. This function MUST be defined in the user application.*/
void NULL(sf_wifi_callback_args_t *p_args);
#endif
/** NetX driver entry function. */
VOID g_sf_el_nx0(NX_IP_DRIVER *p_driver);
void nx_common_init0(void);
extern NX_PACKET_POOL g_packet_pool1;
void g_packet_pool1_err_callback(void *p_instance, void *p_data);
void packet_pool_init1(void);
extern NX_PACKET_POOL g_packet_pool0;
void g_packet_pool0_err_callback(void *p_instance, void *p_data);
void packet_pool_init0(void);
extern ssp_err_t sce_initialize(void);
extern void nx_secure_tls_initialize(void);
extern void nx_secure_common_init(void);
extern NX_IP g_ip0;
void g_ip0_err_callback(void *p_instance, void *p_data);
void ip_init0(void);

#include "ux_api.h"

/* USBX Host Stack initialization error callback function. User can override the function if needed. */
void ux_v2_err_callback(void *p_instance, void *p_data);

#if !defined(NULL)
/* User Callback for Host Event Notification (Only valid for USB Host). */
extern UINT NULL(ULONG event, UX_HOST_CLASS *host_class, VOID *instance);
#endif

#if !defined(NULL)
/* User Callback for Device Event Notification (Only valid for USB Device). */
extern UINT NULL(ULONG event);
#endif

#ifdef UX_HOST_CLASS_STORAGE_H
/* Utility function to get the pointer to a FileX Media Control Block for a USB Mass Storage device. */
UINT ux_system_host_storage_fx_media_get(UX_HOST_CLASS_STORAGE * instance, UX_HOST_CLASS_STORAGE_MEDIA ** p_storage_media, FX_MEDIA ** p_fx_media);
#endif
void ux_common_init0(void);
/* Transfer on DMAC Instance. */
extern const transfer_instance_t g_transfer_u1;
#ifndef NULL
void NULL(transfer_callback_args_t *p_args);
#endif
/* Transfer on DMAC Instance. */
extern const transfer_instance_t g_transfer_u0;
#ifndef NULL
void NULL(transfer_callback_args_t *p_args);
#endif
#include "ux_api.h"
#include "ux_dcd_synergy.h"
#include "sf_el_ux_dcd_fs_cfg.h"
void g_sf_el_ux_dcd_fs_0_err_callback(void *p_instance, void *p_data);
#include "ux_api.h"
#include "ux_dcd_synergy.h"

/* USBX Device Stack initialization error callback function. User can override the function if needed. */
void ux_device_err_callback(void *p_instance, void *p_data);
void ux_device_init0(void);
void ux_device_remove_compiler_padding(unsigned char *p_device_framework, UINT length);
/* Header section starts for g_ux_device_class_cdc_acm0 */
#include "ux_api.h"
#include "ux_device_class_cdc_acm.h"
/* USBX CDC-ACM Instance Activate User Callback Function */
extern VOID ux_cdc_device0_instance_activate(VOID *cdc_instance);
/* USBX CDC-ACM Instance Deactivate User Callback Function */
extern VOID ux_cdc_device0_instance_deactivate(VOID *cdc_instance);
/* Header section ends for g_ux_device_class_cdc_acm0 */
void ux_device_class_cdc_acm_init0(void);
void g_ux_device_class_cdc_acm0_ux_device_open_init(void);
/** ELC Instance */
extern const elc_instance_t g_elc;
/** CGC Instance */
extern const cgc_instance_t g_cgc;
/** FMI on FMI Instance. */
extern const fmi_instance_t g_fmi;
/** IOPORT Instance */
extern const ioport_instance_t g_ioport;
void g_common_init(void);
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* COMMON_DATA_H_ */
