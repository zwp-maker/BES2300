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
#include "tgt_hardware.h"
#include "pmu.h"
#include "hal_timer.h"
#include "hal_gpadc.h"
#include "hal_trace.h"
#include "hal_gpio.h"
#include "hal_iomux.h"
#include "hal_chipid.h"
#include "app_thread.h"
#include "app_battery.h"
#include "apps.h"
#include "app_status_ind.h"
#include "communication_svr.h"
#include "../../services/app_ibrt/inc/app_ibrt_peripheral_manager.h"
#ifdef BT_USB_AUDIO_DUAL_MODE
#include "btusb_audio.h"
#endif
#include <stdlib.h>

#ifdef __INTERCONNECTION__
#include "app_ble_mode_switch.h"
#endif

#if (defined(BTUSB_AUDIO_MODE) || defined(BTUSB_AUDIO_MODE))
extern "C" bool app_usbaudio_mode_on(void);
#endif

#define APP_BATTERY_TRACE(s,...)
// TRACE(s, ##__VA_ARGS__)

#ifndef APP_BATTERY_MIN_MV
#define APP_BATTERY_MIN_MV (3200)
#endif

#ifndef APP_BATTERY_MAX_MV
#define APP_BATTERY_MAX_MV (4200)
#endif

#ifndef APP_BATTERY_PD_MV
#define APP_BATTERY_PD_MV   (3100)
#endif

#ifndef APP_BATTERY_CHARGE_TIMEOUT_MIN
#define APP_BATTERY_CHARGE_TIMEOUT_MIN (90)
#endif

#ifndef APP_BATTERY_CHARGE_OFFSET_MV
#define APP_BATTERY_CHARGE_OFFSET_MV (20)
#endif

#ifndef CHARGER_PLUGINOUT_RESET
#define CHARGER_PLUGINOUT_RESET (1)
#endif

#ifndef CHARGER_PLUGINOUT_DEBOUNCE_MS
#define CHARGER_PLUGINOUT_DEBOUNCE_MS (50)
#endif

#ifndef CHARGER_PLUGINOUT_DEBOUNCE_CNT
#define CHARGER_PLUGINOUT_DEBOUNCE_CNT (3)
#endif

#define APP_BATTERY_CHARGING_PLUGOUT_DEDOUNCE_CNT (APP_BATTERY_CHARGING_PERIODIC_MS<500?3:1)

#define APP_BATTERY_CHARGING_EXTPIN_MEASURE_CNT (APP_BATTERY_CHARGING_PERIODIC_MS<2*1000?2*1000/APP_BATTERY_CHARGING_PERIODIC_MS:1)
#define APP_BATTERY_CHARGING_EXTPIN_DEDOUNCE_CNT (6)

#define APP_BATTERY_CHARGING_OVERVOLT_MEASURE_CNT (APP_BATTERY_CHARGING_PERIODIC_MS<2*1000?2*1000/APP_BATTERY_CHARGING_PERIODIC_MS:1)
#define APP_BATTERY_CHARGING_OVERVOLT_DEDOUNCE_CNT (3)

#define APP_BATTERY_CHARGING_SLOPE_MEASURE_CNT (APP_BATTERY_CHARGING_PERIODIC_MS<20*1000?20*1000/APP_BATTERY_CHARGING_PERIODIC_MS:1)
#define APP_BATTERY_CHARGING_SLOPE_TABLE_COUNT (6)


#define APP_BATTERY_REPORT_INTERVAL (5)

#define APP_BATTERY_MV_BASE ((APP_BATTERY_MAX_MV-APP_BATTERY_PD_MV)/(APP_BATTERY_LEVEL_NUM))

#define APP_BATTERY_STABLE_COUNT (5)
#define APP_BATTERY_MEASURE_PERIODIC_FAST_MS (200)
#define APP_BATTERY_MEASURE_PERIODIC_NORMAL_MS (10000)
#define APP_BATTERY_CHARGING_PERIODIC_MS (APP_BATTERY_MEASURE_PERIODIC_NORMAL_MS)

#define APP_BATTERY_SET_MESSAGE(appevt, status, volt) (appevt = (((uint32_t)status&0xffff)<<16)|(volt&0xffff))
#define APP_BATTERY_GET_STATUS(appevt, status) (status = (appevt>>16)&0xffff)
#define APP_BATTERY_GET_VOLT(appevt, volt) (volt = appevt&0xffff)
#define APP_BATTERY_GET_PRAMS(appevt, prams) ((prams) = appevt&0xffff)

#define APP_GPIO_SET_MESSAGE(appevt, status) (appevt = ((uint32_t)status&0xffff)<<16)
#define APP_GPIO_GET_STATUS(appevt, status) (status = (appevt>>16)&0xffff)

enum APP_BATTERY_MEASURE_PERIODIC_T
{
    APP_BATTERY_MEASURE_PERIODIC_FAST = 0,
    APP_BATTERY_MEASURE_PERIODIC_NORMAL,
    APP_BATTERY_MEASURE_PERIODIC_CHARGING,

