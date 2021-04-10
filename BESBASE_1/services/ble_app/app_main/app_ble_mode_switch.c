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
#include "string.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "stdbool.h"
#include "app_thread.h"
#include "app_utils.h"
#include "apps.h"
#include "app.h"
#include "app_ble_mode_switch.h"
#include "nvrecord.h"
#include "app_bt_func.h"
#include "hal_timer.h"
#include "app_bt.h"
#include "app_hfp.h"
#include "rwprf_config.h"

#ifdef __AI_VOICE__
#include "ai_manager.h"
#endif

#ifdef IBRT
#include "app_ibrt_ui.h"
#endif

#ifdef GSOUND_ENABLED
// #include "gsound_target_ble_utils.h"
#endif

/************************private macro defination***************************/
#define DEBUG_BLE_STATE_MACHINE true

#if DEBUG_BLE_STATE_MACHINE
#define SET_BLE_STATE(newState)                                                                                     \
    do                                                                                                              \
    {                                                                                                               \
        TRACE("[BLE][STATE]%s->%s at line %d", ble_state2str(bleModeEnv.state), ble_state2str(newState), __LINE__); \
        bleModeEnv.state = (newState);                                                                              \
    } while (0);

#define SET_BLE_OP(newOp)                                                                            \
    do                                                                                               \
    {                                                                                                \
        TRACE("[BLE][OP]%s->%s at line %d", ble_op2str(bleModeEnv.op), ble_op2str(newOp), __LINE__); \
        bleModeEnv.op = (newOp);                                                                     \
    } while (0);
#else
#define SET_BLE_STATE(newState)        \
    do                                 \
    {                                  \
        bleModeEnv.state = (newState); \
    } while (0);

#define SET_BLE_OP(newOp)        \
    do                           \
    {                            \
        bleModeEnv.op = (newOp); \
    } while (0);
#endif

#ifdef CHIP_BEST2000
extern void bt_drv_reg_set_rssi_seed(uint32_t seed);
#endif

/************************private type defination****************************/

/************************extern function declearation***********************/
extern uint8_t bt_addr[6];

/**********************private function declearation************************/
/*---------------------------------------------------------------------------
 *            ble_state2str
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    get the string of the ble state
 *
 * Parameters:
 *    state - state to get string
 *
 * Return:
 *    the string of the BLE state
 */
static char *ble_state2str(uint8_t state);

/*---------------------------------------------------------------------------
 *            ble_op2str
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    get the string of the ble operation
 *
 * Parameters:
 *    op - operation to get string
 *
 * Return:
 *    the string of the BLE operation
 */
static char *ble_op2str(uint8_t op);

/*---------------------------------------------------------------------------
 *            ble_adv_config_param
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    configure BLE adv related parameter in @advParam for further use
 *
 * Parameters:
 *    advType - advertisment mode, see @ for more info
 *    advInterval - advertisement interval in MS
 *
 * Return:
 *    void
 */
static void ble_adv_config_param(uint8_t advType, uint16_t advInterval);

/*---------------------------------------------------------------------------
 *            ble_adv_is_allowed
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    check if BLE advertisment is allowed or not
 *
 * Parameters:
 *    void
 *
 * Return:
 *    true - if advertisement is allowed
 *    flase -  if adverstisement is not allowed
 */
static bool ble_adv_is_allowed(void);

/*---------------------------------------------------------------------------
 *            ble_start_adv
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    start BLE advertisement
 *
 * Parameters:
 *    param - see @BLE_ADV_PARAM_T to get more info
 *
 * Return:
 *    void
 */
static void ble_start_adv(void *param);

/*---------------------------------------------------------------------------
 *            ble_start_adv_failed_cb
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    callback function of start BLE advertisement failed
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
static void ble_start_adv_failed_cb(void);

/*---------------------------------------------------------------------------
 *            ble_start_scan
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    start BLE scan api
 *
 * Parameters:
 *    param - see @BLE_SCAN_PARAM_T to get more info
 *
 * Return:
 *    void
 */
