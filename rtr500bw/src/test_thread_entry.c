

//#include "test_thread.h"
#include "MyDefine.h"
#include "Globals.h"
#include "Sfmem.h"
#include "opt_cmd_thread_entry.h"

//void bsp_exbus_init(void);

//void test_opt(void);

#define XRAM_BASE_ADDR (0x84000000)



//uint8_t t_r[2048];
//uint8_t t_w[2048];


/** Test Thread entry function */
void test_thread_entry(void)
{
    /* TODO: add your own code here */

//    volatile bool test = false;       //2019.Dec.26 コメントアウト

    bsp_exbus_init();

//    unsigned char *mem_ptr; //(unsigned char *)XRAM_BASE_ADDR;        //2019.Dec.26 コメントアウト
//    uint16_t i,j;     //2019.Dec.26 コメントアウト
//    volatile uint8_t wd, rd;      //2019.Dec.26 コメントアウト
  

    while(1)
    {
        tx_thread_sleep (10);
    }
    
 



}

#if 0
/**
 *
 */
void test_opt(void)
{
    opticom_initialize();
    tx_thread_sleep (1);
    opticom_uninitialize();
}
#endif

