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
#include "apps.h"
#include "app_tws_ibrt_trace.h"
#include "app_ibrt_if.h"
#include "app_ibrt_customif_ui.h"
#include "me_api.h"
#include "app_ibrt_ui.h"
#include "app_vendor_cmd_evt.h"
#include "besaud_api.h"
#include "app_battery.h"
#include "app_tws_ibrt_cmd_handler.h"
#include "app_tws_ctrl_thread.h"
#include "app_hfp.h"
#include "app_tws_if.h"
#include "app_bt_media_manager.h"
#include "app_spp.h"
#include "app_bt.h"
extern void bt_disconnect_shadown();
extern osTimerId tws_pairing_timeout_handle;
extern uint8_t tws_pairing_timeout_cnt;
#if defined(IBRT)
#ifdef ANC_APP
extern "C" void tws_anc_sync_status(void);
#endif
/*****************************************************************************
 Prototype    : app_ibrt_ui_tws_switch_according_rssi_needed
 Description  :
 Input        : void
 Output       : bool
 Return Value :
 Calls        :
 Called By    :
 History      :
 Date         : 2019/6/5
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/

void app_ibrt_customif_ui_vender_event_handler_ind(uint8_t evt_type, uint8_t *buffer, uint8_t length)
{
    TRACE(" COSONIC:%s: %d", __func__, __LINE__);
    uint8_t subcode = evt_type;
    POSSIBLY_UNUSED ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

    switch (subcode)
    {
        case HCI_DBG_SNIFFER_INIT_CMP_EVT_SUBCODE:{
			TEST_STATUSMACHINE_TRACE("HCI_DBG_SNIFFER_INIT_CMP_EVT_SUBCODE");
		}break;

        case HCI_DBG_IBRT_CONNECTED_EVT_SUBCODE:{
			TEST_STATUSMACHINE_TRACE("HCI_DBG_IBRT_CONNECTED_EVT_SUBCODE");
		}break;

        case HCI_DBG_IBRT_DISCONNECTED_EVT_SUBCODE:{
			TEST_STATUSMACHINE_TRACE("HCI_DBG_IBRT_DISCONNECTED_EVT_SUBCODE");
		}break;

        case HCI_DBG_IBRT_SWITCH_COMPLETE_EVT_SUBCODE:{

            /*
             *  New Master do some special action,such as update battery to phone,
             * since TWS switch may lead to old TWS master update battery fail
             */
#if defined(SUPPORT_BATTERY_REPORT) || defined(SUPPORT_HF_INDICATORS)
            if(p_ibrt_ctrl->current_role == IBRT_MASTER)
            {
                uint8_t battery_level;
                TRACE("New TWS master update battery report after tws switch");
                app_battery_get_info(NULL, &battery_level, NULL);
                app_hfp_battery_report(battery_level);
            }
#endif
			TEST_STATUSMACHINE_TRACE("HCI_DBG_IBRT_SWITCH_COMPLETE_EVT_SUBCODE");
        }break;

        case HCI_NOTIFY_CURRENT_ADDR_EVT_CODE:{
			TEST_STATUSMACHINE_TRACE("HCI_NOTIFY_CURRENT_ADDR_EVT_CODE");
		}break;

        case HCI_DBG_TRACE_WARNING_EVT_CODE:{
			TEST_STATUSMACHINE_TRACE("HCI_DBG_TRACE_WARNING_EVT_CODE");
		}break;

        case HCI_SCO_SNIFFER_STATUS_EVT_CODE:{
			TEST_STATUSMACHINE_TRACE("HCI_SCO_SNIFFER_STATUS_EVT_CODE");
		}break;

        case HCI_DBG_RX_SEQ_ERROR_EVT_SUBCODE:{
			TEST_STATUSMACHINE_TRACE("HCI_DBG_RX_SEQ_ERROR_EVT_SUBCODE");
		}break;

        case HCI_LL_MONITOR_EVT_CODE:{
			TEST_STATUSMACHINE_TRACE("HCI_LL_MONITOR_EVT_CODE");
		}break;
        case HCI_GET_TWS_SLAVE_MOBILE_RSSI_CODE:{
			TEST_STATUSMACHINE_TRACE("HCI_GET_TWS_SLAVE_MOBILE_RSSI_CODE");
		}break;
        default:{
			TEST_STATUSMACHINE_TRACE("IBRT_UI_IDLE_UNKNOWN ++++++++++");
		}break;
    }
}