static void ble_start_scan(void *param);

/*---------------------------------------------------------------------------
 *            ble_start_connect
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    start BLE connection
 *
 * Parameters:
 *    bleBdAddr - address of BLE device to connect
 *
 * Return:
 *    void
 */
static void ble_start_connect(uint8_t *bleBdAddr);

/*---------------------------------------------------------------------------
 *            bld_disconnect
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    disconnect a BLE connection
 *
 * Parameters:
 *    idx - index of the connection
 *
 * Return:
 *    void
 */
static void ble_disconnect(uint8_t idx);

/*---------------------------------------------------------------------------
 *            ble_stop_all_activities
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    stop all BLE pending operations, stop ongoing adv and scan
 *    NOTE: will not disconnect BLE connections
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
static void ble_stop_all_activities(void);

/*---------------------------------------------------------------------------
 *            ble_execute_pending_op
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    execute pended BLE op
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
static void ble_execute_pending_op(void);

/*---------------------------------------------------------------------------
 *            ble_switch_activities
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    switch BLE activities after last state complete
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
static void ble_switch_activities(void);

/************************private variable defination************************/
static BLE_MODE_ENV_T bleModeEnv;
static BLE_ADV_PARAM_T advParam;
static BLE_SCAN_PARAM_T scanParam;

/****************************function defination****************************/
// common used function
static char *ble_state2str(uint8_t state)
{
    char *str = NULL;

#define CASES(state)          \
    case state:               \
        str = "[" #state "]"; \
        break

    switch (state)
    {
        CASES(STATE_IDLE);
        CASES(ADVERTISING);
        CASES(STARTING_ADV);
        CASES(STOPPING_ADV);
        CASES(SCANNING);
        CASES(STARTING_SCAN);
        CASES(STOPPING_SCAN);
        CASES(CONNECTING);
        CASES(STARTING_CONNECT);
        CASES(STOPPING_CONNECT);

    default:
        str = "[INVALID]";
        break;
    }

    return str;
}

static char *ble_op2str(uint8_t op)
{
    char *str = NULL;

#define CASEO(op)          \
    case op:               \
        str = "[" #op "]"; \
        break

    switch (op)
    {
        CASEO(OP_IDLE);
        CASEO(START_ADV);
        CASEO(START_SCAN);
        CASEO(START_CONNECT);
        CASEO(STOP_ADV);
        CASEO(STOP_SCAN);
        CASEO(STOP_CONNECT);

    default:
        str = "[INVALID]";
        break;
    }

    return str;
}

void app_ble_mode_init(void)
{
    memset(&bleModeEnv, 0, sizeof(bleModeEnv));
    bleModeEnv.advSwitch = true;
    SET_BLE_STATE(STATE_IDLE);
    SET_BLE_OP(OP_IDLE);
}

// ble advertisement used functions
static void ble_adv_config_param(uint8_t advType, uint16_t advInterval)
{
    memset(&advParam, 0, sizeof(advParam));

    advParam.advType     = advType;
    advParam.advInterval = advInterval;

    // connectable adv is not allowed if max connection reaches
    if (app_is_arrive_at_max_ble_connections() && (GAPM_ADV_UNDIRECT == advType))
    {
        advParam.advType = GAPM_ADV_NON_CONN;
    }

    for (uint8_t user = 0; user < BLE_ADV_USER_NUM; user++)
    {
        if (bleModeEnv.bleDataFillFunc[user])
        {
            bleModeEnv.bleDataFillFunc[user](( void * )&advParam);
            ASSERT(BLE_ADV_DATA_MAX_LEN >= advParam.advDataLen, "[BLE][ADV]adv data exceed");
            ASSERT(BLE_SCAN_RSP_DATA_MAX_LEN >= advParam.scanRspDataLen, "[BLE][ADV]scan response data exceed");
        }
    }

    ASSERT(advParam.advDataLen <= BLE_ADV_DATA_MAX_LEN, "adv data exceed");
    ASSERT(advParam.scanRspDataLen <= BLE_SCAN_RSP_DATA_MAX_LEN, "scanrsp data exceed");
}

