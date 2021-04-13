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
#include "cp_accel.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_cmu.h"
#include "hal_location.h"
#include "hal_mcu2cp.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "mpu.h"
#include "stdarg.h"
#include "system_cp.h"

#ifdef CP_ACCEL_DEBUG
#define CP_ACCEL_TRACE(s, ...)              TRACE(s, ##__VA_ARGS__)
#else
#define CP_ACCEL_TRACE(s, ...)
#endif

#define CP_NO_FLASH_ACCESS

#define CP_CRASH_START_TIMEOUT              MS_TO_TICKS(100)
#define CP_TRACE_FLUSH_TIMEOUT              MS_TO_TICKS(200)
#define CP_CRASH_DUMP_TIMEOUT               MS_TO_TICKS(500)
#define CP_TRACE_BUF_FULL_INTVL             MS_TO_TICKS(50)

enum CP_ACCEL_STATE_T {
    CP_ACCEL_STATE_CLOSED = 0,
    CP_ACCEL_STATE_OPENING,
    CP_ACCEL_STATE_OPENED,
    CP_ACCEL_STATE_CLOSING,
};

enum CP_SYS_EVENT_T {
    CP_SYS_EVENT_NONE = 0,
    CP_SYS_EVENT_CRASH_START,
    CP_SYS_EVENT_CRASH_END,
    CP_SYS_EVENT_TRACE_FLUSH,
    CP_SYS_EVENT_TRACE_BUF_FULL,
};

static enum CP_ACCEL_STATE_T cp_accel_state = CP_ACCEL_STATE_CLOSED;
static bool ram_inited;
static bool cp_accel_inited;
static CP_ACCEL_EVT_HDLR mcu_evt_hdlr;
static CP_ACCEL_EVT_HDLR mcu_sys_ctrl_hdlr;

static CP_ACCEL_CP_MAIN cp_work_main;
static CP_ACCEL_EVT_HDLR cp_evt_hdlr;

static CP_BSS_LOC volatile enum CP_SYS_EVENT_T cp_sys_evt;
static CP_BSS_LOC bool cp_in_crash;
static CP_BSS_LOC volatile uint8_t cp_irq_cnt;
static CP_BSS_LOC volatile uint8_t cp_in_sleep;
static CP_BSS_LOC uint32_t cp_buf_full_time;

static CP_TEXT_SRAM_LOC int CP_TEXT_SRAM_LOC send_sys_ctrl_cp2mcu(uint32_t event)
{
    return hal_mcu2cp_send_cp(HAL_MCU2CP_ID_1, HAL_MCU2CP_MSG_TYPE_0, (unsigned char *)event, 0);
}

static CP_TEXT_SRAM_LOC void cp_trace_crash_notify(enum HAL_TRACE_STATE_T state)
{
    uint32_t time;

    if (state == HAL_TRACE_STATE_CRASH_ASSERT_START || state == HAL_TRACE_STATE_CRASH_FAULT_START) {
        cp_in_crash = true;
        cp_sys_evt = CP_SYS_EVENT_CRASH_START;
        mpu_close();
        send_sys_ctrl_cp2mcu(0);

        time = hal_sys_timer_get();
        while (cp_sys_evt == CP_SYS_EVENT_CRASH_START &&
                hal_sys_timer_get() - time < CP_CRASH_START_TIMEOUT);
    } else {
        cp_sys_evt = CP_SYS_EVENT_CRASH_END;
    }
}

