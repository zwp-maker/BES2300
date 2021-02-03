/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/
#include "hal_trace.h"
#include "hal_timer.h"
#include "app_audio.h"
#include "app_utils.h"
#include "hal_aud.h"
#include "string.h"
#include "cmsis_os.h"
#include "app_tota.h"
#include "app_tota_cmd_code.h"
#include "app_tota_cmd_handler.h"
#include "app_tota_data_handler.h"
#include "app_spp_tota.h"
#include "cqueue.h"
#include "app_ble_rx_handler.h"
#include "rwapp_config.h"
#include "btapp.h"
#include "app_bt.h"
#include "apps.h"
#include "app_thread.h"
#include "cqueue.h"
#include "hal_location.h"
#include "app_hfp.h"
#include "bt_drv_reg_op.h"
#if defined(IBRT) 
#include "app_tws_ibrt.h"
#endif
typedef struct
{
    uint8_t connectedType;
    APP_TOTA_TRANSMISSION_PATH_E dataPath;
} APP_TOTA_ENV_T;

static APP_TOTA_ENV_T app_tota_env= 
    {
        0,
    };

bool app_is_in_tota_mode(void)
{
    return app_tota_env.connectedType;
}

void app_tota_init(void)
{
    TOTA_LOG_DBG("Init application test over the air.");
    app_spp_tota_init();

    app_tota_cmd_handler_init();
    app_tota_data_reset_env();
#if (BLE_APP_TOTA)
    app_ble_rx_handler_init();
#endif
}

extern "C" void bulk_read_done(void);
void app_tota_connected(uint8_t connType)
{
    app_tota_data_reset_env();
    TOTA_LOG_DBG("Tota is connected.");
    app_tota_env.connectedType |= connType;
#if defined(APP_ANC_TEST)
	app_spp_tota_register_tx_done(bulk_read_done);
#endif
}

void app_tota_disconnected(uint8_t disconnType)
{
    TOTA_LOG_DBG("Tota is disconnected.");
    app_tota_env.connectedType &= disconnType;

	app_spp_tota_register_tx_done(NULL);
}

void app_tota_update_datapath(APP_TOTA_TRANSMISSION_PATH_E dataPath)
{
    app_tota_env.dataPath = dataPath;
}

APP_TOTA_TRANSMISSION_PATH_E app_tota_get_datapath(void)
{
    return app_tota_env.dataPath;
}

void app_tota_data_ack_received(uint32_t dataLength)
{
    TOTA_LOG_DBG("%d bytes have been received by the peer device.", dataLength);
}

static void app_tota_demo_cmd_handler(APP_TOTA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    TOTA_LOG_DBG("Func code 0x%x, param len %d", funcCode, paramLen);
    TOTA_LOG_DBG("Param content:");
    DUMP8("%02x ", ptrParam, paramLen);

}

TOTA_COMMAND_TO_ADD(OP_TOTA_DEMO_CMD, app_tota_demo_cmd_handler, false, 0, NULL );

extern "C" int8_t app_battery_current_level(void);

void app_bt_volumeup();
void app_bt_volumedown();
int app_bt_stream_local_volume_get(void);
uint8_t app_audio_get_eq();
int app_audio_set_eq(uint8_t index);
bool app_meridian_eq(bool onoff);
bool app_is_meridian_on();

#if defined(IBRT) && defined(ANC_APP) && defined(DEBUG_ANC_BY_PHONE)
extern "C" bool app_anc_work_status(void);
extern "C" void app_anc_status_post_extend(uint32_t param0, uint32_t param1, uint32_t param2);
extern "C" void app_anc_status_sync_extend(uint32_t param0, uint32_t param1, uint32_t param2);
#endif