void app_ibrt_customif_ui_global_handler_ind(ibrt_link_type_e link_type, uint8_t evt_type, uint8_t status)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_ibrt_if_get_bt_ctrl_ctx();
    TRACE(" COSONIC:%s: %d", __func__, evt_type);
    switch (evt_type)
    {
        case BTIF_BTEVENT_LINK_CONNECT_CNF:{
			TRACE("BTIF_BTEVENT_LINK_CONNECT_CNF");
			
			TEST_STATUSMACHINE_TRACE("BTIF_BTEVENT_LINK_CONNECT_CNF");
			
		}break;
        case BTIF_BTEVENT_LINK_CONNECT_IND:{
			TRACE("BTIF_BTEVENT_LINK_CONNECT_IND");
			
			TEST_STATUSMACHINE_TRACE("BTIF_BTEVENT_LINK_CONNECT_IND");
		
            if(MOBILE_LINK == link_type){
                app_status_indication_set(APP_STATUS_INDICATION_CONNECTED);
            }
			if (TWS_LINK == link_type){
           		app_status_indication_set(APP_STATUS_INDICATION_CONNECTED);
            }
			
		}break;
        case BTIF_BTEVENT_LINK_DISCONNECT:{//断开连接
			TRACE("BTIF_BTEVENT_LINK_DISCONNECT");
			
			TEST_STATUSMACHINE_TRACE("BTIF_BTEVENT_LINK_DISCONNECT");
		
            if (!app_tws_ibrt_mobile_link_connected()){
                app_ibrt_if_sniff_checker_reset();
            }
            if (MOBILE_LINK == link_type){
                app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP, BT_STREAM_VOICE, BT_DEVICE_ID_1, MAX_RECORD_NUM);
                app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP, BT_STREAM_SBC, BT_DEVICE_ID_1, MAX_RECORD_NUM);
            }
		}break;
        case BTIF_STACK_LINK_DISCONNECT_COMPLETE:{//全部断开
        	TRACE("BTIF_STACK_LINK_DISCONNECT_COMPLETE");
			
			TEST_STATUSMACHINE_TRACE("BTIF_STACK_LINK_DISCONNECT_COMPLETE");
		
            app_status_indication_set(APP_STATUS_INDICATION_DISCONNECTED);
       	}break;

        case BTIF_BTEVENT_ROLE_CHANGE:{
        	TRACE("BTIF_BTEVENT_ROLE_CHANGE");
			
			TEST_STATUSMACHINE_TRACE("BTIF_BTEVENT_ROLE_CHANGE");
		
        }break;

        case BTIF_BTEVENT_BES_AUD_CONNECTED:{//  TWS 连接 与单耳 BT连接
        	TRACE("BTIF_BTEVENT_BES_AUD_CONNECTED");
			
			TEST_STATUSMACHINE_TRACE("BTIF_BTEVENT_BES_AUD_CONNECTED");
		
            if(!app_bt_get_num_of_connected_dev()){//如果未连接设备则进入配对模式
                TRACE("cosonic:enter pairing mode!");
            //     app_bt_accessmode_set(BTIF_BAM_GENERAL_ACCESSIBLE);//预留，如果sdk不自动进入配对则启用
                bt_disconnect_shadown();//5分钟未配对关机
            }

            //tws link callback when besaud connection complete
            if (BTIF_BEC_NO_ERROR == status)
            {
#ifdef ANC_APP
                tws_anc_sync_status();
#endif
                if (p_ibrt_ctrl->current_role == IBRT_MASTER &&
                    app_ibrt_ui_get_enter_pairing_mode())
                {
#if defined(MEDIA_PLAYER_SUPPORT)
                    app_voice_report(APP_STATUS_INDICATION_CONNECTED, 0);
#endif
                    app_status_indication_set(APP_STATUS_INDICATION_BOTHSCAN);
                }
                else
                {
                    app_status_indication_set(APP_STATUS_INDICATION_CONNECTED);
                }
            }

        }break;

        case BTIF_BTEVENT_BES_AUD_DISCONNECTED:{
        	TRACE("BTIF_BTEVENT_BES_AUD_DISCONNECTED");
			
			TEST_STATUSMACHINE_TRACE("BTIF_BTEVENT_BES_AUD_DISCONNECTED");
		
            app_tws_if_tws_disconnected_handler();
        }break;
        case BTIF_BTEVENT_ENCRYPTION_CHANGE:{
	        TRACE("BTIF_BTEVENT_ENCRYPTION_CHANGE");
			
			TEST_STATUSMACHINE_TRACE("BTIF_BTEVENT_ENCRYPTION_CHANGE");
		
	        if(!app_tws_ibrt_mobile_link_connected()){
	            TRACE("\nThe mobile link is false !!!!!!!!!!!!!!!!!!!!!\n");
	        }else{
	            TRACE("\nThe mobile link is true  ?????????????????????\n");
	        }
        }break;

        case BTIF_BTEVENT_MODE_CHANGE:{
			TRACE("BTIF_BTEVENT_MODE_CHANGE");
			
			TEST_STATUSMACHINE_TRACE("BTIF_BTEVENT_MODE_CHANGE");
		
		}break;
        default:{
			TRACE("BTIF_BTEVENT_MODE_UNKNOWN Event !!!!!!!!\n");
			TEST_STATUSMACHINE_TRACE("BTIF_BTEVENT_MODE_UNKNOWN Event !!!!!!!!");
		}break;
    }
}
void app_ibrt_customif_open_box_complete_ind(void)
{

}
void app_ibrt_customif_close_box_complete_ind(void)
{

}
void app_ibrt_customif_fetch_out_complete_ind(void)
{

}
void app_ibrt_customif_put_in_complete_ind(void)
{

}
void app_ibrt_customif_wear_up_complete_ind(void)
{

}
void app_ibrt_customif_wear_down_complete_ind(void)
{

}
void app_ibrt_customif_ui_global_event_update(ibrt_event_type evt_type, ibrt_ui_state_e old_state, 
        ibrt_ui_state_e new_state,ibrt_action_e   action,
        ibrt_ui_error_e status)
{
    TRACE(" COSONIC:%s: %d", __func__, __LINE__);
    switch (new_state){
        case IBRT_UI_IDLE:
            if (IBRT_UI_IDLE != old_state){
                //callback when UI event completed
                switch (evt_type){
                    case IBRT_OPEN_BOX_EVENT:
                        app_ibrt_customif_open_box_complete_ind();
						TEST_STATUSMACHINE_TRACE("IBRT_OPEN_BOX_EVENT");
                        break;
                    case IBRT_FETCH_OUT_EVENT:
                        app_ibrt_customif_fetch_out_complete_ind();
						TEST_STATUSMACHINE_TRACE("IBRT_FETCH_OUT_EVENT");
                        break;
                    case IBRT_PUT_IN_EVENT:
                        app_ibrt_customif_put_in_complete_ind();
						TEST_STATUSMACHINE_TRACE("IBRT_PUT_IN_EVENT");
                        break;
                    case IBRT_CLOSE_BOX_EVENT:
                        app_ibrt_customif_close_box_complete_ind();
						TEST_STATUSMACHINE_TRACE("IBRT_CLOSE_BOX_EVENT");
                        break;
                    case IBRT_WEAR_UP_EVENT:
                        app_ibrt_customif_wear_up_complete_ind();
						TEST_STATUSMACHINE_TRACE("IBRT_WEAR_UP_EVENT");
                        break;
                    case IBRT_WEAR_DOWN_EVENT:
                        app_ibrt_customif_wear_down_complete_ind();
						TEST_STATUSMACHINE_TRACE("IBRT_WEAR_DOWN_EVENT");
                        break;
                    default:
                        break;
                }
            }
			TEST_STATUSMACHINE_TRACE("IBRT_UI_IDLE");
            break;

        case IBRT_UI_IDLE_WAIT:{
			TRACE("IBRT_UI_IDLE_WAIT");
			
			TEST_STATUSMACHINE_TRACE("IBRT_UI_IDLE_WAIT");
		}break;
        

        case IBRT_UI_W4_TWS_CONNECTION:{
        	TRACE("IBRT_UI_W4_TWS_CONNECTION");
			
			TEST_STATUSMACHINE_TRACE("IBRT_UI_W4_TWS_CONNECTION");
       	}break;

        case IBRT_UI_W4_TWS_INFO_EXCHANGE_COMPLETE:{
        	TRACE("IBRT_UI_W4_TWS_INFO_EXCHANGE_COMPLETE");

			TEST_STATUSMACHINE_TRACE("IBRT_UI_W4_TWS_INFO_EXCHANGE_COMPLETE");
        }break;

        case IBRT_UI_W4_TWS_BT_MSS_COMPLETE:{
        	TRACE("IBRT_UI_W4_TWS_BT_MSS_COMPLETE");
			
			TEST_STATUSMACHINE_TRACE("IBRT_UI_W4_TWS_BT_MSS_COMPLETE");
        }break;

        case IBRT_UI_W4_SET_ENV_COMPLETE:{
        	TRACE("IBRT_UI_W4_SET_ENV_COMPLETE");
			
			TEST_STATUSMACHINE_TRACE("IBRT_UI_W4_SET_ENV_COMPLETE");
        }break;

        case IBRT_UI_W4_MOBILE_CONNECTION:{
        	TRACE("IBRT_UI_W4_MOBILE_CONNECTION");

			TEST_STATUSMACHINE_TRACE("IBRT_UI_W4_MOBILE_CONNECTION");
        }break;

        case IBRT_UI_W4_MOBILE_MSS_COMPLETE:{
        	TRACE("IBRT_UI_W4_MOBILE_MSS_COMPLETE");

			TEST_STATUSMACHINE_TRACE("IBRT_UI_W4_MOBILE_MSS_COMPLETE");
        }break;

        case IBRT_UI_W4_MOBILE_ENTER_ACTIVE_MODE:{
        	TRACE("IBRT_UI_W4_MOBILE_ENTER_ACTIVE_MODE");

			TEST_STATUSMACHINE_TRACE("IBRT_UI_W4_MOBILE_ENTER_ACTIVE_MODE");
        }break;

        case IBRT_UI_W4_START_IBRT_COMPLETE:{
        	TRACE("IBRT_UI_W4_START_IBRT_COMPLETE");

			TEST_STATUSMACHINE_TRACE("IBRT_UI_W4_START_IBRT_COMPLETE");
        }break;

        case IBRT_UI_W4_IBRT_DATA_EXCHANGE_COMPLETE:{
        	TRACE("IBRT_UI_W4_IBRT_DATA_EXCHANGE_COMPLETE");

			TEST_STATUSMACHINE_TRACE("IBRT_UI_W4_IBRT_DATA_EXCHANGE_COMPLETE");
        }break;

        case IBRT_UI_W4_TWS_SWITCH_COMPLETE:{
        	TRACE("IBRT_UI_W4_TWS_SWITCH_COMPLETE");
			
			TEST_STATUSMACHINE_TRACE("IBRT_UI_W4_TWS_SWITCH_COMPLETE");
        }break;

        case IBRT_UI_W4_SM_STOP:{
        	TRACE("IBRT_UI_W4_SM_STOP");
			TEST_STATUSMACHINE_TRACE("IBRT_UI_W4_SM_STOP");
        }break;

        default:{
			TEST_STATUSMACHINE_TRACE("IBRT_UI_W4_SM_UNKONWN");
		}break;
    }
}
/*
* custom tws switch interface
* tws switch cmd send sucess, return true, else return false
*/
bool app_ibrt_customif_ui_tws_switch(void)
{
    return app_ibrt_ui_tws_switch();
}
/*
* custom reconfig bd_addr
*/
/**
 * @description: 绑定蓝牙地址
 * @param {bt_bdaddr_t} local_addr
 * @param {bt_bdaddr_t} peer_addr
 * @param {ibrt_role_e} nv_role
 * @return {*}
 * @author: Jevin
 */
