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
#include "cmsis_os.h"
#include <string.h>
#include "app_ibrt_keyboard.h"
#include "app_tws_ibrt_trace.h"
#include "app_ibrt_if.h"
#include "app_ibrt_ui_test.h"
#include "app_tws_ibrt_cmd_handler.h"
#include "app_tws_ctrl_thread.h"
#include "btapp.h"
#include "apps.h"
#include "app_tws_if.h"
#include "app_factory.h"

#if defined(IBRT)
extern struct BT_DEVICE_T  app_bt_device;
extern void app_bt_volumedown();
extern void app_bt_volumeup();
extern int hfp_volume_get(enum BT_DEVICE_ID_T id);
extern int a2dp_volume_get(enum BT_DEVICE_ID_T id);
extern void app_otaMode_enter(APP_KEY_STATUS *status, void *param);

#ifdef SUPPORT_SIRI
extern int app_hfp_siri_voice(bool en);
extern int open_siri_flag;
#endif


void app_ibrt_sync_volume_info()
{
    if(app_tws_ibrt_tws_link_connected())
    {
        ibrt_volume_info_t ibrt_volume_req;
        ibrt_volume_req.a2dp_volume = a2dp_volume_get(BT_DEVICE_ID_1);
        ibrt_volume_req.hfp_volume = hfp_volume_get(BT_DEVICE_ID_1);
        tws_ctrl_send_cmd(APP_TWS_CMD_SYNC_VOLUME_INFO,(uint8_t*)&ibrt_volume_req, sizeof(ibrt_volume_req));
    }
}


extern void app_ibrt_remove_history_paired_device(void);
extern void app_ibrt_remove_phone_paired_device(void);