static CP_TEXT_SRAM_LOC void cp_trace_buffer_ctrl(enum HAL_TRACE_BUF_STATE_T buf_state)
{
    uint32_t time;

    if (cp_sys_evt != CP_SYS_EVENT_NONE) {
        return;
    }

    time = hal_sys_timer_get();

    if (buf_state == HAL_TRACE_BUF_STATE_FLUSH) {
        cp_sys_evt = CP_SYS_EVENT_TRACE_FLUSH;
        if (!cp_in_crash) {
            send_sys_ctrl_cp2mcu(0);
        }

        while (cp_sys_evt == CP_SYS_EVENT_TRACE_FLUSH &&
                hal_sys_timer_get() - time < CP_TRACE_FLUSH_TIMEOUT);
    } else if (buf_state == HAL_TRACE_BUF_STATE_FULL || buf_state == HAL_TRACE_BUF_STATE_NEAR_FULL) {
        if (time - cp_buf_full_time >= CP_TRACE_BUF_FULL_INTVL) {
            cp_buf_full_time = time;
            if (!cp_in_crash) {
                cp_sys_evt = CP_SYS_EVENT_TRACE_BUF_FULL;
                send_sys_ctrl_cp2mcu(0);
            }
        }
    }
}

static SRAM_TEXT_LOC unsigned int cp2mcu_sys_arrived(const unsigned char *data, unsigned int len)
{
    uint32_t time;

    if (cp_sys_evt == CP_SYS_EVENT_TRACE_FLUSH) {
        TRACE_FLUSH();
        cp_sys_evt = CP_SYS_EVENT_NONE;
    } else if (cp_sys_evt == CP_SYS_EVENT_TRACE_BUF_FULL) {
        TRACE("");
        cp_sys_evt = CP_SYS_EVENT_NONE;
    } else if (cp_sys_evt == CP_SYS_EVENT_CRASH_START) {
        cp_sys_evt = CP_SYS_EVENT_NONE;

        TRACE("");
        TRACE("CP Crash starts ...");

        // Wait CP crash dump finishes in interrupt context
        time = hal_sys_timer_get();
        while (cp_sys_evt != CP_SYS_EVENT_CRASH_END &&
                hal_sys_timer_get() - time < CP_CRASH_DUMP_TIMEOUT) {
            if (cp_sys_evt == CP_SYS_EVENT_TRACE_FLUSH) {
                TRACE_FLUSH();
                cp_sys_evt = CP_SYS_EVENT_NONE;
            }
        }

        if (mcu_sys_ctrl_hdlr) {
            mcu_sys_ctrl_hdlr(CP_SYS_EVENT_CRASH_END);
        }

        TRACE("CP Crash ends ...");
        TRACE("");
    }

    return len;
}

static CP_TEXT_SRAM_LOC unsigned int mcu2cp_msg_arrived(const unsigned char *data, unsigned int len)
{
    cp_irq_cnt = 1;

    if (cp_evt_hdlr) {
        cp_evt_hdlr((uint32_t)data);
    }

    return len;
}

static CP_TEXT_SRAM_LOC NOINLINE void accel_loop(void)
{
    uint32_t lock;
    uint32_t cnt;

    mpu_null_check_enable();
#ifdef CP_NO_FLASH_ACCESS
    mpu_set(MPU_ID_2, FLASHX_BASE,      0x4000000, MPU_ATTR_NO_ACCESS);
    mpu_set(MPU_ID_3, FLASH_BASE,       0x4000000, MPU_ATTR_NO_ACCESS);
    mpu_set(MPU_ID_4, FLASH_NC_BASE,    0x4000000, MPU_ATTR_NO_ACCESS);
#endif

    while (1) {
        lock = int_lock();
        cnt = cp_irq_cnt;
        cp_irq_cnt = 0;
        if (cnt == 0) {
            cp_in_sleep = true;
            __WFI();
            cp_in_sleep = false;
        }
        int_unlock(lock);

        while (cnt > 0) {
            if (cp_work_main) {
                cp_work_main();
            }
            cnt = 0;
        }
    }
}

static void accel_main(void)
{
    system_cp_init(!ram_inited);

    ram_inited = true;

    hal_trace_open_cp();

    hal_mcu2cp_open_cp(HAL_MCU2CP_ID_0, HAL_MCU2CP_MSG_TYPE_0, mcu2cp_msg_arrived, NULL, false);
    hal_mcu2cp_open_cp(HAL_MCU2CP_ID_1, HAL_MCU2CP_MSG_TYPE_0, NULL, NULL, false);

    hal_mcu2cp_start_recv_cp(HAL_MCU2CP_ID_0);
    //hal_mcu2cp_start_recv_cp(HAL_MCU2CP_ID_1);

    cp_accel_inited = true;

    accel_loop();
}

