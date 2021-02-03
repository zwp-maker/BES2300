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
#include "stdbool.h"
#include "hal_trace.h"
#include "app_pwl.h"
#include "app_status_ind.h"
#include "string.h"
#include "cosonic.h"

static APP_STATUS_INDICATION_T app_status = APP_STATUS_INDICATION_NUM;
static APP_STATUS_INDICATION_T app_status_ind_filter = APP_STATUS_INDICATION_NUM;
static int app_status_indication_execute(void);

int app_status_indication_filter_set(APP_STATUS_INDICATION_T status)
{
    app_status_ind_filter = status;
    return 0;
}

APP_STATUS_INDICATION_T app_status_indication_get(void)
{
    return app_status;
}

int led_next_status[10]={0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
int head=0;//指向下一个状态
int end=0;//指向下一个空位
int scanner_flag=0;//scanner执行标志

int app_status_indication_set(APP_STATUS_INDICATION_T status){
        
    for(int i=head;i!=end;i++)
    {   i%=10;
        if (led_next_status[i]==status)
        return 0;//若存在与数组已有状态相同则跳过
    } 
    
    if(status==APP_STATUS_INDICATION_CHARGING)//如果是充电指示灯则直接运行，清空数组，关闭scanner
    {
        led_next_status[end]=status;//添加元素
        head=end;
        end=(end+1)%10;//尾指针后移一位
        scanner_flag=0;
        osTimerStop(timer1_handle);
    }
    else {
        led_next_status[end]=status;//添加元素
        end=(end+1)%10;//尾指针后移一位
    }

    if(scanner_flag==0)//如果scanner没有运行则启动；
    app_status_indication_scanner();

    return 1;
}

int app_status_indication_scanner(void){
    scanner_flag=1;
    while (head!=end){
            if ((app_status==APP_STATUS_INDICATION_POWERON) || 
                (app_status==APP_STATUS_INDICATION_CONNECTED) ||
                (app_status==APP_STATUS_INDICATION_PAIRSUCCEED)){
                if(delay_flag==0){
                    led_delay_start(app_status,led_next_status[head]);
                    return 0;
                }
                else delay_flag=0;
            }
        app_status_indication_execute();
        head=(head+1)%10;
    }
    scanner_flag=0;
    return 0;
}

static int app_status_indication_execute(void){
    struct APP_PWL_CFG_T cfg0;
    struct APP_PWL_CFG_T cfg1;

    TRACE("%s %d",__func__, led_next_status[head]);

    // if (app_status == status)
    //     return 0;
    
    // if (app_status_ind_filter == status)
    //     return 0;


    app_status = led_next_status[head];
    memset(&cfg0, 1, sizeof(struct APP_PWL_CFG_T));
    memset(&cfg1, 1, sizeof(struct APP_PWL_CFG_T));
    app_pwl_stop(APP_PWL_ID_0);
    app_pwl_stop(APP_PWL_ID_1);
    switch (led_next_status[head]) {
        case APP_STATUS_INDICATION_POWERON:
            cfg0.part[0].level = 1;
            cfg0.part[0].time = (1000);
            cfg0.part[1].level = 0;
            cfg0.part[1].time = (1000);
            cfg0.parttotal = 2;
            cfg0.startlevel = 0;
            cfg0.periodic = false;

            app_pwl_setup(APP_PWL_ID_0, &cfg0);
            app_pwl_start(APP_PWL_ID_0);
            break;
        case APP_STATUS_INDICATION_INITIAL:
            break;
        case APP_STATUS_INDICATION_CONNECTED:
            cfg0.part[0].level = 1;
            cfg0.part[0].time = (200);
            cfg0.part[1].level = 0;
            cfg0.part[1].time = (200);
            cfg0.periodic = false;
            app_pwl_setup(APP_PWL_ID_0, &cfg0);
            app_pwl_start(APP_PWL_ID_0);
            break;
        case APP_STATUS_INDICATION_CONNECTING:
            cfg1.part[0].level = 1;
            cfg1.part[0].time = (300);
            cfg1.part[1].level = 0;
            cfg1.part[1].time = (5000);
            cfg1.parttotal = 2;
            cfg1.startlevel = 0;
            cfg1.periodic = true;
            app_pwl_setup(APP_PWL_ID_1, &cfg1);
            app_pwl_start(APP_PWL_ID_1);
            break;
        case APP_STATUS_INDICATION_BOTHSCAN:
        case APP_STATUS_INDICATION_PAIRING:
            cfg0.part[0].level = 1;
            cfg0.part[0].time = (400);
            cfg0.part[1].level = 0;
            cfg0.part[1].time = (400);
            cfg0.part[2].level = 1;
            cfg0.part[2].time = (400);
            cfg0.part[3].level = 0;
            cfg0.part[3].time = (400);
            cfg0.part[3].level = 1;
            cfg0.part[3].time = (400);


            cfg0.parttotal = 4;
            cfg0.startlevel = 0;
            cfg0.periodic = true;

            cfg1.part[0].level = 0;
            cfg1.part[0].time = (400);
            cfg1.part[1].level = 1;
            cfg1.part[1].time = (400);
            cfg1.part[2].level = 0;
            cfg1.part[2].time = (400);
            cfg1.part[3].level = 1;
            cfg1.part[3].time = (400);

            cfg1.parttotal = 4;
            cfg1.startlevel = 1;
            cfg1.periodic = true;

            app_pwl_setup(APP_PWL_ID_0, &cfg0);
            app_pwl_start(APP_PWL_ID_0);
            app_pwl_setup(APP_PWL_ID_1, &cfg1);
            app_pwl_start(APP_PWL_ID_1);
            break;
        case APP_STATUS_INDICATION_PAIRSUCCEED:
            cfg0.part[0].level = 0;
            cfg0.part[0].time = (1000);

            cfg0.parttotal = 1;
            cfg0.startlevel = 1;
            cfg0.periodic = false;
            app_pwl_setup(APP_PWL_ID_0, &cfg0);
            app_pwl_start(APP_PWL_ID_0);
            break;
        case APP_STATUS_INDICATION_CHARGING:
            cfg1.part[0].level = 1;
            cfg1.part[0].time = (5000);
            cfg1.parttotal = 1;
            cfg1.startlevel = 1;
            cfg1.periodic = true;
            app_pwl_setup(APP_PWL_ID_1, &cfg1);
            app_pwl_start(APP_PWL_ID_1);
            break;
        case APP_STATUS_INDICATION_FULLCHARGE:
            // cfg0.part[0].level = 1;
            // cfg0.part[0].time = (5000);
            // cfg0.parttotal = 1;
            // cfg0.startlevel = 1;
            // cfg0.periodic = true;
            // app_pwl_setup(APP_PWL_ID_0, &cfg0);
            // app_pwl_start(APP_PWL_ID_0);
            break;
        case APP_STATUS_INDICATION_POWEROFF:
            cfg1.part[0].level = 1;
            cfg1.part[0].time = (1000);
            cfg1.parttotal = 1;
            cfg1.startlevel = 1;
            cfg1.periodic = false;

            app_pwl_setup(APP_PWL_ID_1, &cfg1);
            app_pwl_start(APP_PWL_ID_1);
            break;
        case APP_STATUS_INDICATION_CHARGENEED:
            cfg1.part[0].level = 0;
            cfg1.part[0].time = (500);
            cfg1.part[1].level = 1;
            cfg1.part[1].time = (2000);
            cfg1.parttotal = 2;
            cfg1.startlevel = 1;
            cfg1.periodic = false;
            app_pwl_setup(APP_PWL_ID_1, &cfg1);
            app_pwl_start(APP_PWL_ID_1);
            break;

        case APP_STATUS_INDICATION_INCOMINGCALL:
        case APP_STATUS_INDICATION_CALLING:
            cfg0.part[0].level = 0;
            cfg0.part[0].time = (400);
            cfg0.part[1].level = 1;
            cfg0.part[1].time = (400);
            cfg0.part[2].level = 0;
            cfg0.part[2].time = (400);
            cfg0.part[3].level = 1;
            cfg0.part[3].time = (400);
            cfg0.part[4].level = 1;
            cfg0.part[4].time = (400);
            cfg0.part[5].level = 1;
            cfg0.part[5].time = (400);
            cfg0.part[6].level = 1;
            cfg0.part[6].time = (400);
            cfg0.part[7].level = 1;
            cfg0.part[7].time = (400);
            cfg0.part[8].level = 1;
            cfg0.part[8].time = (400);
            cfg0.part[9].level = 1;
            cfg0.part[9].time = (400);
            cfg0.part[10].level = 1;
            cfg0.part[10].time = (400);
            cfg0.part[11].level = 1;
            cfg0.part[11].time = (400);
            cfg0.part[12].level = 1;
            cfg0.part[12].time = (400);

            cfg0.parttotal = 13;
            cfg0.startlevel = 1;
            cfg0.periodic = true;
            
            app_pwl_setup(APP_PWL_ID_0, &cfg0);
            app_pwl_start(APP_PWL_ID_0);
            break;
        case APP_STATUS_INDICATION_IDLE:
            cfg0.part[0].level = 1;
            cfg0.part[0].time = (1000);
            cfg0.part[1].level = 1;
            cfg0.part[1].time = (1000);
            cfg0.part[2].level = 1;
            cfg0.part[2].time = (1000);
            cfg0.part[3].level = 1;
            cfg0.part[3].time = (1000);
            cfg0.part[4].level = 0;
            cfg0.part[4].time = (1000);

            cfg0.parttotal = 5;
            cfg0.startlevel = 1;
            cfg0.periodic = true;
            
            app_pwl_setup(APP_PWL_ID_0, &cfg0);
            app_pwl_start(APP_PWL_ID_0);
            break;
        case APP_STATUS_INDICATION_PAGESCAN:
            cfg0.part[0].level = 1;
            cfg0.part[0].time = (1000);
            cfg0.part[1].level = 1;
            cfg0.part[1].time = (1000);

            cfg0.parttotal = 5;
            cfg0.startlevel = 1;
            cfg0.periodic = true;
            app_pwl_setup(APP_PWL_ID_0, &cfg0);
            app_pwl_start(APP_PWL_ID_0);

            break;
        default:
            cfg0.part[0].level = 1;
            cfg0.part[0].time = (1000);

            cfg0.parttotal = 1;
            cfg0.startlevel = 1;
            cfg0.periodic = false;

            app_pwl_setup(APP_PWL_ID_0, &cfg0);
            app_pwl_start(APP_PWL_ID_0);
            break;
    }
    return 0;
}