void app_ibrt_customif_ui_reconfig_bd_addr(bt_bdaddr_t local_addr, bt_bdaddr_t peer_addr, ibrt_role_e nv_role)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    app_ibrt_ui_t *p_ibrt_ui = app_ibrt_ui_get_ctx();

    p_ibrt_ctrl->local_addr = local_addr;
    p_ibrt_ctrl->peer_addr  = peer_addr;
    p_ibrt_ctrl->nv_role    = nv_role;

    if (!p_ibrt_ctrl->is_ibrt_search_ui)
    {
        if (IBRT_MASTER == p_ibrt_ctrl->nv_role)
        {
            p_ibrt_ctrl->peer_addr = local_addr;
            btif_me_set_bt_address(p_ibrt_ctrl->local_addr.address);
        }
        else if (IBRT_SLAVE == p_ibrt_ctrl->nv_role)
        {
            p_ibrt_ctrl->local_addr = peer_addr;
            btif_me_set_bt_address(p_ibrt_ctrl->local_addr.address);
        }
        else
        {
            ASSERT(0, "%s nv_role error", __func__);
        }
    }
    p_ibrt_ui->bonding_success = true;
}
/*custom can block connect mobile if needed*/
bool app_ibrt_customif_connect_mobile_needed_ind(void)
{
    //ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    TRACE(" COSONIC:%s: %d", __func__, __LINE__);
    app_ibrt_ui_t *p_ibrt_ui = app_ibrt_ui_get_ctx();
    if(p_ibrt_ui->peer_tws_info.pairing_mode)
     {
        return false;
    }
    return true;
}
/**
 * @description: BT配对连接时回调
 * @param {bt_bdaddr_t} *
 * @return {*}
 * @author: Jevin
 */
