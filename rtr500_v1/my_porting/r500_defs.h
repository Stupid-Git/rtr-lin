/*
 * r500_defs.h
 *
 *  Created on: Nov 25, 2024
 *      Author: karel
 */

#ifndef R500_DEFS_H_
#define R500_DEFS_H_

#include "_r500_config.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>


#if CONFIG_USE_TD_KON_STUB
#include "td_kon_stub.h"
#endif
#if CONFIG_USE_TD_KON_KON
#include "td_kon_kon.h"
#endif

#include "tdx.h"

#include "version.h"
#include "glb_1.h"
#include "General.h"
#include "Globals.h"
#include "MyDefine.h"
#include "flag_def.h"
#include "Error.h"
#include "Lang.h"

// common/inc
#include "Xml.h"


#include "Warning.h"

#include "comp_log.h"
#include "Cmd_func.h"

#include "base.h"


#include "random_stuff.h"
#include "Cmd_func.h"

#include "comp_cmd.h"
#include "comp_cmd_sub.h"


#include "comp_gos.h"
#include "comp_datetime.h"

#include "comp_ble_thread.h"
#include "comp_led_thread.h"
#include "comp_opto_thread.h"
#include "comp_rf_thread.h"
#include "comp_auto_thread.h"




#endif /* R500_DEFS_H_ */
