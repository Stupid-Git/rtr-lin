/**
 * @file    Globals.c
 *
 * @date   Created on: 2019/01/23
 * @author haya
 */

#define GLOBALS_C_

#include "Globals.h"

//#undef GLOBALS_C_



/**
 * 外部SRAM設定
 * CS4   0x84000000, LENGTH = 0x007ffff  512K byte
 */
void bsp_exbus_init(void)
{
    R_BUS->CSRC1[4].CSnCR_b.BSIZE = 2;          // 8 bit bus
    //R_BUS->CSRC1[4].CSnCR_b.EMODE = 1;        // Big Endian
    R_BUS->CSRC1[4].CSnCR_b.EMODE = 0;          // Little Endian
    R_BUS->CSRC1[4].CSnCR_b.MPXEN = 0;          // sepalete bus

    R_BUS->CSRC0[4].CSnMOD_b.WRMOD = 0;         // Byte Mode Strobe
    R_BUS->CSRC0[4].CSnWCR2_b.CSWOFF = 0;       // Write-Access CS Extension Cycle Select=> 1wait
    R_BUS->CSRC0[4].CSnWCR2_b.WDOFF = 0;        // Write-Data Output Extension Cycle Select=> 1wait
    
    R_BUS->CSRECEN = 0x3e00;                    // No recovery cycle wait states

    R_BUS->CSRC0[4].CSnWCR1_b.CSPRWAIT = 0;
    R_BUS->CSRC0[4].CSnWCR1_b.CSPWWAIT = 0;
    R_BUS->CSRC0[4].CSnWCR1_b.CSRWAIT = 6;      // read
    R_BUS->CSRC0[4].CSnWCR1_b.CSWWAIT = 7;      // write

    R_BUS->CSRC1[4].CSnREC_b.WRCV =  1;           // Write Recovery cycle
    
    R_BUS->CSRECEN_b.RECVEN6 = 1;               // Write Recovery for same area write after write access
    R_BUS->CSRECEN_b.RECVEN7 = 1;               // Write Recovery for different area write after write access

    R_BUS->CSRC1[4].CSnCR_b.EXENB = 1;
}


// 以下は CS4の外部アドレス
// XRAM CY62148  0x80000    512K
// 外部RAMは、バイトアクセス ！！
//####  ここは外部RAM #####################################################
//####  ここは外部RAM #####################################################
//####  ここは外部RAM #####################################################

#pragma pack(1)

//uint16_t CurrentWarning		__attribute__((section(".xram")));      // 警報の状態        -> warning.c

char 	BackUpTest[100]        __attribute__((section(".xram")));
//=============  XML ====================================

char    XMLBuff[ 1500 ]      __attribute__((section(".xram")));     ///<  XML送信用
char    XMLTemp[ 300 ]       __attribute__((section(".xram")));         ///<  XMLテンポラリ
char    XMLTForm[ 100 ]      __attribute__((section(".xram")));     ///<  XML作成
char    TagTemp[ 200 ]       __attribute__((section(".xram")));         ///<  XMLタグ作成用
char    XMLName[ 300 ]       __attribute__((section(".xram")));         ///<  XMLファイル名
char    EntityTemp[ 200 ]        __attribute__((section(".xram")));     ///<  XML実体作成用


// ＃実態＃
//EDF def_RFB RFB __attribute__((section(".xram")));     // 101382 byte

///作業用バッファ
/// @attention 複数のスレッドから読み書きされるが 排他制御等していない
char huge_buffer[32768]         __attribute__((section(".xram")));
///作業用バッファ
char work_buffer[32768]         __attribute__((section(".xram")));
///XML作業用バッファ
char xml_buffer[32768*4]        __attribute__((section(".xram")));

///HTTP通信バッファ
char http_buffer[51200]			__attribute__((section(".xram")));		//sakaguchi add

//EDF uint8_t xml_buffer[4096]; //  __attribute__((section(".xram")));
///証明書作業用バッファ
char CertFile_WS[4096]          __attribute__((section(".xram")));
///証明書作業用バッファ
char CertFile_USER[4096]        __attribute__((section(".xram")));
///グループバッファ
char Group_Buffer[1024]         __attribute__((section(".xram")));



//EDF uint8_t log_test[1024]  __attribute__((section(".xram")));


//####  ここは外部RAM #####################################################
//####  ここは外部RAM #####################################################

///WiFiスキャン時の RSSSI,SSID構造体
def_wifi_scan_t AP_List[16]   __attribute__((section(".xram")));



def_EmHeader EmHeader     	__attribute__((section(".xram")));

def_Warn 	 WarnVal	 	__attribute__((section(".xram")));

def_VGroup 	 VGroup	 		__attribute__((section(".xram")));

//HTTP通信フラグ
def_http_file_snd_t	G_HttpFile[10]		__attribute__((section(".xram")));
//def_http_cmd_snd_t	G_HttpCmd[64]		__attribute__((section(".xram")));
def_http_cmd_snd_t	G_HttpCmd[128]		__attribute__((section(".xram")));



/*
//=============  Email =================================

EDF	struct {

    char	Sbj[190];				// SBJ[96]; Subject
	char	__gap2[2];

	char	TOA1[64+2];			// To Address ( ***@aaa.com )
	char	TOA2[64+2];			// To Address ( ***@aaa.com )
	char	TOA3[64+2];			// To Address ( ***@aaa.com )
	char	TOA4[64+2];			// To Address ( ***@aaa.com )

	//uint8_t	POP3[64];				// POP3サーバ名
	//uint8_t	__gap21[2];
	//uint8_t	MID[64];				// POP3 ID
	//uint8_t	__gap22[2];
	//uint8_t	MPW[64];				// POP3 PW
	//uint8_t	__gap23[2];

	char	SRV[64];				// SMTPサーバ名
	char	__gap24[2];
	char	ID[64];					// SMTP認証 ID
	char	__gap25[2];
	char	PW[64];					// SMTP認証 PW
	char	__gap26[2];

	char	TO[96];					// To Desc (address )
	char	__gap7[2];
	char	From[64];				// Return(FROM) Address ( ***@bbb.com )
	char	__gap8[2];
	char	Sender[96];				// Sender(FROM) Desc ( t_and_d )
	char	__gap9[2];
	char	ATA[190];				// ATA[64]; // Attach file name
	char	__gap10[2];
	char	BDY[2400UL];			// Body ※iChipレジスタは96byteだが
	char	__gap11[2];
	char	ENCBDY[2400UL];			// Body ※iChipレジスタは96byteだが
	char	__gap12[2];

} EmHeader __attribute__((section(".xram")));
*/


//EDF uint8_t XramTest[1024*1] __attribute__((section(".xram")));

#pragma pack(4)