#ifdef IBRT_SEARCH_UI
static bool keyeventsFromSlave = false;	//从耳是否有发送按键事件给主耳处理
static uint8_t eq_index = 0;
typedef enum {
    AUDIO_EQ_TYPE_SW_IIR = 0,
    AUDIO_EQ_TYPE_HW_FIR,
    AUDIO_EQ_TYPE_HW_DAC_IIR,
    AUDIO_EQ_TYPE_HW_IIR,
} AUDIO_EQ_TYPE_T;
extern uint32_t bt_audio_set_eq(AUDIO_EQ_TYPE_T audio_eq_type, uint8_t index);//EQ切换接口
extern "C" void app_ibrt_ui_rssi_process(void);
void app_ibrt_tws_mode_key_handler(APP_KEY_STATUS *status, void *param){//双耳按键处理
	//ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    TRACE("%s %d,%d",__func__, status->code, status->event);
	if (APP_KEY_CODE_GOOGLE != status->code){
		switch(status->event)
		{
            case APP_KEY_EVENT_CLICK:{
                TRACE("Single Click !!!!!!");
				if(app_bt_device.hf_callheld[BT_DEVICE_ID_1]==BTIF_HF_CALL_HELD_ACTIVE){
					if(app_bt_device.hfchan_call[BT_DEVICE_ID_1]==BTIF_HF_CALL_ACTIVE){
						hfp_handle_key(HFP_KEY_THREEWAY_HOLD_AND_ANSWER);//AG1/AG2 切换
						break;
					}
                }else if(app_bt_device.hf_callheld[BT_DEVICE_ID_1]==BTIF_HF_CALL_HELD_NONE){
					if(app_bt_device.hfchan_call[BT_DEVICE_ID_1]==BTIF_HF_CALL_ACTIVE){
						if(app_bt_device.phone_earphone_mark){
							hfp_handle_key(HFP_KEY_ADD_TO_EARPHONE);//手机切换到耳机
							break;
						}else{
							hfp_handle_key(HFP_KEY_CHANGE_TO_PHONE);//耳机切换到手机
							break;
						}	
					}
				}
            }break;

            case APP_KEY_EVENT_DOUBLECLICK:{
                TRACE("Double Click !!!!!!");
				if(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1]==BTIF_HF_CALL_SETUP_IN){
					app_ibrt_sync_volume_info();//左右耳 与 设备 音量同步
					hfp_handle_key(HFP_KEY_ANSWER_CALL);	
					break;
				}
				if(LR_flags^keyeventsFromSlave){
					//右耳功能
					if(app_bt_device.a2dp_play_pause_flag){
						a2dp_handleKey(AVRCP_KEY_PAUSE);//暂停
					}else{
						a2dp_handleKey(AVRCP_KEY_PLAY);//播放
					}
				}else{
					//左耳功能
					open_siri_flag = ~open_siri_flag;
					app_hfp_siri_voice(open_siri_flag);				
				}

           	}break;
				
			case APP_KEY_EVENT_TRIPLECLICK:{
            	TRACE("Triple Click !!!!!!");
				app_voice_report(AK_APP_STATUS_INDICATION_EQ_SWITCH, BT_DEVICE_ID_1);//添加Anker EQ切换提示音	zwp
				if(eq_index>=EQ_HW_DAC_IIR_LIST_NUM-1){
					eq_index = 0;
				}else{
					++eq_index;
				}
				bt_audio_set_eq(AUDIO_EQ_TYPE_HW_DAC_IIR,eq_index);//EQ切换
            }break;
			   
            case APP_KEY_EVENT_LONGPRESS:{
				TRACE("Long Press 2s!!!!!!");
				if(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1]==BTIF_HF_CALL_SETUP_IN){//来电状态
					if(app_bt_device.hfchan_call[BT_DEVICE_ID_1]==BTIF_HF_CALL_ACTIVE){//通话状态	
						if(app_bt_device.hf_callheld[BT_DEVICE_ID_1]==BTIF_HF_CALL_HELD_ACTIVE){//通话保持
							hfp_handle_key(HFP_KEY_THREEWAY_HANGUP_AND_ANSWER);//挂断AG2切换AG1
							break;
						}else{
							hfp_handle_key(HFP_KEY_THREEWAY_HOLD_REL_INCOMING);//保持AG1挂断AG2
							break;
						}
					}else{
						hfp_handle_key(HFP_KEY_HANGUP_CALL);//挂断电话
						break;
					}
				}else if(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1]==BTIF_HF_CALL_SETUP_NONE){
					if(app_bt_device.hfchan_call[BT_DEVICE_ID_1]==BTIF_HF_CALL_ACTIVE){	
						hfp_handle_key(HFP_KEY_HANGUP_CALL);//挂断电话
						break;
					}
				}
				//是否处于播放状态
				if(app_bt_device.a2dp_state[BT_DEVICE_ID_1]){
					if(LR_flags^keyeventsFromSlave){
						//右耳功能
						a2dp_handleKey(AVRCP_KEY_FORWARD);//下一曲
					}else{
						//左耳功能
						a2dp_handleKey(AVRCP_KEY_BACKWARD);//上一曲
					}
				}
            }break;

			default:{
				TRACE("%s %d,%d",__func__, status->code, status->event);
			}break;
        }
    }
}
void app_ibrt_mono_mode_key_handler(APP_KEY_STATUS *status, void *param){//单耳按键处理
	//ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
	TRACE("%s %d,%d",__func__, status->code, status->event);
	if (APP_KEY_CODE_GOOGLE != status->code){
		switch(status->event)
		{
            case APP_KEY_EVENT_CLICK:{
                TRACE("\nSingle Click !!!!!!\n");
            }break;

            case APP_KEY_EVENT_DOUBLECLICK:{
                TRACE("\nDouble Click !!!!!!\n");
				if(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1]==BTIF_HF_CALL_SETUP_IN){
					hfp_handle_key(HFP_KEY_ANSWER_CALL);
					break;
				}
				if(app_bt_device.a2dp_play_pause_flag){
					a2dp_handleKey(AVRCP_KEY_PAUSE);//暂停
				}else{
					a2dp_handleKey(AVRCP_KEY_PLAY);//播放
				}				
           	}break;
				
			case APP_KEY_EVENT_TRIPLECLICK:{
				TRACE("\nTriple Click !!!!!!\n");
				//开启/关闭 语音助手
				open_siri_flag = ~open_siri_flag;
				app_hfp_siri_voice(open_siri_flag);
            }break;
			   
            case APP_KEY_EVENT_LONGPRESS:{
				TRACE("\nLong Press !!!!!!\n");
				if((app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1]==BTIF_HF_CALL_SETUP_IN)||
					(app_bt_device.hfchan_call[BT_DEVICE_ID_1]==BTIF_HF_CALL_ACTIVE)){
					hfp_handle_key(HFP_KEY_HANGUP_CALL);
					break;
				}
            	//是否处于播放状态
				if(app_bt_device.a2dp_streamming[BT_DEVICE_ID_1]){
					a2dp_handleKey(AVRCP_KEY_FORWARD);//下一曲
				}
            }break;

			default:{
				TRACE("%s %d,%d",__func__, status->code, status->event);
			}break;
        }
	}
}