static bool ble_adv_is_allowed(void)
{
    if (!app_is_stack_ready())
    {
        TRACE("reason: stack not ready");
        return false;
    }

    if (app_is_power_off_in_progress())
    {
        TRACE("reason: in power off mode");
        return false;
    }

    if (!bleModeEnv.advSwitch)
    {
        TRACE("adv switched off");
        return false;
    }

    if (btapp_hfp_is_sco_active())
    {
        TRACE("SCO ongoing");
        return false;
    }

    return true;
}

static void ble_start_adv(void *param)
{
    switch (bleModeEnv.state)
    {
    case ADVERTISING:
        SET_BLE_STATE(STOPPING_ADV);
        SET_BLE_OP(START_ADV);
        appm_stop_advertising();
        break;

    case SCANNING:
        SET_BLE_STATE(STOPPING_SCAN);
        SET_BLE_OP(START_ADV);
        appm_stop_scanning();
        break;

    case CONNECTING:
        SET_BLE_STATE(STOPPING_CONNECT);
        SET_BLE_OP(START_ADV);
        appm_stop_connecting();
        break;

    case STARTING_ADV:
    case STARTING_SCAN:
    case STARTING_CONNECT:
    case STOPPING_ADV:
    case STOPPING_SCAN:
    case STOPPING_CONNECT:
        SET_BLE_OP(START_ADV);
        break;

    case STATE_IDLE:
        if (!ble_adv_is_allowed())
        {
            TRACE("[BLE][ADV] not allowed.");
            if (START_ADV == bleModeEnv.op)
            {
                SET_BLE_OP(OP_IDLE);
            }
            break;
        }

        memcpy(&bleModeEnv.advInfo, param, sizeof(bleModeEnv.advInfo));
        appm_start_advertising(&bleModeEnv.advInfo);

        SET_BLE_STATE(STARTING_ADV);
        break;

    default:
        break;
    }
}

static void ble_start_adv_failed_cb(void)
{
    if (STARTING_ADV == bleModeEnv.state)
    {
        SET_BLE_STATE(STATE_IDLE);
    }

    // start pending op(start adv again)
}

static void ble_start_scan(void *param)
{
    switch (bleModeEnv.state)
    {
    case ADVERTISING:
        SET_BLE_STATE(STOPPING_ADV);
        SET_BLE_OP(START_SCAN);
        appm_stop_advertising();
        break;

    case SCANNING:
        SET_BLE_STATE(STOPPING_SCAN);
        SET_BLE_OP(START_SCAN);
        appm_stop_scanning();
        break;

    case CONNECTING:
        SET_BLE_STATE(STOPPING_CONNECT);
        SET_BLE_OP(START_SCAN);
        appm_stop_connecting();
        break;

    case STARTING_ADV:
    case STARTING_SCAN:
    case STARTING_CONNECT:
    case STOPPING_ADV:
    case STOPPING_SCAN:
    case STOPPING_CONNECT:
        SET_BLE_OP(START_SCAN);
        break;

    case STATE_IDLE:
        memcpy(&bleModeEnv.scanInfo, param, sizeof(BLE_SCAN_PARAM_T));
        appm_start_scanning(bleModeEnv.scanInfo.scanInterval,
                            bleModeEnv.scanInfo.scanWindow,
                            bleModeEnv.scanInfo.scanType);
        SET_BLE_STATE(STARTING_SCAN);
        break;

    default:
        break;
    }
}

