/**
 * @brief   データフラッシュメモリ処理関数
 * @file    dataflash.c
 *
 * @date     Created on: 2019/12/15
 * @author t.saito
 */
#include "hal_data.h"
//#include "common_data.h"
#include "dataflash.h"
#include "General.h"

/* Flags, set from Callback function */
static volatile _Bool flash_event_not_blank = false;        ///<コールバック関数用変数 notブランクフラグ
static volatile _Bool flash_event_blank = false;            ///<コールバック関数用変数 ブランクフラグ
static volatile _Bool flash_event_erase_complete = false;   ///<コールバック関数用変数  消去完了フラグ
static volatile _Bool flash_event_write_complete = false;   ///<コールバック関数用変数 書き込み完了フラグ
static volatile _Bool flash_event_err_df_access = false;    ///<コールバック関数用変数 データフラッシュアクセスエラー
static volatile _Bool flash_event_err_cf_access = false;    ///<コールバック関数用変数 コードフラッシュアクセスエラー
static volatile _Bool flash_event_err_cmd_locked = false;   ///<コールバック関数用変数 コマンドロックフラグ
static volatile _Bool flash_event_err_failure = false;      ///<コールバック関数用変数 失敗フラグ

volatile uint32_t debug_dataflash_status;

//プロトタイプ
void flash_hp_open(void);
void flash_hp_close(void);
void wait_for_blankcheck_flag(void);
int CheckBlank_data_flash(uint32_t address, uint32_t size);
void eraseBlock_data_flash(uint32_t address, uint32_t num);
void write_data_flash(uint32_t src_address, uint32_t dst_address,  uint32_t length);
void BGO_Callback(flash_callback_args_t * p_args);



//関数本体

/**
 *  @brief  ブランクチェックフラグ変化待ち
 *  hw_flash_hp.cのイベントでセットされる
 */
void wait_for_blankcheck_flag(void)
{
    /* Wait for callback function to set flag */
    while(!(flash_event_not_blank||flash_event_blank));
    if(flash_event_not_blank)
    {
        /* Reset Flag */
        flash_event_blank = false;
    }
    else if(flash_event_blank)
    {
        /* Reset Flag */
        flash_event_not_blank = false;
    }
}


/**
 * @brief  フラッシュ オープン
 */
void flash_hp_open(void)
{
    /* Initialize Flash HP HAL using Flash HP API */
    ssp_err_t err = g_flash0.p_api->open(g_flash0.p_ctrl, g_flash0.p_cfg);
    if(err != SSP_SUCCESS)
    {
        Printf("data flash Open error \n");
    }
}
/**
 * @brief  フラッシュクローズ
 */
void flash_hp_close(void){
    ssp_err_t err = g_flash0.p_api->close(g_flash0.p_ctrl);
    if(err != SSP_SUCCESS)
    {
        Printf("data flash Close error \n");
    }

}
/**
 * @brief  データフラッシュブランクチェック
 * @param address
 * @param size
 * @return 0=FLASH_RESULT_BLANK, 1= FLASH_RESULT_NOT_BLANK, 2 = FLASH_RESULT_BGO_ACTIVE
 */
int CheckBlank_data_flash(uint32_t address, uint32_t size){
    ssp_err_t err;
    flash_result_t blankCheck;

    flash_event_not_blank = false;
    flash_event_blank = false;
    flash_hp_open();
    err = g_flash0.p_api->blankCheck(g_flash0.p_ctrl, address, size, &blankCheck);

    if(err != SSP_SUCCESS)
    {
        Printf("data flash blankCheck error \n");
    }

    if(blankCheck == FLASH_RESULT_BGO_ACTIVE)
    {
        wait_for_blankcheck_flag();     //フラグ変化待ち
        if(flash_event_not_blank){blankCheck= FLASH_RESULT_NOT_BLANK;}
        if(flash_event_blank){blankCheck= FLASH_RESULT_BLANK;}
    }
    flash_hp_close();
    return ((int)blankCheck);
    ;
}

/**
 * @brief  データフラッシュ ブロック消去(64Byteブロック)
 * @param address   消去ブロックアドレス
 * @param num       消去ブロック数
 * @note データ領域のイレース：4/128/256バイト単位
 * @attention   消去後のデータは不定値(アクセスのたびに値が違う)
 */
void eraseBlock_data_flash(uint32_t address, uint32_t num){
    ssp_err_t err;

    flash_event_erase_complete = 0;
    err = g_flash0.p_api->erase(g_flash0.p_ctrl, address, num );
    if(err != SSP_SUCCESS)
    {
        Printf("data flash erase error \n" );
    }

    // wait here for the erase operation to complete (only needed when in BGO)
    if (g_flash0.p_cfg->data_flash_bgo){
        while(!flash_event_erase_complete);
    }
}


/**
 * @brief  データフラッシュ 書き込み
 * @param src_address   ソースアドレス
 * @param dst_address   書き込み先アドレス（データフラッシュ アドレス）
 * @param length    書き込みサイズ（バイト）
 * @note    データ領域へのプログラム：4/8/16バイト単位
 * @attention   消去はブロック単位なので注意
 */
void write_data_flash(uint32_t src_address, uint32_t dst_address,  uint32_t length){
    ssp_err_t err;

    flash_event_write_complete = false;
    err = g_flash0.p_api->write(g_flash0.p_ctrl, src_address, dst_address, length);
    if(err != SSP_SUCCESS)
    {
        Printf("data flash write error \n" );
    }

    // wait here for the write operation to complete (only needed with BGO)
    if (g_flash0.p_cfg->data_flash_bgo){
        while(!flash_event_write_complete)
        {
            if(flash_event_err_df_access){
                __NOP();		//デバッグ用
            }
        }
    }



}



/**
 * @brief  DATA Flash Callback function for FLASH HP HAL.
 * @param p_args    コールバック変数
 */
#if 0 //TODO
void BGO_Callback(flash_callback_args_t * p_args)
{
    debug_dataflash_status = p_args->event; //デバッグ用

    if(p_args->event == FLASH_EVENT_NOT_BLANK)
    {
        flash_event_not_blank = true;
    }
    else if(p_args->event == FLASH_EVENT_BLANK)
    {
        flash_event_blank = true;
    }
    else if(p_args->event ==  FLASH_EVENT_ERASE_COMPLETE)
    {
        flash_event_erase_complete = true;
    }
    else if(p_args->event ==  FLASH_EVENT_WRITE_COMPLETE)
    {
        flash_event_write_complete = true;
    }
    else if(p_args->event == FLASH_EVENT_ERR_DF_ACCESS)
    {
        flash_event_err_df_access = true;
    }
    else if(p_args->event == FLASH_EVENT_ERR_CF_ACCESS)
    {
        flash_event_err_cf_access = true;
    }
    else if(p_args->event == FLASH_EVENT_ERR_CMD_LOCKED)
    {
        flash_event_err_cmd_locked = true;
    }
    else if(p_args->event == FLASH_EVENT_ERR_FAILURE)
    {
        flash_event_err_failure = true;
    }
}
#endif // 0 //TODO
















