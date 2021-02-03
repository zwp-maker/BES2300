
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
#ifndef __PLATFORM_DEPS_H__
#define __PLATFORM_DEPS_H__

/* all platform dependences shoule be included only here */

#include "cmsis_os.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "string.h"
#include "tgt_hardware.h"
#include "bt_drv_reg_op.h"
#include "intersyshci.h"
#include "besbt_cfg.h"
#include "osif.h"
#include "ddbif.h"

#if defined(__cplusplus)
extern "C" {
#endif

void Plt_HciSendData(unsigned char type, unsigned short cmd_conn, unsigned short len, unsigned char *buffer);
void Plt_HciSendBuffer(unsigned char type, unsigned char *buff, int len);
void Plt_NotifyScheduler(void);
void Plt_LockHCIBuffer(void);
void Plt_UNLockHCIBuffer(void);

#define Plt_Assert ASSERT

char OS_Init(void);
void OS_LockStack(void);
void OS_UnlockStack(void);
unsigned char OS_LockIsExist(void);
void OS_StopHardware(void);
void OS_ResumeHardware(void);
void OS_NotifyEvm(void);

void Plt_TimerWrapperInit(void);
void Plt_TimerWrapperStart(unsigned int ms);
void Plt_TimerWrapperStop(void);
void Plt_UNLockTimerWrapper(void);
void Plt_LockTimerWrapper(void);
unsigned int Plt_GetTicks(void);
unsigned int Plt_GetTicksMax(void);

#define Plt_TICKS_TO_MS(ticks) TICKS_TO_MS(ticks)

#if defined(__cplusplus)
}
#endif

#endif /* __PLATFORM_DEPS_H__ */