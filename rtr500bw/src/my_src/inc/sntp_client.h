/*
 * sntp_client.h
 *
 *  Created on: 2019/07/25
 *      Author: karel
 */

#ifndef _SNTP_CLIENT_H_
#define _SNTP_CLIENT_H_




#ifdef EDF
#undef EDF
#endif

#ifndef _SNTP_CLIENT_C_
    #define EDF extern
#else
    #define EDF
#endif


/** NTP server address */
//#define SNTP_SERVER "time.google.com"         sakaguchi del UT-0028

/** Seconds between Unix Epoch (1/1/1970) and NTP Epoch (1/1/1999) */
#define UNIX_TO_NTP_EPOCH_SECS          0x83AA7E80

EDF int sntp_setup_and_run(void);
EDF int sntp_setup_and_run2(void);
EDF void sntp_stop(void);
EDF unsigned long get_time(void);
EDF unsigned long get_adjust_time(void);
EDF unsigned long get_time(void);



EDF unsigned long get_sntp_boot_time(void);/* Return sntp_boot_time in seconds */

#endif /* SNTP_CLIENT_H_ */
