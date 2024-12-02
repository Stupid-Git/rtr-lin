/*
 * comp_cmd_sub.h
 *
 *  Created on: Nov 25, 2024
 *      Author: karel
 */

#ifndef COMP_CMD_SUB_H_
#define COMP_CMD_SUB_H_

#include <stdint.h>
#include "Globals.h" // for comSOH_t


#ifdef EDF
#undef EDF
#endif
#ifndef COMP_CMD_SUB_C_
#define EDF extern
#else
#define EDF
#endif


uint32_t CFWriteInfo(void);                                          // 機器情報の設定
uint32_t CFReadInfo(void);                                           // 機器情報の取得

uint32_t CFWriteNetwork(void);                                       // ネットワークの設定
uint32_t CFReadNetwork(void);                                        // ネットワークの取得
uint32_t CFReadWalnAp(void);                                         // WLAM Ap List

uint32_t CFWriteNetParam(void);
uint32_t CFReadNetParam(void);
uint32_t CFWriteWlanParam(void);
uint32_t CFReadWlanParam(void);
uint32_t CFWProxy(void);
uint32_t CFRProxy(void);
uint32_t CFWriteBleParam(void);
uint32_t CFReadBleParam(void);
uint32_t CFReadBleAdv(void);
uint32_t CFBleAuth(void);


uint32_t CFWriteDtime(void);                                     // 時刻情報の設定
uint32_t CFReadDtime(void);                                      // 時刻情報の取得


uint32_t CFWriteSettingsFile(void);                              // 設定（登録）ファイル書き込み
uint32_t CFReadSettingsFile(void);                               // 設定（登録）ファイル読み込み

uint32_t CFWriteCertFile(void);                                  // ルート証明書書き込み
uint32_t CFReadCertFile(void);                                   // ルート証明書読み込み


uint32_t CFWriteWarning(void);                                       // 警報設定の書き込み
uint32_t CFReadWarning(void);                                        // 警報設定の読み込み
uint32_t CFWriteMonitor(void);
uint32_t CFReadMonitor(void);
uint32_t CFWriteSuction(void);
uint32_t CFReadSuction(void);
uint32_t CFWriteEncode(void);
uint32_t CFReadEncode(void);

uint32_t CFWriteUnitSettingsFile(void);                          // 設定（登録）ファイルに含まれる子機の設定書き込み

uint32_t CFReadVGroup(void);
uint32_t CFWriteVGroup(void);

uint32_t CFReadSettingsGroup(void);
uint32_t CFStartMonitor(void);
uint32_t CFClearMonitor(void);
uint32_t CFReadMonitorResult(void);

uint32_t CFWriteUserDefine(void);                                // ユーザ定義情報の書き込み
uint32_t CFReadUserDefine(void);                                 // ユーザ定義情報の読み込み

uint32_t CFInitilize(void);                                      // 本体の初期化
uint32_t CFBSStatus(void);                                       // 本体の状態
uint32_t CFNetworkStatus(void);                                  // ネットワークの状態
uint32_t CFReadLog(void);

uint32_t CFExtentionReg(void);                                   // 光通信 子機登録
uint32_t CFExtentionSerial(void);                                // 光通信 シリアル番号の取得
uint32_t CFExtSettings(void);                                    // 光通信 設定値読み書き
uint32_t CFExtSuction(void);                                     // 光通信 データ吸い上げ
uint32_t CFExtWirelessOFF(void);                                 // 光通信 電波制御
uint32_t CFExtRecStop(void);                                     // 光通信 記録停止
uint32_t CFExtAdjust(void);                                      // 光通信 アジャストメント
uint32_t CFExtScale(void);                                       // 光通信 スケール変換
uint32_t CFExtSOH(void);                                         // 光通信 ＳＯＨコマンドの送受信
uint32_t CFExtBlp(void);                                         // 光通信 Bluetooth設定の設定と取得


uint32_t CFExWirelessLevel(void);                              // 子機の電波強度＆電池残量
uint32_t CFRelayWirelessLevel(void);                           // 中継機の電波残量＆電池残量
uint32_t CFWirelessSettingsRead(void);                         // 設定値読み込み
uint32_t CFWirelessSettingsWrite(void);                        // 設定値書き込み
uint32_t CFWirelessMonitor(void);                              // 現在値取得
uint32_t CFWirelessSuction(void);                              // データ吸い上げコマンド
uint32_t CFSearchKoki(void);                                   // 子機検索
uint32_t CFSearchRelay(void);                                  // 中継機検索
uint32_t CFExRealscan(void);                                   // リアルスキャン
uint32_t CFThroughoutSearch(void);                             // 子機と中継機の一斉検索
uint32_t CFRecStartGroup(void);                                // グループ一斉記録開始
uint32_t CFWirelessRecStop(void);                              // 記録停止
uint32_t CFSetProtection(void);                                // 子機のプロテクト設定
uint32_t CFExInterval(void);                                   // 子機のモニタリング間隔設定
uint32_t CFWirelessRegistration(void);                         // 子機登録変更

