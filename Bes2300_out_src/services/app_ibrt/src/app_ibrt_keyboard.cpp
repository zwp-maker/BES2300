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
#include "app_battery.h"
#include "app_factory.h"

void synchronized_power_off(void);//jevin 同步关机
uint8_t Com_key;//jackdong  组合按键标志位
void app_KeyboardTimer_init(void);
void app_KeyboardTimer_stop(void);
static void app_KeyboardTimer_start(uint16_t Check_Time);
static void app_KeyboardTimer_handler(void const *param);
osTimerDef (APP_KEYBOARDTIMER,app_KeyboardTimer_handler);
static osTimerId app_KeyboardTimer = NULL;
extern "C" void app_bt_accessmode_set(  btif_accessible_mode_t mode);
extern "C" int nv_record_enum_latest_two_paired_dev(btif_device_record_t* record1,btif_device_record_t* record2);
void btdrv_enable_dut(void);//进入dut模式
extern "C" void app_ibrt_nvrecord_rebuild(void);//恢复出厂设置
void app_ibrt_remove_phone_paired_device(void);
void app_ibrt_remove_history_paired_device(void);

//下一曲上一曲
void app_ibrt_ui_audio_forward_test(void);
void app_ibrt_ui_audio_backward_test(void);
//获取音乐状态
uint8_t avrcp_get_media_status(void);

#if defined(IBRT)
extern struct BT_DEVICE_T  app_bt_device;
extern void app_bt_volumedown();
extern void app_bt_volumeup();
extern int hfp_volume_get(enum BT_DEVICE_ID_T id);
extern int a2dp_volume_get(enum BT_DEVICE_ID_T id);
extern void app_otaMode_enter(APP_KEY_STATUS *status, void *param);

#ifdef SUPPORT_SIRI
//extern int app_hfp_siri_report();
extern int app_hfp_siri_voice(bool en);
extern int open_siri_flag;
#endif

/**
 * @description: 初始化组合按键定时器
 * @param {*}
 * @return {*}
 * @author: jackdong
 */

 void app_KeyboardTimer_init(void)
{
        Com_key =0x00;
      if (app_KeyboardTimer == NULL)
        app_KeyboardTimer = osTimerCreate (osTimer(APP_KEYBOARDTIMER), osTimerOnce, NULL);        
}
/**
 * @description: 启动组合按键定时器
 * @param {*}
 * @return {*}
 * @author: jackdong
 */

static void app_KeyboardTimer_start(uint16_t Check_Time)
{
    TRACE("%s", __func__);
    osTimerStart(app_KeyboardTimer,Check_Time);
}


/**
 * @description: 清除标志以及停止定时器
 * @param {*}
 * @return {*}
 * @author: jackdong
 */
 void app_KeyboardTimer_stop(void)
{
    TRACE("%s", __func__);
    Com_key =0x00;
    osTimerStop(app_KeyboardTimer);
}


 /**
  * @description: 按键定时超时句柄
  * @param {*}
  * @return {*}
  * @author: jackdong
  */
 static void app_KeyboardTimer_handler(void const *param)
 {
     TRACE("%s", __func__);
     TRACE("Com_key timeout!!!");
    Com_key =0x00;
    osTimerStop(app_KeyboardTimer);
 }



void app_ibrt_sync_volume_info()
{
    if(app_tws_ibrt_tws_link_connected())
    {
        ibrt_volume_info_t ibrt_volume_req;//
        ibrt_volume_req.a2dp_volume = a2dp_volume_get(BT_DEVICE_ID_1);
        ibrt_volume_req.hfp_volume = hfp_volume_get(BT_DEVICE_ID_1);
        tws_ctrl_send_cmd(APP_TWS_CMD_SYNC_VOLUME_INFO,(uint8_t*)&ibrt_volume_req, sizeof(ibrt_volume_req));
    }
}

bool app_hfp_siri_is_active(void){
	TRACE("The open_siri_flage is %d\n",open_siri_flag);
	return open_siri_flag;
}


