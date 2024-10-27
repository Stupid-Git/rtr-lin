/**
 * @file    smtp_thread_entry.h
 *
 * @date    2019/11/29
 * @author  tooru.hayashi
 */



#ifndef _SMTP_THREAD_ENTRY_H_
#define _SMTP_THREAD_ENTRY_H_




#ifdef EDF
#undef EDF
#endif

#ifndef _SMTP_THREAD_ENTRY_C_
    #define EDF extern
#else
    #define EDF
#endif



EDF int MailPramSetTest(void);


#endif /* _SMTP_THREAD_ENTRY_H_ */