uint32_t CFBleSettingsWrite(void);                             // BLE設定の書き込み
uint32_t CFBleSettingsRead(void);                              // BLE設定の読み出し
uint32_t CFScaleSettingsWrite(void);                           // スケール変換式の書き込み
uint32_t CFScaleSettingsRead(void);                            // スケール変換式の読み出し


uint32_t CFPushSettingsRead(void);                           // ＰＵＳＨ 子機の設定値読み込み
uint32_t CFPushSettingsWrite(void);                          // ＰＵＳＨ 子機の設定値書き込み
uint32_t CFPushSuction(void);                                // ＰＵＳＨ データ吸い上げ
uint32_t CFPushRecErase(void);                               // ＰＵＳＨ 子機の記録データ消去
uint32_t CFPushItemListRead(void);                           // ＰＵＳＨ 子機の品目リスト読み込み
uint32_t CFPushItemListWrite(void);                          // ＰＵＳＨ 子機の品目リスト書き込み
uint32_t CFPushPersonListRead(void);                         // ＰＵＳＨ 作業者リスト読み込み
uint32_t CFPushPersonListWrite(void);                        // ＰＵＳＨ 作業者リスト書き込み
uint32_t CFPushWorkgrpRead(void);                            // ＰＵＳＨ ワークグループ読み込み
uint32_t CFPushWorkgrpWrite(void);                           // ＰＵＳＨ ワークグループ書き込み
uint32_t CFPushMessage(void);                                // ＰＵＳＨ メッセージ表示
uint32_t CFPushRemoteMeasure(void);                          // ＰＵＳＨ リモート測定
uint32_t CFPushClock(void);                                  // ＰＵＳＨ 時計設定

uint32_t  CFTestLAN(void);                                   // LAN通信テスト for USB
uint32_t  CFTestWarning(void);                               // 警報の通信テスト
uint32_t  CFWait(void);                                      // 自律動作の一時停止

uint32_t  CFEventDownload(void);                             // データ吸い上げイベント


uint32_t CFKensa(void);
uint32_t CFInspect(void);
//未使用uint32_t CFDebug(void);
uint32_t CFReadSystemInfo(void);
uint32_t CFReadSFlash(void);

uint16_t sram_inspc(void);

uint32_t CFR500Special(void);        // R500無線モジュール直接SOHコマンド送受信

uint32_t CFWriteFirmData(void);          // [EFIRM] ファームデータ書き込み（キャッシュ）   t.saito
uint32_t CFRoleChange(void);         //[EBROL]   BLEクライアント、サーバー切り替え


//各関数の共通部をサブ関数化 t.saito
uint32_t Sub_CFSearch(int data_size, int print_sw);
uint32_t Sub_Act_2(void);
void uncmpressData(int sw);
void Sub_CF_DataProcess(int maxDataSize, int sw);

//光通信 呼び出し関数　opt_cmd_thread_entry.hから移動、static化
uint32_t IR_Nomal_command(uint8_t Cmd, uint8_t SubCmd, uint16_t length, char *pTxData);     // 光通信　通常コマンド
uint32_t IR_data_download_t(char *, uint8_t, uint8_t, uint64_t);     // 光通信　データ吸い上げ(1024byte毎)
//uint32_t IR_data_block_read(void);                          // 光通信　データブロック要求
uint32_t IR_header_read(uint8_t, uint16_t);                 // 光通信　データ吸い上げ要求
uint32_t IR_64byte_write(char, char *);               // 光通信　６４バイト情報を書き込む
uint32_t IR_64byte_read(char, char *);                // 光通信　６４バイト情報を読み込
//uint32_t IR_read_type(void);
uint32_t IR_soh_direct(comSOH_t *pTx, uint32_t length);                               // 光通信　SOHダイレクトコマンド
uint32_t IR_Regist(void);

uint32_t opt_command_exec(void);                         //T2からの光通信コマンド実行


#endif /* COMP_CMD_SUB_H_ */