    APP_BATTERY_MEASURE_PERIODIC_QTY,
};

struct APP_BATTERY_MEASURE_CHARGER_STATUS_T
{
    HAL_GPADC_MV_T prevolt;
    int32_t slope_1000[APP_BATTERY_CHARGING_SLOPE_TABLE_COUNT];
    int slope_1000_index;
    int cnt;
};


typedef void (*APP_BATTERY_EVENT_CB_T)(enum APP_BATTERY_STATUS_T, APP_BATTERY_MV_T volt);

struct APP_BATTERY_MEASURE_T
{
    uint32_t start_time;
    enum APP_BATTERY_STATUS_T status;
#ifdef __INTERCONNECTION__
    uint8_t currentBatteryInfo;
    uint8_t lastBatteryInfo;
    uint8_t isMobileSupportSelfDefinedCommand;
#else
    uint8_t currlevel;
#endif
    APP_BATTERY_MV_T currvolt;
    APP_BATTERY_MV_T lowvolt;
    APP_BATTERY_MV_T highvolt;
    APP_BATTERY_MV_T pdvolt;
    uint32_t chargetimeout;
    enum APP_BATTERY_MEASURE_PERIODIC_T periodic;
    HAL_GPADC_MV_T voltage[APP_BATTERY_STABLE_COUNT];
    uint16_t index;
    struct APP_BATTERY_MEASURE_CHARGER_STATUS_T charger_status;
    APP_BATTERY_EVENT_CB_T cb;
    APP_BATTERY_CB_T user_cb;
};


static void app_battery_pluginout_debounce_start(void);
static void app_battery_pluginout_debounce_handler(void const *param);
osTimerDef (APP_BATTERY_PLUGINOUT_DEBOUNCE, app_battery_pluginout_debounce_handler);
static osTimerId app_battery_pluginout_debounce_timer = NULL;
static uint32_t app_battery_pluginout_debounce_ctx = 0;
static uint32_t app_battery_pluginout_debounce_cnt = 0;

static void app_battery_timer_handler(void const *param);
osTimerDef (APP_BATTERY, app_battery_timer_handler);
static osTimerId app_battery_timer = NULL;
static struct APP_BATTERY_MEASURE_T app_battery_measure;

static uint8_t overvolt_full_charge_cnt = 0;
static uint8_t ext_pin_full_charge_cnt = 0;

static int app_battery_charger_handle_process(void);
static int app_battery_charger_handle_reset(void);

#ifdef __INTERCONNECTION__
uint8_t* app_battery_get_mobile_support_self_defined_command_p(void)
{
    return &app_battery_measure.isMobileSupportSelfDefinedCommand;
}
#endif

void app_battery_irqhandler(uint16_t irq_val, HAL_GPADC_MV_T volt)
{
    uint8_t i;
    uint32_t meanBattVolt = 0;
    HAL_GPADC_MV_T vbat = volt;
    APP_BATTERY_TRACE("%s %d",__func__, vbat);
    if (vbat == HAL_GPADC_BAD_VALUE)
    {
        app_battery_measure.cb(APP_BATTERY_STATUS_INVALID, vbat);
        return;
    }

#if (defined(BTUSB_AUDIO_MODE) || defined(BTUSB_AUDIO_MODE))
    if(app_usbaudio_mode_on()) return ;
#endif
    app_battery_measure.voltage[app_battery_measure.index++%APP_BATTERY_STABLE_COUNT] = vbat<<2;

    if (app_battery_measure.index > APP_BATTERY_STABLE_COUNT)
    {
        for (i=0; i<APP_BATTERY_STABLE_COUNT; i++)
        {
            meanBattVolt += app_battery_measure.voltage[i];
        }
        meanBattVolt /= APP_BATTERY_STABLE_COUNT;
        if (app_battery_measure.cb)
        {
            if (meanBattVolt>app_battery_measure.highvolt)
            {
                app_battery_measure.cb(APP_BATTERY_STATUS_OVERVOLT, meanBattVolt);
            }
            else if((meanBattVolt>app_battery_measure.pdvolt) && (meanBattVolt<app_battery_measure.lowvolt))
            {
                app_battery_measure.cb(APP_BATTERY_STATUS_UNDERVOLT, meanBattVolt);
            }
            else if(meanBattVolt<app_battery_measure.pdvolt)
            {
                app_battery_measure.cb(APP_BATTERY_STATUS_PDVOLT, meanBattVolt);
            }
            else
            {
                app_battery_measure.cb(APP_BATTERY_STATUS_NORMAL, meanBattVolt);
            }
        }
    }
    else
    {
        int8_t level = 0;
        meanBattVolt = vbat<<2;
        level = (meanBattVolt-APP_BATTERY_PD_MV)/APP_BATTERY_MV_BASE;

        if (level<APP_BATTERY_LEVEL_MIN)
            level = APP_BATTERY_LEVEL_MIN;
        if (level>APP_BATTERY_LEVEL_MAX)
            level = APP_BATTERY_LEVEL_MAX;

        app_battery_measure.currvolt = meanBattVolt;
#ifdef __INTERCONNECTION__
        APP_BATTERY_INFO_T* pBatteryInfo = (APP_BATTERY_INFO_T*)&app_battery_measure.currentBatteryInfo;
        pBatteryInfo->batteryLevel = level;
#else
        app_battery_measure.currlevel = level;
#endif
    }
}