extern bool key_enter_dut_mode_flag_0;
extern osTimerId key_enter_dut_mode_timer_handle;
void app_ibrt_idle_mode_key_handler(APP_KEY_STATUS *status, void *param){
	TRACE("%s %d,%d",__func__, status->code, status->event);
	if (APP_KEY_CODE_GOOGLE != status->code){
		switch(status->event)
		{
			case APP_KEY_EVENT_QUADRACLICK:{
				TRACE("\nQuadra Click !!!!!!\n");
				if(!key_enter_dut_mode_flag_0){
					osTimerStart(key_enter_dut_mode_timer_handle,7000);
					key_enter_dut_mode_flag_0 = true;
				}
			}break;
			
			case APP_KEY_EVENT_LONGPRESS_5s:{
				TRACE("\nLong Press 5s!!!!!!\n");
				if(key_enter_dut_mode_flag_0){
					app_factorymode_enter();//enter DUT mode
				}
			}break;
			
			default:{
				TRACE("%s %d,%d",__func__, status->code, status->event);
			}break;
		}
	}
}
extern void synchronized_power_off(void);
void app_ibrt_search_ui_handle_key(APP_KEY_STATUS *status, void *param)
{  
	//ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
	TRACE("%s %d,%d",__func__, status->code, status->event);
	if(status->event == APP_KEY_EVENT_LONGLONGPRESS){
		TRACE("\nLong Long Press !!!!!!\n");
		//app_ibrt_remove_history_paired_device();
		//app_ibrt_remove_phone_paired_device();
		synchronized_power_off();//if tws connected，synchronized power off
		return;
	}
	if(app_tws_ibrt_tws_link_connected()&&app_tws_ibrt_mobile_link_connected()){//TWS_Link and Mobile_Link is active
		app_ibrt_tws_mode_key_handler(status,param);//双耳模式			
	}else if(app_tws_ibrt_mobile_link_connected()){
		app_ibrt_mono_mode_key_handler(status,param);//MONO模式			
	}else{
		app_ibrt_idle_mode_key_handler(status,param);//IDLE模式,无TWS配对,无BT配对
	} 
}
#endif

void app_ibrt_normal_ui_handle_key(APP_KEY_STATUS *status, void *param)
{
    if (APP_KEY_CODE_GOOGLE != status->code)
    {
        switch(status->event)
        {
            case APP_KEY_EVENT_CLICK:
                TRACE("first blood.");
                bt_key_handle_func_click();
                break;

            case APP_KEY_EVENT_DOUBLECLICK:
                TRACE("double kill");
                app_ibrt_if_enter_freeman_pairing();
                break;

            case APP_KEY_EVENT_LONGPRESS:
                app_ibrt_if_enter_pairing_after_tws_connected();
                break;

            case APP_KEY_EVENT_TRIPLECLICK:
                break;

            case HAL_KEY_EVENT_LONGLONGPRESS:
                TRACE("long long press");
                app_shutdown();
                break;

            case APP_KEY_EVENT_ULTRACLICK:
                TRACE("ultra kill");
                break;

            case APP_KEY_EVENT_RAMPAGECLICK:
                TRACE("rampage kill!you are crazy!");
                break;

            case APP_KEY_EVENT_UP:
                break;
        }
    }
}

int app_ibrt_if_keyboard_notify(APP_KEY_STATUS *status, void *param)
{
    if (app_tws_ibrt_slave_ibrt_link_connected())
    {
        tws_ctrl_send_cmd(APP_TWS_CMD_KEYBOARD_REQUEST, (uint8_t *)status, sizeof(APP_KEY_STATUS));
    }
    return 0;
}