static SRAM_TEXT_LOC unsigned int cp2mcu_msg_arrived(const unsigned char *data, unsigned int len)
{
    if (mcu_evt_hdlr) {
        mcu_evt_hdlr((uint32_t)data);
    }

    return len;
}

int cp_accel_open(CP_ACCEL_CP_MAIN cp_main, CP_ACCEL_EVT_HDLR cp_hdlr, CP_ACCEL_EVT_HDLR mcu_hdlr, CP_ACCEL_EVT_HDLR mcu_sys_hdlr)
{
    if (cp_accel_state != CP_ACCEL_STATE_CLOSED) {
        return 1;
    }

    cp_accel_state = CP_ACCEL_STATE_OPENING;
    cp_work_main = cp_main;
    cp_evt_hdlr = cp_hdlr;
    mcu_evt_hdlr = mcu_hdlr;
    mcu_sys_ctrl_hdlr = mcu_sys_hdlr;

    hal_trace_cp_register(cp_trace_crash_notify, cp_trace_buffer_ctrl);

    hal_cmu_cp_enable(RAMCP_BASE + RAMCP_SIZE, (uint32_t)accel_main);

    hal_mcu2cp_open_mcu(HAL_MCU2CP_ID_0, HAL_MCU2CP_MSG_TYPE_0, cp2mcu_msg_arrived, NULL, false);
    hal_mcu2cp_open_mcu(HAL_MCU2CP_ID_1, HAL_MCU2CP_MSG_TYPE_0, cp2mcu_sys_arrived, NULL, false);

    hal_mcu2cp_start_recv_mcu(HAL_MCU2CP_ID_0);
    hal_mcu2cp_start_recv_mcu(HAL_MCU2CP_ID_1);

    cp_accel_state = CP_ACCEL_STATE_OPENED;

    return 0;
}

int cp_accel_close(void)
{
    cp_accel_state = CP_ACCEL_STATE_CLOSING;
    cp_accel_inited = false;

    hal_mcu2cp_close_mcu(HAL_MCU2CP_ID_0, HAL_MCU2CP_MSG_TYPE_0);
    hal_mcu2cp_close_mcu(HAL_MCU2CP_ID_1, HAL_MCU2CP_MSG_TYPE_0);

    hal_cmu_cp_disable();

    system_cp_term();

    hal_mcu2cp_close_cp(HAL_MCU2CP_ID_0, HAL_MCU2CP_MSG_TYPE_0);
    hal_mcu2cp_close_cp(HAL_MCU2CP_ID_1, HAL_MCU2CP_MSG_TYPE_0);

    hal_trace_cp_register(NULL, NULL);

    cp_accel_state = CP_ACCEL_STATE_CLOSED;

    return 0;
}

int SRAM_TEXT_LOC cp_accel_init_done(void)
{
    return cp_accel_inited;
}

int SRAM_TEXT_LOC cp_accel_send_event_mcu2cp(uint32_t event)
{
    return hal_mcu2cp_send_mcu(HAL_MCU2CP_ID_0, HAL_MCU2CP_MSG_TYPE_0, (unsigned char *)event, 0);
}

int CP_TEXT_SRAM_LOC cp_accel_send_event_cp2mcu(uint32_t event)
{
    return hal_mcu2cp_send_cp(HAL_MCU2CP_ID_0, HAL_MCU2CP_MSG_TYPE_0, (unsigned char *)event, 0);
}

int SRAM_TEXT_LOC cp_accel_busy(void)
{
    if (cp_accel_state != CP_ACCEL_STATE_CLOSED) {
        if (cp_accel_state == CP_ACCEL_STATE_OPENED && cp_in_sleep &&
                !hal_mcu2cp_local_irq_pending_cp(HAL_MCU2CP_ID_0)) {
            return false;
        }
        return true;
    }

    return false;
}