#ifdef IBRT_SEARCH_UI
void app_ibrt_search_ui_handle_key(APP_KEY_STATUS *status, void *param)
{ 
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    TRACE("%s status->code=:%d,status->event=:%d,Com_key=:%d",__func__, status->code, status->event,Com_key);
	TRACE("hf_callheld:%d\n",app_bt_device.hf_callheld[BT_DEVICE_ID_1]);
	TRACE("hfchan_call:%d\n",app_bt_device.hfchan_call[BT_DEVICE_ID_1]);
	TRACE("hf_audio_state:%d\n",app_bt_device.hf_audio_state[BT_DEVICE_ID_1]);
	TRACE("hfchan_callSetup:%d\n",app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1]);
	TRACE("a2dp_state:%d\n",app_bt_device.a2dp_state[BT_DEVICE_ID_1]);
	TRACE("a2dp_streamming:%d\n",app_bt_device.a2dp_streamming[BT_DEVICE_ID_1]);

	static bool click_flag = true;
    if (APP_KEY_CODE_GOOGLE != status->code)
    {
        switch(status->event)
        {
			//单击事件(click)	 
            case APP_KEY_EVENT_CLICK:{
				TRACE("APP_KEY_EVENT_CLICK!!\n\n");
				/*if(app_bt_device.hf_callheld[BT_DEVICE_ID_1]==BTIF_HF_CALL_HELD_ACTIVE){
					TRACE("switch AG1/AG2 +++++++++++\n\n");
					//AG1/AG2 切换
               		hfp_handle_key(HFP_KEY_THREEWAY_HOLD_AND_ANSWER);
                }else if(app_bt_device.hf_callheld[BT_DEVICE_ID_1]==BTIF_HF_CALL_HELD_NONE){
					if(app_bt_device.hfchan_call[BT_DEVICE_ID_1] == BTIF_HF_CALL_ACTIVE){
						if(!app_bt_device.phone_earphone_mark){
							TRACE("change to Phone+++++++++++\n\n");
							hfp_handle_key(HFP_KEY_CHANGE_TO_PHONE);//耳机切换到手机
						}else if(app_bt_device.phone_earphone_mark){
							TRACE("change to EarPhone+++++++++++\n\n");
							hfp_handle_key(HFP_KEY_ADD_TO_EARPHONE);//手机切换到耳机
						}	
					}
				}*/
				TRACE("app_tws_ibrt_mobile_link_connected return is %s \n",app_tws_ibrt_mobile_link_connected()?"True":"False");
				TRACE("app_tws_ibrt_slave_ibrt_link_connected return is %s\n",app_tws_ibrt_slave_ibrt_link_connected()?"True":"False");				
				TRACE("app_tws_ibrt_tws_link_connected return is %s\n",app_tws_ibrt_tws_link_connected()?"True":"False");
				TRACE("current role is %x/ nv_role is %x",p_ibrt_ctrl->current_role,p_ibrt_ctrl->nv_role);
				if(click_flag){
					TRACE("Hello world +++++++\n\n");
					app_ibrt_enter_limited_mode();
					osDelay(1000);
					app_start_tws_serching_direactly();
					click_flag = false;	
				}		
			}break;
			//双击事件(double click)
            case APP_KEY_EVENT_DOUBLECLICK:{
				TRACE("APP_KEY_EVENT_DOUBLECLICK!!");
				//耳机在盒内     
				if(get_plugin_state()==APP_BATTERY_CHARGER_PLUGIN){
					break;
              	}
				//如果已经建立手机连接
				if(app_tws_ibrt_mobile_link_connected()){
					if(app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1]==BTIF_HF_CALL_SETUP_IN){
						if(app_bt_device.hfchan_call[BT_DEVICE_ID_1]==BTIF_HF_CALL_ACTIVE){	
							TRACE("Hold AG1 and Answer AG2+++++++\n");
							hfp_handle_key(HFP_KEY_THREEWAY_HOLD_AND_ANSWER);//保持AG1接通AG2
						}else{
							if(app_bt_device.hf_callheld[BT_DEVICE_ID_1]==BTIF_HF_CALL_HELD_NONE){
								TRACE("Answer AG1++++++++\n\n");
								hfp_handle_key(HFP_KEY_ANSWER_CALL);//接听电话
								TRACE("The Hold State is++++++++%d\n\n",app_bt_device.hf_callheld[BT_DEVICE_ID_1]);
							}	
						}
						break;
					}
					if(app_tws_is_left_side()^keyeventsFromSlave){
						//当前是否在播放音乐
						if(app_bt_device.a2dp_state[BT_DEVICE_ID_1]){
							TRACE("Music is ongoing++++++ %d\n\n",app_bt_device.a2dp_state[BT_DEVICE_ID_1]);
							//播放暂停
							if(app_bt_device.a2dp_play_pause_flag){
								TRACE("Pause Music+++++++\n\n");
								a2dp_handleKey(AVRCP_KEY_PAUSE);//暂停
							}else{
								TRACE("Play Music++++++++\n\n");
								a2dp_handleKey(AVRCP_KEY_PLAY);//播放
							}
						}
					}else{
						//唤醒siri
						if((app_bt_device.hf_callheld[BT_DEVICE_ID_1]==BTIF_HF_CALL_HELD_NONE)
							&& (!app_bt_device.a2dp_streamming[BT_DEVICE_ID_1])){
							TRACE("app_hfp_siri_is_active value is %s \n",app_hfp_siri_is_active()?"True":"False");
							if(!app_hfp_siri_is_active()){
								TRACE("Open Siri !!!!!!!\n");
								app_hfp_siri_voice(true);
							}else{
								TRACE("Close Siri!!!!!!!\n");
								app_hfp_siri_voice(false);
							}
						}	
					}
				}
			}break;

			//三击事件(triple click)
            case APP_KEY_EVENT_TRIPLECLICK:{
				TRACE("APP_KEY_EVENT_TRIPLECLICK!!");
				if(get_plugin_state()==APP_BATTERY_CHARGER_PLUGIN){//耳机在盒内     
                	break;
              	}
				click_flag = true;	///////////////////记得要删掉
				app_ibrt_remove_history_paired_device();
                app_ibrt_remove_phone_paired_device();
				app_ibrt_nvrecord_rebuild();//恢复出厂设置
            }break;			
			//长按事件(2s)
            case APP_KEY_EVENT_LONGPRESS:{
				TRACE("APP_KEY_EVENT_LONGPRESS!!");
            	if(get_plugin_state()==APP_BATTERY_CHARGER_PLUGIN){//耳机在盒内
            		break;
              	} 
				//挂断电话
				if(app_bt_device.hfchan_call[BT_DEVICE_ID_1]==BTIF_HF_CALL_ACTIVE){
					TRACE("HangUp the Call\n\n");
					hfp_handle_key(HFP_KEY_HANGUP_CALL);
					break;
				}
				//判断当前状态是否为播放状态
				if(app_bt_device.a2dp_state[BT_DEVICE_ID_1]){
					//切换上一曲/下一曲
					if(app_tws_is_left_side() ^ keyeventsFromSlave){
						TRACE("Play Forward++++++++\n\n");
						a2dp_handleKey(AVRCP_KEY_FORWARD);//上一曲
					}else{
						TRACE("Play Backward++++++++\n\n");
						a2dp_handleKey(AVRCP_KEY_BACKWARD);//下一曲
					}
				}	
			}break;
			//超长按事件(5s)
            case APP_KEY_EVENT_LONGLONGPRESS:{
            	TRACE("APP_KEY_EVENT_LONGLONGPRESS!!");
                app_voice_report(APP_STATUS_INDICATION_POWERON, 0);//调试音测试
	            if(get_plugin_state()==APP_BATTERY_CHARGER_PLUGIN){//耳机在盒内      
	            	break;
	            }
                if(Com_key==0x00){//前面没有触发4击事件
                    TRACE(" 5S ABC!!");
                    Com_key=0x01;
                }
            }break;
			//超超长按事件(10s)
            case APP_KEY_EVENT_LONGLONGLONGPRESS:{
            	TRACE("APP_KEY_EVENT_LONGLONGLONGPRESS");     
                if(get_plugin_state()==APP_BATTERY_CHARGER_PLUGIN){//耳机在盒内      
                	break;
				}           
                app_ibrt_remove_history_paired_device();
                app_ibrt_remove_phone_paired_device();
				
                app_ibrt_nvrecord_rebuild();//恢复出厂设置
                app_shutdown();
                //nv_record_rebuild();
			}break;

            case APP_KEY_EVENT_ULTRACLICK:{//4击+长按
            	TRACE("APP_KEY_EVENT_ULTRACLICK");
                if(get_plugin_state()==APP_BATTERY_CHARGER_PLUGIN){//耳机在盒内     
                	break;
              	} 
                if(Com_key==0x00){//前面没有触发4击事件
               		app_KeyboardTimer_start(7500);//jakcddong  防止按键困难+1.5秒判断超时
                    Com_key=0x02;
                }
            }break;					
			case APP_KEY_EVENT_UP_AFTER_LONGPRESS:{//
                 if(Com_key==0x01){   
                    app_KeyboardTimer_start(3500);//2秒无法按出效果
                    TRACE(" APP_KEY_EVENT_UP!!");
                 }
            }break;
			//底层没有返回该 event 的处理过程,需要自行添加处理
            case APP_KEY_EVENT_RAMPAGECLICK:{
				TRACE("rampage kill! you are crazy!");
            }break;
			default:{
				TRACE("the key event do nothing");
			}break;

        }
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
/**
 * @description: 发生按键cmd给对耳
 * @param {APP_KEY_STATUS} *status 所需处理的状态
 * @param {void} *param
 * @return {*}
 * @author: Jevin
 */
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
/**
 * @description: 从耳接收到的按键事件
 * @param {uint16_t} rsp_seq
 * @param {uint8_t} *p_buff 传输数据的buffer区
 * @param {uint16_t} length 数据的字长
 * @return {*}
 * @author: Jevin
 */
void app_ibrt_keyboard_request_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length)
{
    if (app_tws_ibrt_mobile_link_connected())
    {   
        keyeventsFromSlave=true;
#ifdef IBRT_SEARCH_UI
        app_ibrt_search_ui_handle_key((APP_KEY_STATUS *)p_buff, NULL);
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