static void ble_start_connect(uint8_t *bleBdAddr)
{
    SET_BLE_OP(START_CONNECT);

    if ((CONNECTING != bleModeEnv.state) &&
        (STARTING_CONNECT != bleModeEnv.state))
    {
        switch (bleModeEnv.state)
        {
        case ADVERTISING:
            SET_BLE_STATE(STOPPING_ADV);
            appm_stop_advertising();
            break;

        case SCANNING:
            SET_BLE_STATE(STOPPING_SCAN);
            appm_stop_scanning();
            break;

        case STATE_IDLE:
            SET_BLE_STATE(START_CONNECT);
            struct gap_bdaddr bdAddr;
            memcpy(bdAddr.addr.addr, bleBdAddr, BTIF_BD_ADDR_SIZE);
            bdAddr.addr_type = 0;
            TRACE("Master paired with mobile dev is scanned, connect it via BLE.");
            appm_start_connecting(&bdAddr);
            break;

        default:
            break;
        }
    }
}

static void ble_disconnect(uint8_t idx)
{
    if (BLE_CONNECTED != bleModeEnv.connectInfo[idx].state)
    {
        TRACE("connection state of %d is not connected", idx);
        return;
    }

    bleModeEnv.connectInfo[idx].state = BLE_DISCONNECTING;
    appm_disconnect(idx);
}

static void ble_stop_all_activities(void)
{
    switch (bleModeEnv.state)
    {
    case ADVERTISING:
        SET_BLE_OP(OP_IDLE);
        SET_BLE_STATE(STOPPING_ADV);
        appm_stop_advertising();
        break;

    case SCANNING:
        SET_BLE_OP(OP_IDLE);
        SET_BLE_STATE(STOPPING_SCAN);
        appm_stop_scanning();
        break;

    case CONNECTING:
        SET_BLE_OP(OP_IDLE);
        SET_BLE_STATE(STOPPING_CONNECT);
        appm_stop_connecting();
        break;

    case STARTING_ADV:
        SET_BLE_OP(STOP_ADV);
        break;

    case STARTING_SCAN:
        SET_BLE_OP(STOP_SCAN);
        break;

    case STARTING_CONNECT:
        SET_BLE_OP(STOP_CONNECT);
        break;

    case STOPPING_ADV:
    case STOPPING_SCAN:
    case STOPPING_CONNECT:
        SET_BLE_OP(OP_IDLE);
        break;

    default:
        break;
    }
}

static void ble_execute_pending_op(void)
{
    uint8_t op = bleModeEnv.op;

    switch (op)
    {
    case START_ADV:
        ble_start_adv(&advParam);
        break;

    case START_SCAN:
        ble_start_scan(&scanParam);
        break;

    case START_CONNECT:
        ble_start_connect(bleModeEnv.bleAddrToConnect);
        break;

    case STOP_ADV:
    case STOP_SCAN:
    case STOP_CONNECT:
        ble_stop_all_activities();
        break;

    default:
        break;
    }
}

static void ble_switch_activities(void)
{
    switch (bleModeEnv.state)
    {
    case STARTING_ADV:
        if (START_ADV == bleModeEnv.op)
        {
            SET_BLE_OP(OP_IDLE);
        }
        SET_BLE_STATE(ADVERTISING);
        break;

    case STARTING_SCAN:
        if (START_SCAN == bleModeEnv.op)
        {
            SET_BLE_OP(OP_IDLE);
        }
        SET_BLE_STATE(SCANNING);
        break;

    case STARTING_CONNECT:
        if (START_CONNECT == bleModeEnv.op)
        {
            SET_BLE_OP(OP_IDLE);
        }
        SET_BLE_STATE(CONNECTING);
        break;

    case STOPPING_ADV:
        if (STOP_ADV == bleModeEnv.op)
        {
            SET_BLE_OP(OP_IDLE);
        }
        SET_BLE_STATE(STATE_IDLE);
        break;

    case STOPPING_SCAN:
        if (STOP_SCAN == bleModeEnv.op)
        {
            SET_BLE_OP(OP_IDLE);
        }
        SET_BLE_STATE(STATE_IDLE);
        break;

    case STOPPING_CONNECT:
        if (STOP_CONNECT == bleModeEnv.op)
        {
            SET_BLE_OP(OP_IDLE);
        }
        SET_BLE_STATE(STATE_IDLE);
        break;

    default:
        break;
    }

    ble_execute_pending_op();
}