static void app_battery_timer_start(enum APP_BATTERY_MEASURE_PERIODIC_T periodic)
{
    uint32_t periodic_millisec = 0;

    if (app_battery_measure.periodic != periodic){
        app_battery_measure.periodic = periodic;
        switch (periodic)
        {
            case APP_BATTERY_MEASURE_PERIODIC_FAST:
                periodic_millisec = APP_BATTERY_MEASURE_PERIODIC_FAST_MS;
                break;
            case APP_BATTERY_MEASURE_PERIODIC_CHARGING:
                periodic_millisec = APP_BATTERY_CHARGING_PERIODIC_MS;
                break;
            case APP_BATTERY_MEASURE_PERIODIC_NORMAL:
                periodic_millisec = APP_BATTERY_MEASURE_PERIODIC_NORMAL_MS;
            default:
                break;
        }

        osTimerStop(app_battery_timer);
        osTimerStart(app_battery_timer, periodic_millisec);
    }
}

static void app_battery_timer_handler(void const *param)
{
    hal_gpadc_open(HAL_GPADC_CHAN_BATTERY, HAL_GPADC_ATP_ONESHOT, app_battery_irqhandler);
    ntc_capture_open();
}

static void app_battery_event_process(enum APP_BATTERY_STATUS_T status, APP_BATTERY_MV_T volt)
{
    uint32_t app_battevt;
    APP_MESSAGE_BLOCK msg;

    APP_BATTERY_TRACE("%s %d,%d",__func__, status, volt);
    msg.mod_id = APP_MODUAL_BATTERY;
    APP_BATTERY_SET_MESSAGE(app_battevt, status, volt);
    msg.msg_body.message_id = app_battevt;
    msg.msg_body.message_ptr = (uint32_t)NULL;
    app_mailbox_put(&msg);
}

int app_battery_handle_process_normal(uint32_t status,  union APP_BATTERY_MSG_PRAMS prams)
{
    int8_t level = 0;
	//cosonic_trace(battery_manager,true,"");

    switch (status)
    {
        case APP_BATTERY_STATUS_UNDERVOLT:
            TRACE("UNDERVOLT:%d", prams.volt);
            app_status_indication_set(APP_STATUS_INDICATION_CHARGENEED);
#ifdef MEDIA_PLAYER_SUPPORT
#if defined(IBRT)

#else
            app_voice_report(APP_STATUS_INDICATION_CHARGENEED, 0);
#endif
#endif
        case APP_BATTERY_STATUS_NORMAL:
        case APP_BATTERY_STATUS_OVERVOLT:
            app_battery_measure.currvolt = prams.volt;
            level = (prams.volt-APP_BATTERY_PD_MV)/APP_BATTERY_MV_BASE;

            if (level<APP_BATTERY_LEVEL_MIN)
                level = APP_BATTERY_LEVEL_MIN;
            if (level>APP_BATTERY_LEVEL_MAX)
                level = APP_BATTERY_LEVEL_MAX;
#ifdef __INTERCONNECTION__            
            APP_BATTERY_INFO_T* pBatteryInfo;
            pBatteryInfo = (APP_BATTERY_INFO_T*)&app_battery_measure.currentBatteryInfo;
            pBatteryInfo->batteryLevel = level;
            if(level == APP_BATTERY_LEVEL_MAX)
            {
                level = 9;
            }
            else
            {
                level /= 10;
            }
#else
            app_battery_measure.currlevel = level;
#endif
            app_status_battery_report(level);
            break;
        case APP_BATTERY_STATUS_PDVOLT:
#ifndef BT_USB_AUDIO_DUAL_MODE
            TRACE("PDVOLT-->POWEROFF:%d", prams.volt);
            osTimerStop(app_battery_timer);
            app_shutdown();
#endif
            break;
        case APP_BATTERY_STATUS_CHARGING:
            TRACE("CHARGING-->APP_BATTERY_CHARGER :%d", prams.charger);
            if (prams.charger == APP_BATTERY_CHARGER_PLUGIN)
            {
#ifdef BT_USB_AUDIO_DUAL_MODE
                TRACE("_debug: %s:%d",__func__,__LINE__);
                btusb_switch(BTUSB_MODE_USB);
#else
#if CHARGER_PLUGINOUT_RESET
                app_reset();
#else
                app_battery_measure.status = APP_BATTERY_STATUS_CHARGING;
#endif
#endif
            }
            else if (prams.charger == APP_BATTERY_CHARGER_PLUGOUT)
            {
#ifdef BT_USB_AUDIO_DUAL_MODE
                TRACE("_debug: %s:%d",__func__,__LINE__);
                btusb_switch(BTUSB_MODE_BT);
#endif
            }
            break;
        case APP_BATTERY_STATUS_INVALID:
        default:
            break;
    }

    app_battery_timer_start(APP_BATTERY_MEASURE_PERIODIC_NORMAL);
    return 0;
}