void app_ibrt_customif_mobile_connected_ind(bt_bdaddr_t * addr)
{
    TRACE(" COSONIC:%s: %d", __func__, __LINE__);
    app_ibrt_if_config_keeper_mobile_update(addr);
}

void app_ibrt_customif_ibrt_connected_ind(bt_bdaddr_t * addr)
{
    TRACE(" COSONIC:%s: %d", __func__, __LINE__);
    app_ibrt_if_config_keeper_mobile_update(addr);
}

void app_ibrt_customif_tws_connected_ind(bt_bdaddr_t * addr)//TWS 连接成功回调函数
{
    TRACE(" COSONIC:%s: %d", __func__, __LINE__);
    app_ibrt_if_config_keeper_tws_update(addr);
}

void app_ibrt_customif_profile_state_change_ind(uint32_t profile,uint8_t connected)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    TRACE("custom if profle=%x state change to =%x",profile,connected);
    TRACE("COSONIC:%s: %d", __func__, __LINE__);
    switch (profile)
    {
        case BTIF_APP_A2DP_PROFILE_ID:
            if (connected)
            {
                TRACE("cutomif A2DP profile connected");
                if (p_ibrt_ctrl->current_role == IBRT_SLAVE){              
                    //TO DO
                }else if (p_ibrt_ctrl->current_role == IBRT_MASTER){
                    //TO DO
                }else{
                    //TO DO
                }
            }
            else
            {
                TRACE("cutomif A2DP profile disconnected");
                if (p_ibrt_ctrl->current_role == IBRT_SLAVE){              
                    //TO DO
                }else if (p_ibrt_ctrl->current_role == IBRT_MASTER){
                    //TO DO
                }else{
                    //TO DO
                }
            }
            break;

        case BTIF_APP_AVRCP_PROFILE_ID:
            if (connected)
            {
                TRACE("cutomif AVRCP profile connected");
                if (p_ibrt_ctrl->current_role == IBRT_SLAVE){              
                    //TO DO
                }else if (p_ibrt_ctrl->current_role == IBRT_MASTER){
                    //TO DO
                }else{
                    //TO DO
                }
            }
            else
            {
                TRACE("cutomif AVRCP profile disconnected");
                if (p_ibrt_ctrl->current_role == IBRT_SLAVE){              
                    //TO DO
                }else if (p_ibrt_ctrl->current_role == IBRT_MASTER){
                    //TO DO
                }else{
                    //TO DO
                }
            }
            break;

        case BTIF_APP_HFP_PROFILE_ID:
            if (connected)
            {
                TRACE("cutomif HFP profile connected");
                if (p_ibrt_ctrl->current_role == IBRT_SLAVE){              
                    //TO DO
                }else if (p_ibrt_ctrl->current_role == IBRT_MASTER){
                    //TO DO
                }else{
                    //TO DO
                }
            }
            else
            {
                TRACE("cutomif HFP profile disconnected");
                if (p_ibrt_ctrl->current_role == IBRT_SLAVE){              
                    //TO DO
                }else if (p_ibrt_ctrl->current_role == IBRT_MASTER){
                    //TO DO
                }else{
                    //TO DO
                }
            }
            break;

        case BTIF_APP_SPP_CLIENT_AI_VOICE_ID:
        case BTIF_APP_SPP_SERVER_AI_VOICE_ID:
        case BTIF_APP_SPP_SERVER_GREEN_ID:
        case BTIF_APP_SPP_CLIENT_CCMP_ID:
        case BTIF_APP_SPP_CLIENT_RED_ID:
        case BTIF_APP_SPP_SERVER_RED_ID:
        case BTIF_APP_SPP_SERVER_TOTAD_ID:
        case BTIF_APP_SPP_SERVER_GSOUND_CTL_ID:
        case BTIF_APP_SPP_SERVER_GSOUND_AUD_ID:
        case BTIF_APP_SPP_SERVER_BES_OTA_ID:
        case BTIF_APP_SPP_SERVER_FP_RFCOMM_ID:
            if (connected)
            {
                //TRACE("cutomif SPP profile connected");
               	if (p_ibrt_ctrl->current_role == IBRT_SLAVE){              
                    //TO DO
               	}else if (p_ibrt_ctrl->current_role == IBRT_MASTER){
                    //TO DO
                }else{
                    //TO DO
                }

            }
            else
            {
                TRACE("cutomif SPP profile disconnected");
				if (p_ibrt_ctrl->current_role == IBRT_SLAVE){			   
					//TO DO
				}else if (p_ibrt_ctrl->current_role == IBRT_MASTER){
					//TO DO
				}else{
					//TO DO
				}
            }
            break;

        default:
            TRACE("unknown profle=%x state change",profile);
            break;
    }

}