// BLE advertisement event callbacks
void app_advertising_started(void)
{
    app_bt_start_custom_function_in_bt_thread(0,
                                              0,
                                              ( uint32_t )ble_switch_activities);
}

void app_advertising_starting_failed(void)
{
    app_bt_start_custom_function_in_bt_thread(0,
                                              0,
                                              ( uint32_t )ble_start_adv_failed_cb);
}

void app_advertising_stopped(void)
{
    app_bt_start_custom_function_in_bt_thread(0,
                                              0,
                                              ( uint32_t )ble_switch_activities);
}

// BLE scan event callbacks
void app_scanning_started(void)
{
    app_bt_start_custom_function_in_bt_thread(0,
                                              0,
                                              ( uint32_t )ble_switch_activities);
}

void app_scanning_stopped(void)
{
    app_bt_start_custom_function_in_bt_thread(0,
                                              0,
                                              ( uint32_t )ble_switch_activities);
}

// BLE connect event callbacks
void app_connecting_started(void)
{
    app_bt_start_custom_function_in_bt_thread(0,
                                              0,
                                              ( uint32_t )ble_switch_activities);
}

void app_connecting_stopped(void)
{
    app_bt_start_custom_function_in_bt_thread(0,
                                              0,
                                              ( uint32_t )ble_switch_activities);
}

void app_ble_connected_evt_handler(uint8_t conidx, const uint8_t *pPeerBdAddress)
{
    if ((ADVERTISING == bleModeEnv.state) ||
        (CONNECTING == bleModeEnv.state))
    {
        SET_BLE_STATE(STATE_IDLE);
    }

    if (START_CONNECT == bleModeEnv.op)
    {
        SET_BLE_OP(OP_IDLE);
    }

    bleModeEnv.connectNum++;
    bleModeEnv.connectInfo[conidx].state = BLE_CONNECTED;
    memcpy(bleModeEnv.connectInfo[conidx].addr, pPeerBdAddress, BTIF_BD_ADDR_SIZE);

    app_stop_fast_connectable_ble_adv_timer();
    app_ble_start_adv(GAPM_ADV_UNDIRECT, BLE_ADVERTISING_INTERVAL);
}

void app_ble_disconnected_evt_handler(uint8_t conidx)
{
    uint8_t ret = 0;

    bleModeEnv.connectNum--;
    bleModeEnv.connectInfo[conidx].state = BLE_DISCONNECTED;
    memset(bleModeEnv.connectInfo[conidx].addr, 0, BTIF_BD_ADDR_SIZE);

#ifdef IBRT
    if (app_ibrt_ui_get_snoop_via_ble_enable())
    {
        app_ibrt_ui_set_master_notify_flag(false);
        ret = app_ibrt_ui_clear_box_connect_state(IBRT_BOX_CONNECT_MASTER, FALSE);
    }
#endif

    if (0 == ret)
    {
        app_ble_start_adv(GAPM_ADV_UNDIRECT, BLE_ADVERTISING_INTERVAL);
    }
}

// BLE APIs for external use
void app_ble_register_data_fill_handle(uint8_t user, BLE_DATA_FILL_FUNC_T func)
{
    bool needUpdateAdv = false;

    if (BLE_ADV_USER_NUM <= user)
    {
        TRACE("invalid user");
    }
    else
    {
        if (func != bleModeEnv.bleDataFillFunc[user] &&
            NULL != func)
        {
            needUpdateAdv                    = true;
            bleModeEnv.bleDataFillFunc[user] = func;
        }
    }

    if (needUpdateAdv)
    {
        app_ble_start_adv(GAPM_ADV_UNDIRECT, BLE_ADVERTISING_INTERVAL);
    }
}