int app_battery_handle_process_charging(uint32_t status,  union APP_BATTERY_MSG_PRAMS prams)
{
	// cosonic_trace(battery_manager,true,"");

    switch (status)
    {
        case APP_BATTERY_STATUS_OVERVOLT:
        case APP_BATTERY_STATUS_NORMAL:
        case APP_BATTERY_STATUS_UNDERVOLT:
            app_battery_measure.currvolt = prams.volt;
            app_status_battery_report(prams.volt);
            break;
        case APP_BATTERY_STATUS_CHARGING:
            TRACE("CHARGING:%d/%d", prams.charger);
            if (prams.charger == APP_BATTERY_CHARGER_PLUGOUT)
            {
#ifdef BT_USB_AUDIO_DUAL_MODE
                TRACE("%s:%d.",__func__,__LINE__);
#else
#if CHARGER_PLUGINOUT_RESET    
                TRACE("CHARGING-->RESET");
                osTimerStop(app_battery_timer);
                app_shutdown();
#else
                app_battery_measure.status = APP_BATTERY_STATUS_NORMAL;
#endif
#endif
            }
            else if (prams.charger == APP_BATTERY_CHARGER_PLUGIN)
            {
#ifdef BT_USB_AUDIO_DUAL_MODE
                TRACE("%s:%d.",__func__,__LINE__);
#endif
            }
            break;
        case APP_BATTERY_STATUS_INVALID:
        default:
            break;
    }

    if (app_battery_charger_handle_process()<=0)
    {
        if (app_status_indication_get() != APP_STATUS_INDICATION_FULLCHARGE)
        {
            TRACE("FULL_CHARGING:%d", app_battery_measure.currvolt);
            app_status_indication_set(APP_STATUS_INDICATION_FULLCHARGE);
#ifdef MEDIA_PLAYER_SUPPORT
#if defined(BT_USB_AUDIO_DUAL_MODE) || defined(IBRT)
#else
            app_voice_report(APP_STATUS_INDICATION_FULLCHARGE, 0);
#endif
#endif
        }
    }

    app_battery_timer_start(APP_BATTERY_MEASURE_PERIODIC_CHARGING);
    
    return 0;
}

static int app_battery_handle_process(APP_MESSAGE_BODY *msg_body)
{
    uint8_t status;
    union APP_BATTERY_MSG_PRAMS msg_prams;

    APP_BATTERY_GET_STATUS(msg_body->message_id, status);
    APP_BATTERY_GET_PRAMS(msg_body->message_id, msg_prams.prams);

    uint32_t generatedSeed = hal_sys_timer_get();
    for (uint8_t index = 0; index < sizeof(bt_addr); index++)
    {
        generatedSeed ^= (((uint32_t)(bt_addr[index])) << (hal_sys_timer_get()&0xF));
    }
    srand(generatedSeed);

    if (status == APP_BATTERY_STATUS_PLUGINOUT){
        app_battery_pluginout_debounce_start();
    }
    else
    {
        switch (app_battery_measure.status)
        {
            case APP_BATTERY_STATUS_NORMAL:
                app_battery_handle_process_normal((uint32_t)status, msg_prams);
                break;
            
            case APP_BATTERY_STATUS_CHARGING:
                app_battery_handle_process_charging((uint32_t)status, msg_prams);
                break;

            default:
                break;
        }
    }
    if (NULL != app_battery_measure.user_cb)
    {
        uint8_t batteryLevel;
#ifdef __INTERCONNECTION__
        APP_BATTERY_INFO_T* pBatteryInfo;
        pBatteryInfo = (APP_BATTERY_INFO_T*)&app_battery_measure.currentBatteryInfo;
        pBatteryInfo->chargingStatus = ((app_battery_measure.status == APP_BATTERY_STATUS_CHARGING)? 1:0);
        batteryLevel = pBatteryInfo->batteryLevel;

#else
        batteryLevel = app_battery_measure.currlevel;
#endif
        app_battery_measure.user_cb(app_battery_measure.currvolt,
                                    batteryLevel, app_battery_measure.status,status,msg_prams);
    }

    return 0;
}

int app_battery_register(APP_BATTERY_CB_T user_cb)
{
    if(NULL == app_battery_measure.user_cb)
    {
        app_battery_measure.user_cb = user_cb;
        return 0;
    }
    return 1;
}

