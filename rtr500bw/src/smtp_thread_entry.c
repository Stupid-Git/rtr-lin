
#define _SMTP_THREAD_ENTRY_C_

#include "flag_def.h"
#include <string.h>
#include "Globals.h"
#include "General.h"
#include "Config.h"

#include "wifi_thread.h"
#include "smtp_client.h"
#include "smtp_thread.h"

extern TX_THREAD smtp_thread;


#define CLIENT_AUTHENTICATION_TYPE NX_SMTP_CLIENT_AUTH_LOGIN

smtp_client_t          g_smtpCLIENT;
smtp_msg_t             g_smtpMSG;

static void subSendMail(smtp_client_t *p_client, smtp_msg_t *p_msg);
UINT send_mail(smtp_client_t *p_client, smtp_msg_t *p_msg);
int MailPramSetTest(void);



char em_temp[256];

/**
 **  SMTP Thread entry function
 */
void smtp_thread_entry(void)
{
    /* TODO: add your own code here */

    UINT status;
    /*volatile*/ 
    smtp_client_t  *p_client;
    /*volatile*/ 
    smtp_msg_t     *p_msg;
    

    for(;;){
        memset (&g_smtpCLIENT, 0, sizeof(smtp_client_t));

    
        p_client = &g_smtpCLIENT;
        p_client->p_packet_pool = &g_packet_pool0;

        Printf("size = %d\r\n", sizeof(smtp_msg_t));
        memset (&g_smtpMSG, 0, sizeof(smtp_msg_t));
        p_msg = &g_smtpMSG;
 

        status = send_mail(p_client, p_msg);

        Printf("SMTP Send status %02X\r\n", status);        // a3が返ってきた DNS error

        tx_event_flags_set (&event_flags_smtp, FLG_SMTP_SUCCESS, TX_OR);

        tx_thread_suspend(&smtp_thread);
        tx_thread_sleep (1);

    }
  
}

/**
 * メール送信 サブ関数
 * @param p_client
 * @param p_msg
 * @note    2020.01.29  サブ関数化
 */
static void subSendMail(smtp_client_t *p_client, smtp_msg_t *p_msg){
    sprintf((char *)EmHeader.SRV,  "%s", my_config.smtp.Server);       //  直接読んでも良いが、最後にNUllが無い場合に対応
    sprintf((char *)EmHeader.ID, "%s", (char *)&my_config.smtp.UserID);
    sprintf((char *)EmHeader.PW, "%s", (char *)&my_config.smtp.UserPW);
    sprintf((char *)EmHeader.From, "%s", (char *)my_config.smtp.From);

    p_client->server_name = (char *)EmHeader.SRV;
    p_client->user_name = (char *)EmHeader.ID;
    p_client->user_pwd  = (char *)EmHeader.PW;
    p_client->server_port = (UINT)(my_config.smtp.Port[0] + my_config.smtp.Port[1] * 256);


    p_msg->mail_to = (char *)EmHeader.TO;
    p_msg->mail_from = (char *)EmHeader.From;

}

/**
 *
 * @param p_client
 * @param p_msg
 * @return
 */
UINT send_mail(smtp_client_t *p_client, smtp_msg_t *p_msg)
{
    UINT status = 0;

    subSendMail(p_client, p_msg);      //2020.01.29    サブ関数化

    p_msg->subject   = (char *)EmHeader.Sbj;
    p_msg->body      = (char *)EmHeader.BDY;
    p_msg->file      = (char *)EmHeader.ATA;
    if(EmHeader.ATA[0] == 0x00){
        p_msg->attach = 0;
    }
    else{
        p_msg->attach = 1;    
    }

    /*
    p_msg->mail_to   = "tooru.hayashi@tandd.co.jp"; 
    p_msg->mail_from = "bwa36476@nifty.com";
    p_msg->subject   = "NX test mail (R500BW_smtp_1) nifty port 587";
    p_msg->body      = "Hello Dude.";

    p_client->server_name = "smtp.nifty.com";
    p_client->server_port = 587;
    p_client->user_name = "bwa36476";
    p_client->user_pwd = "3ss536xn";
    */

    Printf("un %s\r\n", p_client->user_name);
    Printf("pw %s\r\n", p_client->user_pwd);

    status = smtp_mail_client_send(p_client, p_msg);

    return (status);
}

/**
 *
 * @return
 */
int MailPramSetTest(void)
{
    smtp_msg_t     *p_msg;
    smtp_client_t  *p_client;
    p_client = &g_smtpCLIENT;
    p_msg = &g_smtpMSG;


    Printf("SMTP Parameter \r\n");
    subSendMail(p_client, p_msg);      //2020.01.29    サブ関数化

    Printf("Email Sever Name %s \r\n", p_client->server_name);
    Printf("Email User  Name %s \r\n", p_client->user_name);
    Printf("Email User  Pwd  %s \r\n", p_client->user_pwd);
    Printf("Email Srv   Port %d \r\n", p_client->server_port);


    return (0);
}
