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
#include "cmsis_os.h"
#include "rwip_config.h"
#include "ke_msg.h"
#include "att.h"
#include "gapm_task.h"
#include "app_voicepath_ble.h"

#ifdef GSOUND_ENABLED
#include "gsound_gatt_server.h"
#include "gsound_custom_ble.h"
#endif

#ifdef __SMART_VOICE__
#include "app_sv.h"
#endif

/************************private macro defination***************************/

/************************private type defination****************************/

/**********************private function declearation************************/

/************************private variable defination************************/

/****************************function defination****************************/
void app_ble_voicepath_init(void)
{
#ifdef GSOUND_ENABLED
    app_gsound_ble_init();
#elif defined(__SMART_VOICE__)
    app_ble_smartvoice_init();
#endif
}

const struct ke_state_handler *app_voicepath_ble_get_msg_handler_table(void)
{
#ifdef GSOUND_ENABLED
    return gsound_ble_get_msg_handler_table();
#elif defined(__SMART_VOICE__)
    return app_sv_get_msg_handler_table();
#endif
}

void app_ble_voicepath_add_svc(void)
{
#ifdef GSOUND_ENABLED
    return app_gsound_ble_add_svc();
#elif defined(__SMART_VOICE__)
    return app_ble_voicepath_add_smartvoice_svc();
#endif
}

const struct prf_task_cbs *voicepath_prf_itf_get(void)
{
#ifdef GSOUND_ENABLED
    return gsound_prf_itf_get();
#elif defined(__SMART_VOICE__)
    return sv_prf_itf_get();
#endif
}

void app_ble_bms_add_svc(void)
{

    // Register the BMS service
    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                             TASK_GAPM,
                                                             TASK_APP,
                                                             gapm_profile_task_add_cmd,
                                                             0);

    req->operation = GAPM_PROFILE_TASK_ADD;
#if BLE_CONNECTION_MAX > 1
    req->sec_lvl = PERM(SVC_AUTH, SEC_CON) | PERM(SVC_MI, ENABLE);
#else
    req->sec_lvl = PERM(SVC_AUTH, SEC_CON);
#endif
    req->prf_task_id = TASK_ID_BMS;
    req->app_task    = TASK_APP;
    req->start_hdl   = 0;

    ke_msg_send(req);
}

#if (ANCS_PROXY_ENABLE)
void app_ble_ancsp_add_svc(void)
{
    TRACE("Registering ANCS Proxy GATT Service");
    struct gapm_profile_task_add_cmd *req =
        KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                         TASK_GAPM,
                         TASK_APP,
                         gapm_profile_task_add_cmd,
                         0);

    req->operation = GAPM_PROFILE_TASK_ADD;
#if BLE_CONNECTION_MAX > 1
    req->sec_lvl = PERM(SVC_AUTH, SEC_CON) | PERM(SVC_MI, ENABLE);
#else
    req->sec_lvl = PERM(SVC_AUTH, SEC_CON);
#endif
    req->prf_task_id = TASK_ID_ANCSP;
    req->app_task    = TASK_APP;
    req->start_hdl   = 0;

    ke_msg_send(req);
}

void app_ble_amsp_add_svc(void)
{
    TRACE("Registering AMS Proxy GATT Service");
    struct gapm_profile_task_add_cmd *req =
        KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                         TASK_GAPM,
                         TASK_APP,
                         gapm_profile_task_add_cmd,
                         0);

    req->operation = GAPM_PROFILE_TASK_ADD;
#if BLE_CONNECTION_MAX > 1
    req->sec_lvl = PERM(SVC_AUTH, ENABLE) | PERM(SVC_MI, ENABLE);
#else
    req->sec_lvl = PERM(SVC_AUTH, ENABLE);
#endif
    req->prf_task_id = TASK_ID_AMSP;
    req->app_task    = TASK_APP;
    req->start_hdl   = 0;

    ke_msg_send(req);
}
#endif

void app_voicepath_mtu_exchanged_handler(uint8_t conidx, uint16_t MTU)
{
#ifdef GSOUND_ENABLED
    app_gsound_ble_mtu_exchanged_handler(conidx, MTU);
#elif defined(__SMART_VOICE__)
    if (app_sv_get_connectoin_index() == conidx)
    {
        app_smartvoice_config_mtu(MTU);
    }
#endif
}

void app_voicepath_disconnected_evt_handler(uint8_t conidx)
{
#ifdef GSOUND_ENABLED
    if (app_gsound_ble_get_connection_index() == conidx)
    {
        app_gsound_ble_disconnected_evt_handler(conidx);
    }
#elif defined(__SMART_VOICE__)
    if (app_sv_get_connectoin_index() == conidx)
    {
        app_voicepath_disconnected_evt_handler()
    }
#endif
}

void app_voicepath_ble_conn_parameter_updated(uint8_t conidx,
                                              uint16_t minConnIntervalInMs,
                                              uint16_t maxConnIntervalInMs)
{
#ifdef GSOUND_ENABLED
    gsound_ble_conn_parameter_updated(conidx, minConnIntervalInMs, maxConnIntervalInMs);
#elif defined(__SMART_VOICE__)
    app_voicepath_ble_conn_parameter_updated(conidx, minConnIntervalInMs, maxConnIntervalInMs);
#endif
}