int app_battery_get_info(APP_BATTERY_MV_T *currvolt, uint8_t *currlevel, enum APP_BATTERY_STATUS_T *status)
{
    if (currvolt)
    {
        *currvolt = app_battery_measure.currvolt;
    }

    if (currlevel)
    {
#ifdef __INTERCONNECTION__
        *currlevel = app_battery_measure.currentBatteryInfo;
#else
        *currlevel = app_battery_measure.currlevel;
#endif
    }

    if (status)
    {
        *status = app_battery_measure.status;
    }

    return 0;
}

int app_battery_open(void)
{
    APP_BATTERY_TRACE("%s batt range:%d~%d",__func__, APP_BATTERY_MIN_MV, APP_BATTERY_MAX_MV);
    int nRet = APP_BATTERY_OPEN_MODE_INVALID;
    if (app_battery_timer == NULL)
        app_battery_timer = osTimerCreate (osTimer(APP_BATTERY), osTimerPeriodic, NULL);

    if (app_battery_pluginout_debounce_timer == NULL)
        app_battery_pluginout_debounce_timer = osTimerCreate (osTimer(APP_BATTERY_PLUGINOUT_DEBOUNCE), osTimerOnce, &app_battery_pluginout_debounce_ctx);

    ntc_capture_open();

    app_battery_measure.status = APP_BATTERY_STATUS_NORMAL;
#ifdef __INTERCONNECTION__
    app_battery_measure.currentBatteryInfo = APP_BATTERY_DEFAULT_INFO;
    app_battery_measure.lastBatteryInfo = APP_BATTERY_DEFAULT_INFO;
    app_battery_measure.isMobileSupportSelfDefinedCommand = 0;
#else
    app_battery_measure.currlevel = APP_BATTERY_LEVEL_MAX;
#endif
    app_battery_measure.currvolt = APP_BATTERY_MAX_MV;
    app_battery_measure.lowvolt = APP_BATTERY_MIN_MV;
    app_battery_measure.highvolt = APP_BATTERY_MAX_MV;
    app_battery_measure.pdvolt = APP_BATTERY_PD_MV;
    app_battery_measure.chargetimeout = APP_BATTERY_CHARGE_TIMEOUT_MIN;

    app_battery_measure.periodic = APP_BATTERY_MEASURE_PERIODIC_QTY;
    app_battery_measure.cb = app_battery_event_process;
    app_battery_measure.user_cb = NULL;

    app_battery_measure.charger_status.prevolt = 0;
    app_battery_measure.charger_status.slope_1000_index = 0;
    app_battery_measure.charger_status.cnt = 0;

    app_set_threadhandle(APP_MODUAL_BATTERY, app_battery_handle_process);

    if(app_battery_ext_charger_detecter_cfg.pin != HAL_IOMUX_PIN_NUM)
    {
        hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&app_battery_ext_charger_detecter_cfg, 1);
        hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)app_battery_ext_charger_detecter_cfg.pin, HAL_GPIO_DIR_IN, 1);
    }

    if(app_battery_ext_charger_enable_cfg.pin != HAL_IOMUX_PIN_NUM)
    {
        hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&app_battery_ext_charger_enable_cfg, 1);
        hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)app_battery_ext_charger_enable_cfg.pin, HAL_GPIO_DIR_OUT, 0);
    }
	
	cosonic_trace(battery_manager,true,"");
	
    if (app_battery_charger_indication_open() == APP_BATTERY_CHARGER_PLUGIN)
    {
        app_battery_measure.status = APP_BATTERY_STATUS_CHARGING;        
        app_battery_measure.start_time = hal_sys_timer_get();
        //pmu_charger_plugin_config();
        //if (app_battery_ext_charger_enable_cfg.pin != HAL_IOMUX_PIN_NUM)
        //{
        //	hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)app_battery_ext_charger_enable_cfg.pin, HAL_GPIO_DIR_OUT, 0);
        //}
        
#if (CHARGER_PLUGINOUT_RESET == 0)
        nRet = APP_BATTERY_OPEN_MODE_CHARGING_PWRON;
#else       
        nRet = APP_BATTERY_OPEN_MODE_CHARGING;
#endif
    }
    else
    {        
        app_battery_measure.status = APP_BATTERY_STATUS_NORMAL;
        //pmu_charger_plugout_config();    
        nRet = APP_BATTERY_OPEN_MODE_NORMAL;

        if (app_battery_ext_charger_enable_cfg.pin != HAL_IOMUX_PIN_NUM)
        {
            hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)app_battery_ext_charger_enable_cfg.pin, HAL_GPIO_DIR_OUT, 1);
        }
    }
    return nRet;
}

int app_battery_start(void)
{
    APP_BATTERY_TRACE("%s %d",__func__, APP_BATTERY_MEASURE_PERIODIC_FAST_MS);

    app_battery_timer_start(APP_BATTERY_MEASURE_PERIODIC_FAST);

    return 0;
}

int app_battery_stop(void)
{
    osTimerStop(app_battery_timer);

    return 0;
}

int app_battery_close(void)
{
    hal_gpadc_close(HAL_GPADC_CHAN_BATTERY);

    return 0;
}


