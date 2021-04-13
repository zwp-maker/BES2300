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
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "hal_wdt.h"
#include "hal_sleep.h"
#include "pmu.h"
#include "analog.h"
#include "app_thread.h"
#include "app_utils.h"

#define APP_WDT_THREAD_PING_TIME  MS_TO_HWTICKS(1000)

static uint32_t app_wdt_ping_time_thr = 0;
static uint32_t last_wdk_kick = 0;
static uint32_t wdk_kick_onprocess = 0;

int app_sysfreq_req(enum APP_SYSFREQ_USER_T user, enum APP_SYSFREQ_FREQ_T freq)
{
    int ret;

    ret = hal_sysfreq_req((enum HAL_SYSFREQ_USER_T)user, (enum HAL_CMU_FREQ_T)freq);

    return ret;
}

void app_wdt_ping(void)
{
//    TRACE("app_wdt_ping:%d enter",  HWTICKS_TO_MS(hal_sys_timer_get()));
    hal_wdt_ping(HAL_WDT_ID_0);
#ifndef CHIP_BEST2000
    pmu_wdt_feed();
#endif
//    TRACE("app_wdt_ping:%d exit",  HWTICKS_TO_MS(hal_sys_timer_get()));
}

void app_wdt_thread_ping(void)
{
    uint32_t curtime = 0;

    curtime = hal_sys_timer_get();
    if (hal_timer_get_passed_ticks(curtime, last_wdk_kick) > APP_WDT_THREAD_PING_TIME){
        wdk_kick_onprocess = true;
        app_wdt_ping();
        wdk_kick_onprocess = false;
        last_wdk_kick = curtime;
    }
}

uint32_t idle_watchdog_ping_wdk_kick(void);
uint32_t idle_watchdog_ping_wdk_onprocess(void);

static void pmu_wdt_irq_handle(void)
{
    TRACE_IMM("%s %d/%d %d/%d %d/%d", __func__, wdk_kick_onprocess, idle_watchdog_ping_wdk_onprocess(),
                              last_wdk_kick, idle_watchdog_ping_wdk_kick(),
                              HWTICKS_TO_MS(last_wdk_kick), HWTICKS_TO_MS(idle_watchdog_ping_wdk_kick()));
    analog_aud_codec_mute();
    ASSERT(0, "%s", __func__);
}
static void app_wdt_irq_handle(enum HAL_WDT_ID_T id, uint32_t status)
{
    TRACE_IMM("%s %d/%d %d/%d %d/%d", __func__, wdk_kick_onprocess, idle_watchdog_ping_wdk_onprocess(),
                              last_wdk_kick, idle_watchdog_ping_wdk_kick(),
                              HWTICKS_TO_MS(last_wdk_kick), HWTICKS_TO_MS(idle_watchdog_ping_wdk_kick()));
    analog_aud_codec_mute();
    ASSERT(0, "%s id:%d status:%d",__func__, id, status);
}

int app_wdt_open(int seconds)
{
    uint32_t lock = int_lock();

    hal_wdt_set_irq_callback(HAL_WDT_ID_0, app_wdt_irq_handle);
    hal_wdt_set_timeout(HAL_WDT_ID_0, seconds);
    hal_wdt_start(HAL_WDT_ID_0);
    pmu_wdt_set_irq_handler(pmu_wdt_irq_handle);
#ifndef CHIP_BEST2000
    pmu_wdt_config(seconds * 1100, seconds * 1100);
    pmu_wdt_start();
#endif
    app_wdt_ping_time_thr = MS_TO_TICKS(seconds * 1000 / 5);
    int_unlock(lock);
 
    return 0;
}

int app_wdt_reopen(int seconds)
{
    uint32_t lock = int_lock();
    hal_wdt_stop(HAL_WDT_ID_0);
    hal_wdt_set_timeout(HAL_WDT_ID_0, seconds);
    hal_wdt_start(HAL_WDT_ID_0);
#ifndef CHIP_BEST2000
    pmu_wdt_config(seconds * 1000, seconds * 1000);
    pmu_wdt_start();
#endif
    app_wdt_ping_time_thr = MS_TO_TICKS(seconds * 1000 / 5);
    int_unlock(lock);

    return 0;
}

int app_wdt_close(void)
{
    uint32_t lock = int_lock();
    hal_wdt_stop(HAL_WDT_ID_0);
#ifndef CHIP_BEST2000
    pmu_wdt_stop();
#endif
    int_unlock(lock);

    return 0;
}