void app_ble_system_ready(void)
{
#ifdef CHIP_BEST2000
    uint32_t generatedSeed = hal_sys_timer_get();

    for (uint8_t index = 0; index < sizeof(bt_addr); index++)
    {
        generatedSeed ^= (((uint32_t)(bt_addr[index])) << (hal_sys_timer_get() & 0xF));
    }

    bt_drv_reg_set_rssi_seed(generatedSeed);
#endif

#if defined(ENHANCED_STACK)
    app_notify_stack_ready(STACK_READY_BLE);
#else
    app_notify_stack_ready(STACK_READY_BLE | STACK_READY_BT);
#endif
}

bool app_ble_start_adv(uint8_t advType, uint16_t advInterval)
{
    TRACE("[BLE][ADV]type:%d, interval:%d", advType, advInterval);

    if (!ble_adv_is_allowed())
    {
        TRACE("[BLE][ADV] not allowed.");
        return false;
    }

    ble_adv_config_param(advType, advInterval);

    // param of adv request is exactly same as current adv
    if (ADVERTISING == bleModeEnv.state &&
        bleModeEnv.advInfo.advType == advParam.advType &&
        bleModeEnv.advInfo.advInterval == advParam.advInterval &&
        bleModeEnv.advInfo.advDataLen == advParam.advDataLen &&
        !memcmp(bleModeEnv.advInfo.advData, advParam.advData, advParam.advDataLen) &&
        bleModeEnv.advInfo.scanRspDataLen == advParam.advDataLen &&
        !memcmp(bleModeEnv.advInfo.scanRspData, advParam.scanRspData, advParam.scanRspDataLen))
    {
        TRACE("reason: adv param not changed");
        TRACE("[BLE][ADV] not allowed.");
        return false;
    }

    app_bt_start_custom_function_in_bt_thread(( uint32_t )&advParam,
                                              0,
                                              ( uint32_t )ble_start_adv);

    return true;
}

bool app_ble_start_connectable_adv(uint16_t advInterval)
{
    return app_ble_start_adv(GAPM_ADV_UNDIRECT, advInterval);
}

void app_ble_start_scan(uint16_t scanWindow, uint16_t scanInterval)
{
    scanParam.scanWindow   = scanWindow;
    scanParam.scanInterval = scanInterval;

#ifdef IBRT
    if (app_ibrt_ui_get_snoop_via_ble_enable())
    {
        scanParam.scanType = SCAN_ALLOW_ADV_ALL;
    }
    else
    {
        scanParam.scanType = SCAN_ALLOW_ADV_WLST;
    }
#else
    scanParam.scanType = SCAN_ALLOW_ADV_WLST;
#endif

    app_bt_start_custom_function_in_bt_thread((uint32_t)(&scanParam),
                                              0,
                                              ( uint32_t )ble_start_scan);
}

void app_ble_start_connect(uint8_t *bdAddrToConnect)
{
    memcpy(bleModeEnv.bleAddrToConnect, bdAddrToConnect, BTIF_BD_ADDR_SIZE);
    app_bt_start_custom_function_in_bt_thread(( uint32_t )bleModeEnv.bleAddrToConnect,
                                              0,
                                              ( uint32_t )ble_start_connect);
}

void app_ble_stop_activities(void)
{
    TRACE("%s", __func__);
    app_stop_fast_connectable_ble_adv_timer();
    app_bt_start_custom_function_in_bt_thread(0, 0, ( uint32_t )ble_stop_all_activities);
}

void app_ble_start_disconnect(uint8_t idx)
{
    if (BLE_CONNECTED != bleModeEnv.connectInfo[idx].state)
    {
        TRACE("connection state of %d is not connected", idx);
        return;
    }

    app_bt_start_custom_function_in_bt_thread(idx,
                                              0,
                                              ( uint32_t )ble_disconnect);
}

void app_start_connectable_ble_adv(uint16_t advInterval)
{
    app_ble_start_adv(GAPM_ADV_UNDIRECT, advInterval);
}