int32_t app_battery_charger_slope_calc(int32_t t1, int32_t v1, int32_t t2, int32_t v2)
{
    int32_t slope_1000;
    slope_1000 = (v2-v1)*1000/(t2-t1);
    return slope_1000;
}

static int app_battery_charger_handle_reset(void)
{
    overvolt_full_charge_cnt = 0;
    ext_pin_full_charge_cnt = 0;
    return 0;
}

static int app_battery_charger_handle_process(void)
{
    int nRet = 1;
    //uint32_t charging_min;

    // charging_min = hal_sys_timer_get() - app_battery_measure.start_time;
    // charging_min = TICKS_TO_MS(charging_min)/1000/60;
    // if (charging_min >= app_battery_measure.chargetimeout)
    // {
    //     TRACE("TIMEROUT-->FULL_CHARGING");
    //     nRet = -1;
    //     goto exit;
    // }

    TRACE("app_battery_charger_handle_process currvolt:%d chg:%d", app_battery_measure.currvolt,
                                                            hal_gpio_pin_get_val((enum HAL_GPIO_PIN_T)app_battery_ext_charger_detecter_cfg.pin));

#ifndef CHIP_BEST1400
    if ((app_battery_measure.charger_status.cnt%APP_BATTERY_CHARGING_OVERVOLT_MEASURE_CNT) == 0)
    {
        if (overvolt_full_charge_cnt>=APP_BATTERY_CHARGING_OVERVOLT_DEDOUNCE_CNT)
        {
            goto exit;
        }
        else if (app_battery_measure.currvolt>=(app_battery_measure.highvolt+APP_BATTERY_CHARGE_OFFSET_MV))
        {
            overvolt_full_charge_cnt++;
        }
        else
        {
            overvolt_full_charge_cnt = 0;
        }
        if (overvolt_full_charge_cnt>=APP_BATTERY_CHARGING_OVERVOLT_DEDOUNCE_CNT)
        {
            TRACE("OVERVOLT-->FULL_CHARGING");
            overvolt_full_charge_cnt = 0;
            nRet = -1;
            goto exit;
        }
    }
#endif

    if ((app_battery_measure.charger_status.cnt%APP_BATTERY_CHARGING_EXTPIN_MEASURE_CNT) == 0)
    {
        if (app_battery_ext_charger_detecter_cfg.pin != HAL_IOMUX_PIN_NUM)
        {            
            if (ext_pin_full_charge_cnt>=APP_BATTERY_CHARGING_EXTPIN_DEDOUNCE_CNT)
            {
                goto exit;
            }
            else if (hal_gpio_pin_get_val((enum HAL_GPIO_PIN_T)app_battery_ext_charger_detecter_cfg.pin))
            {
                ext_pin_full_charge_cnt++;
            }
            else
            {
                ext_pin_full_charge_cnt = 0;
            }
            if (ext_pin_full_charge_cnt>=APP_BATTERY_CHARGING_EXTPIN_DEDOUNCE_CNT)
            {
                TRACE("EXT PIN-->FULL_CHARGING");
                ext_pin_full_charge_cnt = 0;
                nRet = -1;
                goto exit;
            }
        }
    }
#ifndef CHIP_BEST1400
    if ((app_battery_measure.charger_status.cnt%APP_BATTERY_CHARGING_SLOPE_MEASURE_CNT) == 0)
    {        
        int8_t i=0,cnt=0;
        uint32_t slope_1000 = 0;
        if (!app_battery_measure.charger_status.prevolt)
        {
            app_battery_measure.charger_status.slope_1000[app_battery_measure.charger_status.slope_1000_index%APP_BATTERY_CHARGING_SLOPE_TABLE_COUNT] = slope_1000;
            app_battery_measure.charger_status.prevolt = app_battery_measure.currvolt;
            for (i=0; i<APP_BATTERY_CHARGING_SLOPE_TABLE_COUNT; i++)
            {
                app_battery_measure.charger_status.slope_1000[i]=100;
            }
        }
        else
        {
            slope_1000 = app_battery_charger_slope_calc(0, app_battery_measure.charger_status.prevolt,
                         APP_BATTERY_CHARGING_PERIODIC_MS*APP_BATTERY_CHARGING_SLOPE_MEASURE_CNT/1000, app_battery_measure.currvolt);
            app_battery_measure.charger_status.slope_1000[app_battery_measure.charger_status.slope_1000_index%APP_BATTERY_CHARGING_SLOPE_TABLE_COUNT] = slope_1000;
            app_battery_measure.charger_status.prevolt = app_battery_measure.currvolt;
            for (i=0; i<APP_BATTERY_CHARGING_SLOPE_TABLE_COUNT; i++)
            {
                if (app_battery_measure.charger_status.slope_1000[i]>0)
                    cnt++;
                else
                    cnt--;
                TRACE("slope_1000[%d]=%d cnt:%d", i,app_battery_measure.charger_status.slope_1000[i], cnt);
            }
            TRACE("app_battery_charger_slope_proc slope*1000=%d cnt:%d nRet:%d", slope_1000, cnt, nRet);
            if (cnt>1)
            {
                nRet = 1;
            }/*else (3>=cnt && cnt>=-3){
                nRet = 0;
            }*/else
            {
                if (app_battery_measure.currvolt>=(app_battery_measure.highvolt-APP_BATTERY_CHARGE_OFFSET_MV))
                {
                    TRACE("SLOPE-->FULL_CHARGING");
                    nRet = -1;
                }
            }
        }
        app_battery_measure.charger_status.slope_1000_index++;
    }
#endif
    app_battery_measure.charger_status.cnt++;
exit:
    return nRet;
}