static void app_tota_vendor_cmd_handler(APP_TOTA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    TOTA_LOG_DBG("Func code 0x%x, param len %d", funcCode, paramLen);
    TOTA_LOG_DBG("Param content:");
    DUMP8("%02x ", ptrParam, paramLen);
    uint8_t resData[20]={0};
    uint32_t resLen=1;
    switch (funcCode)
    {
        case OP_TOTA_BATTERY_STATUS_CMD:
            {
                uint8_t level = app_battery_current_level();
                resData[0] = level;
                resLen = 1;
                TRACE("battery_current = %d",level);
            }
            break;
        case OP_TOTA_MERIDIAN_EFFECT_CMD:
            resData[0] = app_meridian_eq(ptrParam[0]);
            break;
        case OP_TOTA_EQ_SELECT_CMD:
            break;
        case OP_TOTA_VOLUME_PLUS_CMD:
            {
                app_bt_volumeup();
                uint8_t level = app_bt_stream_local_volume_get();
                resData[0] = level;
                TRACE("volume = %d",level);
            }
            break;
        case OP_TOTA_VOLUME_DEC_CMD:
            {
                app_bt_volumedown();
                uint8_t level = app_bt_stream_local_volume_get();
                resData[0] = level;
                resLen = 1;
                TRACE("volume = %d",level);
            }
            break;
        case OP_TOTA_VOLUME_SET_CMD:
            {
                uint8_t scolevel = ptrParam[0];
                uint8_t a2dplevel = ptrParam[1];
                app_bt_set_volume(APP_BT_STREAM_HFP_PCM,scolevel);
                app_bt_set_volume(APP_BT_STREAM_A2DP_SBC,a2dplevel);
                btapp_hfp_report_speak_gain();
                btapp_a2dp_report_speak_gain();
            }
            break;
        case OP_TOTA_VOLUME_GET_CMD:
            {
                resData[0] = app_bt_stream_hfpvolume_get();
                resData[1] = app_bt_stream_a2dpvolume_get();
                resLen = 2;
            }
            break;
        case OP_TOTA_EQ_SET_CMD:
            {
                int eq_index = ptrParam[0];
                if (eq_index == 3)
                    resData[0] = app_meridian_eq(true);
                else
                    resData[0] = app_audio_set_eq(eq_index);
                resLen = 1;
            }
            break;
        case OP_TOTA_EQ_GET_CMD:
            {
                resData[0] = app_audio_get_eq();
                resLen = 1;
            }
            break;
        case OP_TOTA_FWVERSION_GET_CMD:
            {
                resData[0] = 0xA;
                resData[1] = 0XB;
                resData[2] = 0xC;
                resData[3] = 0XD;
                resLen = 4;
            }
            break;
        case OP_TOTA_RSSI_GET_CMD:
#if defined(IBRT)
            {
                ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
                uint8_t temp;
                resData[0] = p_ibrt_ctrl->raw_rssi.agc_idx0;
                resData[1] = p_ibrt_ctrl->raw_rssi.rssi0;
                resData[2] = p_ibrt_ctrl->raw_rssi.rssi0_max;
                resData[3] = p_ibrt_ctrl->raw_rssi.rssi0_min;

                resData[4] = p_ibrt_ctrl->raw_rssi.agc_idx1;
                resData[5] = p_ibrt_ctrl->raw_rssi.rssi1;
                resData[6] = p_ibrt_ctrl->raw_rssi.rssi1_max;
                resData[7] = p_ibrt_ctrl->raw_rssi.rssi1_min;
                resData[8] = p_ibrt_ctrl->raw_rssi.ser;

                resData[9] = p_ibrt_ctrl->peer_raw_rssi.agc_idx0;
                resData[10] = p_ibrt_ctrl->peer_raw_rssi.rssi0;
                resData[11] = p_ibrt_ctrl->peer_raw_rssi.rssi0_max;
                resData[12] = p_ibrt_ctrl->peer_raw_rssi.rssi0_min;

                resData[13] = p_ibrt_ctrl->peer_raw_rssi.agc_idx1;
                resData[14] = p_ibrt_ctrl->peer_raw_rssi.rssi1;
                resData[15] = p_ibrt_ctrl->peer_raw_rssi.rssi1_max;
                resData[16] = p_ibrt_ctrl->peer_raw_rssi.rssi1_min;
                resData[17] = p_ibrt_ctrl->peer_raw_rssi.ser;

                TRACE("%d %d %d %d : %d %d %d %d %d%<> %d %d %d %d : %d %d %d %d %d%   %d",
                    p_ibrt_ctrl->raw_rssi.agc_idx0,
                    p_ibrt_ctrl->raw_rssi.rssi0,
                    p_ibrt_ctrl->raw_rssi.rssi0_max,
                    p_ibrt_ctrl->raw_rssi.rssi0_min,
                    p_ibrt_ctrl->raw_rssi.agc_idx1,
                    p_ibrt_ctrl->raw_rssi.rssi1,
                    p_ibrt_ctrl->raw_rssi.rssi1_max,
                    p_ibrt_ctrl->raw_rssi.rssi1_min,
                    p_ibrt_ctrl->raw_rssi.ser,
                    p_ibrt_ctrl->peer_raw_rssi.agc_idx0,
                    p_ibrt_ctrl->peer_raw_rssi.rssi0,
                    p_ibrt_ctrl->peer_raw_rssi.rssi0_max,
                    p_ibrt_ctrl->peer_raw_rssi.rssi0_min,
                    p_ibrt_ctrl->peer_raw_rssi.agc_idx1,
                    p_ibrt_ctrl->peer_raw_rssi.rssi1,
                    p_ibrt_ctrl->peer_raw_rssi.rssi1_max,
                    p_ibrt_ctrl->peer_raw_rssi.rssi1_min,
                    p_ibrt_ctrl->peer_raw_rssi.ser,
                    p_ibrt_ctrl->current_role);

                if(p_ibrt_ctrl->nv_role == IBRT_SLAVE){
                    for(int i=0;i<9;i++){
                        temp = resData[i+9];
                        resData[i+9] = resData[i]; 
                        resData[i] = temp; 
                    }
                }
                resData[18] = p_ibrt_ctrl->nv_role;
                resLen = 19;
            }
#endif
            break;
        default:
            TRACE("wrong cmd 0x%x",funcCode);
            resData[0] = -1;
            return;
    }
    app_tota_send_response_to_command(funcCode,TOTA_NO_ERROR,resData,resLen,app_tota_get_datapath());
    return;
}

