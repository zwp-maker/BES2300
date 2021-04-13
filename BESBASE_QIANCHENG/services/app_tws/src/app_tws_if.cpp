/***************************************************************************
*
*Copyright 2015-2019 BES.
*All rights reserved. All unpublished rights reserved.
*
*No part of this work may be used or reproduced in any form or by any
*means, or stored in a database or retrieval system, without prior written
*permission of BES.
*
*Use of this work is governed by a license granted by BES.
*This work contains confidential and proprietary information of
*BES. which is protected by copyright, trade secret,
*trademark and other intellectual property rights.
*
****************************************************************************/

/*****************************header include********************************/
#include "cmsis.h"
#include "hal_trace.h"
#include "me_api.h"
#include "besbt.h"
#include "app.h"
#include "app_tws_ctrl_thread.h"
#include "app_tws_ibrt_cmd_handler.h"
#include "app_tws_if.h"
#include "btapp.h"
#include "nvrecord_env.h"
#include "nvrecord_extension.h"
#include "nvrecord_ble.h"

#ifdef __THIRDPARTY
#include "app_thirdparty.h"
#endif

#ifdef IBRT
#include "app_ibrt_customif_ui.h"
#endif

#ifdef BLE_ENABLE
#include "app_ble_mode_switch.h"
#endif

#ifdef VOICE_DATAPATH
#include "app_voicepath.h"
#endif

#ifdef GSOUND_ENABLED
#include "gsound_target_tws_app.h"
#endif

#ifdef __AI_VOICE__
#include "ai_thread.h"
#endif

#ifdef _GFPS_
#include "nvrecord_fp_account_key.h"
#include "app_gfps.h"
#endif

/************************private macro defination***************************/

/************************private type defination****************************/

/**********************private function declearation************************/
extern "C" void app_sec_init(void);

/************************private variable defination************************/

/****************************function defination****************************/
void app_tws_if_role_switch_started_handler(void)
{
    TRACE("[%s]+++", __func__);

    TRACE("[%s]---", __func__);
}

void app_tws_if_tws_role_switch_complete_handler(uint8_t newRole)
{
    TRACE("[%s]+++ role %d", __func__, newRole);

    TRACE("[%s]---", __func__);
}

void app_tws_if_tws_disconnected_handler(void)
{
    TRACE("tws disconnected.");
}