#if 1
static enum APP_BATTERY_CHARGER_T app_battery_charger_forcegetstatus(void)
{
    enum APP_BATTERY_CHARGER_T status = APP_BATTERY_CHARGER_QTY;
    enum PMU_CHARGER_STATUS_T charger;

    charger = pmu_charger_get_status();

    if (charger == PMU_CHARGER_PLUGIN)
    {
        status = APP_BATTERY_CHARGER_PLUGIN;
        // TRACE("force APP_BATTERY_CHARGER_PLUGIN");
    }
    else
    {
        status = APP_BATTERY_CHARGER_PLUGOUT;
        // TRACE("force APP_BATTERY_CHARGER_PLUGOUT");
    }

    return status;
}
#endif

static void app_battery_charger_handler(enum PMU_CHARGER_STATUS_T status)
{
    TRACE("%s: status=0x%02x", __func__, status);

    pmu_charger_set_irq_handler(NULL);
    app_battery_event_process(APP_BATTERY_STATUS_PLUGINOUT,
                              (status == PMU_CHARGER_PLUGIN) ? APP_BATTERY_CHARGER_PLUGIN : APP_BATTERY_CHARGER_PLUGOUT);
}

static void app_battery_pluginout_debounce_start(void)
{
    TRACE("%s", __func__);
    app_battery_pluginout_debounce_ctx = (uint32_t)app_battery_charger_forcegetstatus();
    app_battery_pluginout_debounce_cnt = 1;
    osTimerStart(app_battery_pluginout_debounce_timer, CHARGER_PLUGINOUT_DEBOUNCE_MS);
}

static void app_battery_pluginout_debounce_handler(void const *param)
{
    enum APP_BATTERY_CHARGER_T status_charger = app_battery_charger_forcegetstatus();

	TRACE("%s", __func__);
    if(app_battery_pluginout_debounce_ctx == (uint32_t) status_charger){
        app_battery_pluginout_debounce_cnt++;
    }
    else
    {
    	TRACE("%s dithering cnt", __func__, app_battery_pluginout_debounce_cnt);
		app_battery_pluginout_debounce_cnt = 0;
        app_battery_pluginout_debounce_ctx = (uint32_t)status_charger;
    }
    
    if (app_battery_pluginout_debounce_cnt >= CHARGER_PLUGINOUT_DEBOUNCE_CNT){        
        TRACE("%s %s", __func__, status_charger == APP_BATTERY_CHARGER_PLUGOUT ? "PLUGOUT" : "PLUGIN");
        app_battery_charger_handle_reset();

        if (status_charger == APP_BATTERY_CHARGER_PLUGIN)
        {//入盒处理
            if (app_battery_ext_charger_enable_cfg.pin != HAL_IOMUX_PIN_NUM)
            {
                hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)app_battery_ext_charger_enable_cfg.pin, HAL_GPIO_DIR_OUT, 0);
                //plug_in_start();
                cosonic_trace(battery_manager,true,"APP_BATTERY_CHARGER_PLUGIN");
            }
            app_battery_measure.start_time = hal_sys_timer_get();
        }
        else
        {//出盒处理
            if (app_battery_ext_charger_enable_cfg.pin != HAL_IOMUX_PIN_NUM)
            {
                hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)app_battery_ext_charger_enable_cfg.pin, HAL_GPIO_DIR_OUT, 1);
                //plug_out_start();
                cosonic_trace(battery_manager,true,"APP_BATTERY_CHARGER_PLUGOUT");
            }
        }
        app_battery_event_process(APP_BATTERY_STATUS_CHARGING, status_charger);        
        pmu_charger_set_irq_handler(app_battery_charger_handler);
        osTimerStop(app_battery_pluginout_debounce_timer);
    }else{
        osTimerStart(app_battery_pluginout_debounce_timer, CHARGER_PLUGINOUT_DEBOUNCE_MS);
    }
}