#if defined(IBRT) && defined(ANC_APP) && defined(DEBUG_ANC_BY_PHONE)

extern "C" int anc_set_gain(int32_t gain_ch_l, int32_t gain_ch_r,enum ANC_TYPE_T anc_type);

static void app_tota_ibrt_anc_cmd_handler(APP_TOTA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    TOTA_LOG_DBG("[%s] Func code 0x%x, param len %d", __func__, funcCode, paramLen);
    TOTA_LOG_DBG("[%s] Param content:", __func__);
    DUMP8("%02x ", ptrParam, paramLen);
    
    uint8_t temp = 0;
    int32_t temp1 = 0;
    
    switch (funcCode)
    {
        case OP_TOTA_ANC_STATUS_SYNC_CMD:
        {
            TRACE("[%s] OP_TOTA_ANC_STATUS_SYNC_CMD, param0=%d", __func__, ptrParam[0]);
            
            temp = !app_anc_work_status();
            app_anc_status_sync_extend(temp, (uint32_t)OP_TOTA_ANC_STATUS_SYNC_CMD, ptrParam[0]);
        }
        break;
        case OP_TOTA_ANC_ONOFF_WRITE_CMD:
        {
            TRACE("[%s] OP_TOTA_ANC_ONOFF_WRITE_CMD param0=%d", __func__, ptrParam[0]);
            temp = !app_anc_work_status();
            app_anc_status_post_extend(temp, (uint32_t)OP_TOTA_ANC_ONOFF_WRITE_CMD, ptrParam[0]);
        }
        break;
        case OP_TOTA_ANC_LEVEL_WRITE_CMD:
        {
            TRACE("[%s] OP_TOTA_ANC_LEVEL_WRITE_CMD param0=%d", __func__, ptrParam[0]);
            temp = !app_anc_work_status();
            app_anc_status_post_extend(temp, (uint32_t)OP_TOTA_ANC_LEVEL_WRITE_CMD, ptrParam[0]);
        }
        break;
        case OP_TOTA_ANC_TOTAL_GAIN_WRITE_CMD:
        {
            temp1 = ptrParam[0];
            temp1 = (temp1<<8)|ptrParam[1];
            temp1 = (temp1<<8)|ptrParam[2];
            temp1 = (temp1<<8)|ptrParam[3];
            
            TRACE("[%s] OP_TOTA_ANC_TOTAL_GAIN_WRITE_CMD param0=%d", __func__, temp1);
            anc_set_gain(temp1, temp1, ANC_FEEDFORWARD);
        }
        break;
        
        default:TRACE("[%s] wrong cmd 0x%x",__func__, funcCode);break;
    }
}
#endif


TOTA_COMMAND_TO_ADD(OP_TOTA_BATTERY_STATUS_CMD, app_tota_vendor_cmd_handler, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_MERIDIAN_EFFECT_CMD, app_tota_vendor_cmd_handler, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_EQ_SELECT_CMD, app_tota_vendor_cmd_handler, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_VOLUME_PLUS_CMD, app_tota_vendor_cmd_handler, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_VOLUME_DEC_CMD, app_tota_vendor_cmd_handler, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_VOLUME_SET_CMD, app_tota_vendor_cmd_handler, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_VOLUME_GET_CMD, app_tota_vendor_cmd_handler, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_EQ_SET_CMD, app_tota_vendor_cmd_handler, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_EQ_GET_CMD, app_tota_vendor_cmd_handler, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_FWVERSION_GET_CMD, app_tota_vendor_cmd_handler, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_RSSI_GET_CMD, app_tota_vendor_cmd_handler, false, 0, NULL );

#if defined(IBRT) && defined(ANC_APP) && defined(DEBUG_ANC_BY_PHONE)
TOTA_COMMAND_TO_ADD(OP_TOTA_ANC_STATUS_SYNC_CMD, app_tota_ibrt_anc_cmd_handler, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_ANC_ONOFF_WRITE_CMD, app_tota_ibrt_anc_cmd_handler, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_ANC_LEVEL_WRITE_CMD, app_tota_ibrt_anc_cmd_handler, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_ANC_STATUS_REPORT_CMD, app_tota_ibrt_anc_cmd_handler, false, 0, NULL );
TOTA_COMMAND_TO_ADD(OP_TOTA_ANC_TOTAL_GAIN_WRITE_CMD, app_tota_ibrt_anc_cmd_handler, false, 0, NULL );
#endif