void app_ble_force_switch_adv(bool onOff)
{
    bleModeEnv.advSwitch = onOff;
}

bool app_ble_is_in_connected_state(void)
{
    // TODO: freddie add check
    return false;
}

bool app_ble_is_in_advertising_state(void)
{
    return (ADVERTISING == bleModeEnv.state) ||
           (STARTING_ADV == bleModeEnv.state) ||
           (STOPPING_ADV == bleModeEnv.state);
}

static uint32_t POSSIBLY_UNUSED ble_get_manufacture_data_ptr(uint8_t *advData,
                                                             uint32_t dataLength,
                                                             uint8_t *manufactureData)
{
    uint8_t followingDataLengthOfSection;
    uint8_t rawContentDataLengthOfSection;
    uint8_t flag;
    while (dataLength > 0)
    {
        followingDataLengthOfSection = *advData++;
        dataLength--;
        if (dataLength < followingDataLengthOfSection)
        {
            return 0;  // wrong adv data format
        }

        if (followingDataLengthOfSection > 0)
        {
            flag = *advData++;
            dataLength--;

            rawContentDataLengthOfSection = followingDataLengthOfSection - 1;
            if (BLE_ADV_MANU_FLAG == flag)
            {
                uint32_t lengthToCopy;
                if (dataLength < rawContentDataLengthOfSection)
                {
                    lengthToCopy = dataLength;
                }
                else
                {
                    lengthToCopy = rawContentDataLengthOfSection;
                }

                memcpy(manufactureData, advData - 2, lengthToCopy + 2);
                return lengthToCopy + 2;
            }
            else
            {
                advData += rawContentDataLengthOfSection;
                dataLength -= rawContentDataLengthOfSection;
            }
        }
    }

    return 0;
}

static void ble_adv_data_parse(uint8_t *bleBdAddr,
                               int8_t rssi,
                               unsigned char *adv_buf,
                               unsigned char len)
{
#ifdef IBRT
    bd_addr_t *box_ble_addr = ( bd_addr_t * )app_ibrt_ui_get_box_ble_addr();
    TRACE("%s", __func__);
    //DUMP8("%02x ", (uint8_t *)box_ble_addr, BTIF_BD_ADDR_SIZE);
    DUMP8("%02x ", bleBdAddr, BTIF_BD_ADDR_SIZE);

    if (app_ibrt_ui_get_snoop_via_ble_enable())
    {
        if (!memcmp(box_ble_addr, bleBdAddr, BTIF_BD_ADDR_SIZE) && app_ibrt_ui_is_slave_scaning())
        {
            app_ibrt_ui_set_slave_scaning(FALSE);
            app_scanning_stopped();
            app_ble_start_connect(( uint8_t * )box_ble_addr);
        }
    }
#endif
}

//received adv data
void app_adv_reported_scanned(struct gapm_adv_report_ind *ptInd)
{
    /*
    TRACE("Scanned RSSI %d BD addr:", (int8_t)ptInd->report.rssi);
    DUMP8("0x%02x ", ptInd->report.adv_addr.addr, BTIF_BD_ADDR_SIZE);
    TRACE("Scanned adv data:");
    DUMP8("0x%02x ", ptInd->report.data, ptInd->report.data_len);
    */

    ble_adv_data_parse(ptInd->report.adv_addr.addr,
                       ( int8_t )ptInd->report.rssi,
                       ptInd->report.data,
                       ( unsigned char )ptInd->report.data_len);
}

#ifdef __INTERCONNECTION__
/**
 * @brief : set is_interconnection_discoverable_adv flag
 * 
 * @param discoverable 
 */
void app_set_interconnection_discoverable(uint8_t discoverable)
{
    bleModeEnv.interconnectionDiscoverable = discoverable;
}

/**
 * @brief : get discoverable state of interconnection
 * 
 * @return uint8_t 
 */
uint8_t app_get_interconnection_discoverable(void)
{
    return bleModeEnv.interconnectionDiscoverable;
}
#endif