/*
* custom config main function
*/

void app_ibrt_customif_ui_pairing_set(void)
{
    TRACE(" COSONIC:%s: %d", __func__, __LINE__);
}

void app_ibrt_customif_ui_pairing_clear(void)
{
    TRACE(" COSONIC:%s: %d", __func__, __LINE__);
}



int app_ibrt_customif_ui_start(void)
{
    ibrt_ui_config_t config;

    // zero init the config
    memset(&config, 0, sizeof(ibrt_ui_config_t));

    //freeman mode config, default should be false
    config.freeman_enable                           = false;

    //tws earphone set the same addr, UI will be flexible, default should be true
    config.tws_use_same_addr                        = true;

    //ibrt slave will reconnect to mobile if tws connect failed, default should be true
    config.slave_reconnect_enable                   = true;

    //do tws switch when wearup or weardown, must be true because MIC will be with IBRT master
    config.wear_updown_tws_switch_enable            = true;

    //pairing mode default value, default should be set false
    config.enter_pairing_mode                       = false;

    /*
     following cases the reconnect will be fail for freeman, 
     please set to true if you want to reconnect successful:
     1. freeman has link key but mobile deleted the link key
     2. freeman changed its bt address after reboot and use the new address to reconnect mobile
    */
    config.freeman_accept_mobile_new_pairing        = false;

    //for some proj no box key, default should be false;
    config.enter_pairing_on_empty_mobile_addr       = false;//蓝牙地址为空时进配对

    //for some proj no box key, default should be false
    config.enter_pairing_on_reconnect_mobile_failed = false;//回连手机失败后进配对

    //when mobile has connected, enter_pairing_on_reconnect_mobile_failed will be cleared, default false
    config.enter_pairing_on_reconnect_mobile_failed_once = false;

    //for some proj no box key, default should be false
    config.enter_pairing_on_mobile_disconnect       = true;//手主动断开进配对
    
    //disconnect mobile from timeout enter pairing mode
    config.enter_pairing_on_mobile_timeout_disconnect = false;//蓝牙超时断连进配对

    //reconnect timeout after disconnect mobile from timeout enter pairing mode
    config.enter_pairing_on_mobile_timeout_disconnect_reconnect_timeout = true;

    //for 08 error reconnect event, default must be true
    config.disc_tws_before_reconnect_mobile         = true;

    //do tws switch when RSII value change, default should be true
    config.tws_switch_according_to_rssi_value       = false;//根据信号强度主从切

    //disable tws switch, NOT recommended to open
    config.disable_tws_switch                       = false;

    //disable tws switch, NOT recommended to open
    config.disable_stop_ibrt                        = true;

    //exchange snoop info by BLE_box, special custom config, default should be false
    config.snoop_via_ble_enable                     = false;
    //controller basband monitor
    config.lowlayer_monitor_enable                  = false;

    config.delay_exit_sniff                         = true;
    config.delay_ms_exit_sniff                      = 3000;

    config.check_plugin_excute_closedbox_event      = true;//入盒处理关盖事件
    config.nv_slave_enter_pairing_on_mobile_disconnect      = false;
    config.nv_slave_enter_pairing_on_empty_mobile_addr      = false;

    //only allow paired mobile device incoming when not in paring mode,default should be false
    config.mobile_incoming_filter_unpaired          = false;
    //close box debounce time config
    config.close_box_event_wait_response_timeout          = IBRT_UI_CLOSE_BOX_EVENT_WAIT_RESPONSE_TIMEOUT;
    //do tws switch when rssi value change over threshold
    config.rssi_threshold                                 = IBRT_UI_ROLE_SWITCH_THRESHOLD_WITH_RSSI;

    //wait time before launch reconnect event
    config.reconnect_mobile_wait_response_timeout         = IBRT_UI_RECONNECT_MOBILE_WAIT_RESPONSE_TIMEOUT;

    //reconnect event internal config wait timer when tws disconnect
    config.reconnect_wait_ready_timeout                   = IBRT_UI_MOBILE_RECONNECT_WAIT_READY_TIMEOUT;
    config.reconnect_mobile_wait_ready_timeout            = IBRT_UI_MOBILE_RECONNECT_WAIT_READY_TIMEOUT;
    config.reconnect_tws_wait_ready_timeout               = IBRT_UI_TWS_RECONNECT_WAIT_READY_TIMEOUT;
    config.reconnect_ibrt_wait_response_timeout           = IBRT_UI_RECONNECT_IBRT_WAIT_RESPONSE_TIMEOUT;
    config.nv_master_reconnect_tws_wait_response_timeout  = IBRT_UI_NV_MASTER_RECONNECT_TWS_WAIT_RESPONSE_TIMEOUT;
    config.nv_slave_reconnect_tws_wait_response_timeout   = IBRT_UI_NV_SLAVE_RECONNECT_TWS_WAIT_RESPONSE_TIMEOUT;

    //pairing mode timeout config
    config.disable_bt_scan_timeout                        = IBRT_UI_DISABLE_BT_SCAN_TIMEOUT;

    //open box reconnect mobile times config
    config.open_reconnect_mobile_max_times                = IBRT_UI_OPEN_RECONNECT_MOBILE_MAX_TIMES;

    //open box reconnect tws times config
    config.open_reconnect_tws_max_times                   = IBRT_UI_OPEN_RECONNECT_TWS_MAX_TIMES;

    //connection timeout reconnect mobile times config
    config.reconnect_mobile_max_times                     = IBRT_UI_RECONNECT_MOBILE_MAX_TIMES;

    //connection timeout reconnect tws times config
    config.reconnect_tws_max_times                        = IBRT_UI_RECONNECT_TWS_MAX_TIMES;

    //connection timeout reconnect ibrt times config
    config.reconnect_ibrt_max_times                       = IBRT_UI_RECONNECT_IBRT_MAX_TIMES;

    //reconnect tws one cycle
    config.tws_reconnect_cycle                            = IBRT_TWS_RECONNECT_ONE_CYCLE;

    //reconnect mobile one cycle
    config.mobile_reconnect_cycle                         = IBRT_MOBILE_RECONNECT_ONE_CYCLE;

    //BES internal config, DO NOT modify
    config.long_private_poll_interval                     = IBRT_UI_LONG_POLL_INTERVAL;
    config.default_private_poll_interval                  = IBRT_UI_DEFAULT_POLL_INTERVAL;
    config.short_private_poll_interval                    = IBRT_UI_SHORT_POLL_INTERVAL;
    config.default_private_poll_interval_in_sco           = IBRT_UI_DEFAULT_POLL_INTERVAL_IN_SCO;
    config.short_private_poll_interval_in_sco             = IBRT_UI_SHORT_POLL_INTERVAL_IN_SCO;
    config.default_bt_tpoll                               = IBRT_TWS_BT_TPOLL_DEFAULT;

    //for fast connect when only one headset in the nearby
    config.tws_page_timeout_on_last_success               = IBRT_TWS_PAGE_TIMEOUT_ON_CONNECT_SUCCESS_LAST;
    config.tws_page_timeout_on_last_failed                = IBRT_TWS_PAGE_TIMEOUT_ON_CONNECT_FAILED_LAST;
    config.mobile_page_timeout                            = IBRT_MOBILE_PAGE_TIMEOUT;
    config.tws_page_timeout_on_reconnect_mobile_failed    = IBRT_TWS_PAGE_TIMEOUT_ON_RECONNECT_MOBILE_FAILED;
    config.tws_page_timeout_on_reconnect_mobile_success   = IBRT_TWS_PAGE_TIMEOUT_ON_RECONNECT_MOBILE_SUCCESS;

    //tws connection timeout
    config.tws_connection_timeout                         = IBRT_UI_TWS_CONNECTION_TIMEOUT;

    config.rx_seq_error_timeout                           = IBRT_UI_RX_SEQ_ERROR_TIMEOUT;
    config.rx_seq_error_threshold                         = IBRT_UI_RX_SEQ_ERROR_THRESHOLD;
    config.rx_seq_recover_wait_timeout                    = IBRT_UI_RX_SEQ_ERROR_RECOVER_TIMEOUT;

    config.rssi_monitor_timeout                           = IBRT_UI_RSSI_MONITOR_TIMEOUT;


    config.wear_updown_detect_supported                   = false;
    config.stop_ibrt_timeout                              = IBRT_UI_STOP_IBRT_TIMEOUT;


    config.radical_scan_interval_nv_slave                 = IBRT_UI_RADICAL_SAN_INTERVAL_NV_SLAVE;
    config.radical_scan_interval_nv_master                = IBRT_UI_RADICAL_SAN_INTERVAL_NV_MASTER;
    config.event_hung_timeout                             = IBRT_EVENT_HUNG_TIMEOUT;
    config.rssi_tws_switch_threshold                      = IBRT_TWS_SWITCH_RSSI_THRESHOLD;
    config.stop_ibrt_wait_time_after_tws_switch           = IBRT_STOP_IBRT_WAIT_TIME;
    config.tws_conn_failed_wait_time                      = TWS_CONN_FAILED_WAIT_TIME;

    config.sm_running_timeout                             = SM_RUNNING_TIMEOUT;
    config.peer_sm_running_timeout                        = PEER_SM_RUNNING_TIMEOUT;
    config.reconnect_peer_sm_running_timeout              = RECONNECT_PEER_SM_RUNNING_TIMEOUT;
    config.connect_no_03_timeout                          = CONNECT_NO_03_TIMEOUT;
    config.disconnect_no_05_timeout                       = DISCONNECT_NO_05_TIMEOUT;

    config.tws_switch_tx_data_protect                     = true;
    config.tws_cmd_send_timeout                           = IBRT_UI_TWS_CMD_SEND_TIMEOUT;
    config.tws_cmd_send_counter_threshold                 = IBRT_UI_TWS_COUNTER_THRESHOLD;
    config.tws_switch_stable_timeout                      = IBRT_UI_TWS_SWITCH_STABLE_TIMEOUT;

    config.invoke_event_when_box_closed                   = true;
    config.mss_retry_max_times                            = IBRT_UI_MSS_MAX_TIMES;

    //if open_box/close box detect supported, may open this config to speed up connection setup
    config.tws_stay_when_close_box                        = false;
    config.free_tws_timeout                               = IBRT_UI_DISC_TWS_TIMEOUT;

    config.profile_concurrency_supported                  = false;

    config.audio_sync_mismatch_resume_version             = 2;
    //config.delay_reconnect_mobile                         = 0;//do not modify, just for special request
    config.bqb_test_enable                                = false;//attention,this is just for bqb test, do not modify
    config.filter_duplicate_event                         = true;
    app_ibrt_if_register_global_handler_ind(app_ibrt_customif_ui_global_handler_ind);
    app_ibrt_if_register_vender_handler_ind(app_ibrt_customif_ui_vender_event_handler_ind);
    app_ibrt_if_register_global_event_update_ind(app_ibrt_customif_ui_global_event_update);
    app_ibrt_if_register_link_connected_ind(app_ibrt_customif_mobile_connected_ind,
                                            app_ibrt_customif_ibrt_connected_ind,
                                            app_ibrt_customif_tws_connected_ind);
    app_ibrt_if_register_profile_state_change_ind(app_ibrt_customif_profile_state_change_ind);
    app_ibrt_if_register_connect_mobile_needed_ind(app_ibrt_customif_connect_mobile_needed_ind);
    app_ibrt_if_register_pairing_mode_ind(app_ibrt_customif_ui_pairing_set,app_ibrt_customif_ui_pairing_clear);
    app_ibrt_if_ui_reconfig(&config);
    if (config.delay_exit_sniff)
    {
        app_ibrt_if_sniff_checker_init(config.delay_ms_exit_sniff);
    }

    return 0;
}

#endif