void app_ibrt_send_keyboard_request(uint8_t *p_buff, uint16_t length)
{
    if (app_tws_ibrt_slave_ibrt_link_connected())
    {
        app_ibrt_send_cmd_without_rsp(APP_TWS_CMD_KEYBOARD_REQUEST, p_buff, length);
    }
}

void app_ibrt_keyboard_request_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if (app_tws_ibrt_mobile_link_connected())
    {
#ifdef IBRT_SEARCH_UI
		keyeventsFromSlave = true;
        app_ibrt_search_ui_handle_key((APP_KEY_STATUS *)p_buff, NULL);
		keyeventsFromSlave = false;
#else
        app_ibrt_normal_ui_handle_key((APP_KEY_STATUS *)p_buff, NULL);
#endif

        app_ibrt_ui_test_voice_assistant_key((APP_KEY_STATUS *)p_buff, NULL);
    }
}

void app_ibrt_if_start_user_action(uint8_t *p_buff, uint16_t length)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    TRACE("%s role %d code %d",__func__, p_ibrt_ctrl->current_role, p_buff[0]);

    if (IBRT_SLAVE == p_ibrt_ctrl->current_role)
    {
        app_ibrt_ui_send_user_action(p_buff, length);
    }
    else
    {
        app_ibrt_ui_perform_user_action(p_buff, length);
    }
}

extern void app_bt_volumeup();
extern void app_bt_volumedown();

void app_ibrt_ui_perform_user_action(uint8_t *p_buff, uint16_t length)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    if (IBRT_MASTER != p_ibrt_ctrl->current_role)
    {
        TRACE("%s not ibrt master, skip cmd %02x\n", __func__, p_buff[0]);
        return;
    }

    switch (p_buff[0])
    {
        case IBRT_ACTION_PLAY:
            a2dp_handleKey(AVRCP_KEY_PLAY);
            break;
        case IBRT_ACTION_PAUSE:
            a2dp_handleKey(AVRCP_KEY_PAUSE);
            break;
        case IBRT_ACTION_FORWARD:
            a2dp_handleKey(AVRCP_KEY_FORWARD);
            break;
        case IBRT_ACTION_BACKWARD:
            a2dp_handleKey(AVRCP_KEY_BACKWARD);
            break;
        case IBRT_ACTION_AVRCP_VOLUP:
            a2dp_handleKey(AVRCP_KEY_VOLUME_UP);
            break;
        case IBRT_ACTION_AVRCP_VOLDN:
            a2dp_handleKey(AVRCP_KEY_VOLUME_DOWN);
            break;
        case IBRT_ACTION_HFSCO_CREATE:
            if (!btif_hf_audio_link_is_up())
            {
                btif_pts_hf_create_audio_link();
            }
            else
            {
                TRACE("%s cannot create audio link\n", __func__);
            }
            break;
        case IBRT_ACTION_HFSCO_DISC:
            if (btif_hf_audio_link_is_up())
            {
                btif_pts_hf_disc_audio_link();
            }
            else
            {
                TRACE("%s cannot disc audio link\n", __func__);
            }
            break;
        case IBRT_ACTION_REDIAL:
            if (btif_hf_service_link_is_up())
            {
                btif_pts_hf_redial_call();
            }
            else
            {
                TRACE("%s cannot redial call\n", __func__);
            }
            break;
        case IBRT_ACTION_ANSWER:
            if (btif_hf_service_link_is_up())
            {
                btif_pts_hf_answer_call();
            }
            else
            {
                TRACE("%s cannot answer call\n", __func__);
            }
            break;
        case IBRT_ACTION_HANGUP:
            if (btif_hf_service_link_is_up())
            {
                btif_pts_hf_hangup_call();
            }
            else
            {
                TRACE("%s cannot hangup call\n", __func__);
            }
            break;
        case IBRT_ACTION_LOCAL_VOLUP:
            app_bt_volumeup();
            app_ibrt_sync_volume_info();
            break;
        case IBRT_ACTION_LOCAL_VOLDN:
            app_bt_volumedown();
            app_ibrt_sync_volume_info();
            break;
        default:
            TRACE("%s unknown user action %d\n", __func__, p_buff[0]);
            break;
    }
}

#endif