int app_battery_charger_indication_open(void)
{
    enum APP_BATTERY_CHARGER_T status = APP_BATTERY_CHARGER_QTY;
    uint8_t cnt = 0;

    APP_BATTERY_TRACE("%s",__func__);

    pmu_charger_init();

    do
    {
        status = app_battery_charger_forcegetstatus();
        if (status == APP_BATTERY_CHARGER_PLUGIN)
            break;
        osDelay(20);
    }
    while(cnt++<5);

    if (app_battery_ext_charger_indicator_cfg.pin != HAL_IOMUX_PIN_NUM)
    {
		if(hal_gpio_pin_get_val((enum HAL_GPIO_PIN_T)app_battery_ext_charger_indicator_cfg.pin))
        {
            status = APP_BATTERY_CHARGER_PLUGIN;
        }
	}
	
    pmu_charger_set_irq_handler(app_battery_charger_handler);
	
    return status;
}

int8_t app_battery_current_level(void)
{
#ifdef __INTERCONNECTION__
    return app_battery_measure.currentBatteryInfo & 0x7f;
#else
    return app_battery_measure.currlevel;
#endif
}

static void ntc_Timer_handle(void const *param);
osTimerDef (APP_NTC, ntc_Timer_handle);				
static osTimerId app_ntc_id = NULL; 

typedef uint16_t NTP_VOLTAGE_MV_T;
typedef uint16_t NTP_TEMPERATURE_C_T;

#define NTC_CAPTURE_STABLE_COUNT (5)
#define NTC_CAPTURE_TEMPERATURE_STEP (4)
#define NTC_CAPTURE_TEMPERATURE_REF (15)
#define NTC_CAPTURE_VOLTAGE_REF (1100)

typedef void (*NTC_CAPTURE_MEASURE_CB_T)(NTP_TEMPERATURE_C_T);

struct NTC_CAPTURE_MEASURE_T
{
    NTP_VOLTAGE_MV_T currvolt;
    NTP_VOLTAGE_MV_T voltage[NTC_CAPTURE_STABLE_COUNT];
    uint16_t index;
};

static struct NTC_CAPTURE_MEASURE_T ntc_capture_measure;

void ntc_capture_irqhandler(uint16_t irq_val, HAL_GPADC_MV_T volt)
{
    uint32_t meanVolt = 0;
    uint8_t  i=0x00;
    TRACE("%s %d irq:0x%04x",__func__, volt, irq_val);

    if (volt == HAL_GPADC_BAD_VALUE)
    {
        //打印
        return;
    }
	ntc_capture_measure.voltage[ntc_capture_measure.index++]=volt;//读取ADC存到缓冲区

	if(ntc_capture_measure.index>=10)//连续读陬10次电压值
	{
		for(i=0;i<10;i++)
		{
			meanVolt+=ntc_capture_measure.voltage[i];//电压值累加
		}
		ntc_capture_measure.currvolt=meanVolt/10;//取平均值
		TRACE("ntc_capture_measure.currvolt :0x%04x",ntc_capture_measure.currvolt);
		//NTC温度控制  
		// 低于-20或者高于60度自动关机     
		//温度大于0度小于15度1C充电
		//温度小于0度或者高于45度 停止充电
		//15-45度  2C充电
		ntc_capture_Close();
	}else{
		osTimerStart(app_ntc_id, 100);//100MS定时器   
	}

}


/**
 * @description: 
 * @param {*}
 * @return {*}
 * @author: jackdong
 * @Date: 2021-04-10 09:48:59
 */
int ntc_capture_open(void)
{
	hal_gpio_pin_set((enum HAL_GPIO_PIN_T)cfg_anker_pinmux_ntc.pin);//NTC供电
	ntc_capture_measure.currvolt = 0;
	ntc_capture_measure.index = 0;
	ntc_Timer_Start();
    return 0;
}

/**
 * @description: 关闭NTC检测
 * @param void
 * @return  none
 * @author: jackdong
 * @Date: 2021-04-09 16:48:16
 */
void ntc_capture_Close(void)
{
	osTimerStop(app_ntc_id);
	hal_gpio_pin_clr((enum HAL_GPIO_PIN_T)cfg_anker_pinmux_ntc.pin);//拉低供电IO
	hal_gpadc_close(HAL_GPADC_CHAN_3);//关闭ADC检测
}

/**
 * @description: ntc_Timer_Start   NTC定时器
 * @param {*}
 * @return {*}
 * @author: jackdong
 * @Date: 2021-04-10 08:53:19
 */
void ntc_Timer_Start(void)
{	
	if(app_ntc_id == NULL)//检查是否创建
	{
		app_ntc_id = osTimerCreate(osTimer(APP_NTC), osTimerOnce, NULL);//创建NTC定时器
	}
	if (app_ntc_id != NULL)
	{
		osTimerStart(app_ntc_id, 100);//启动定时器
	}
}


/**
 * @description: ntc_Timer_handle 定时器中断函数
 * @param {void const} *param
 * @return {*}
 * @author: jackdong
 * @Date: 2021-04-10 08:48:36
 */
static void ntc_Timer_handle(void const *param)
{
    hal_gpadc_open(HAL_GPADC_CHAN_3, HAL_GPADC_ATP_ONESHOT, ntc_capture_irqhandler);//ADC 采集并且回调
}



uint16_t AT_app_battery_get_current_volt(void)
{
	return app_battery_measure.currvolt;
}

