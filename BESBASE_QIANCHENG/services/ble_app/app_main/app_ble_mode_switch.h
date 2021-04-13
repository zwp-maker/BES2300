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

#ifndef __APP_BLE_MODE_SWITCH_H__
#define __APP_BLE_MODE_SWITCH_H__

#ifdef __cplusplus
extern "C" {
#endif

/*****************************header include********************************/
#include "bluetooth.h"

/******************************macro defination*****************************/
#ifndef BLE_CONNECTION_MAX
#define BLE_CONNECTION_MAX (1)
#endif

// 3 bytes reserved for FLAG field
#define BLE_ADV_DATA_MAX_LEN (28)
#define BLE_SCAN_RSP_DATA_MAX_LEN (31)

// the default interval is 160ms, note that for Bisto user case, to
// let GVA iOS version pop-out notification smoothly, the maximum interval should be this value
#define BLE_ADVERTISING_INTERVAL (160)
#define BLE_FAST_ADVERTISING_INTERVAL (48)

#define BLE_ADV_SVC_FLAG  0x16
#define BLE_ADV_MANU_FLAG 0xFF

#define GVA_AMA_CROSS_ADV_TIME_INTERVAL (1500)

/******************************type defination******************************/
/**
 * @brief The state type of the ble
 *
 */
enum BLE_STATE_E {
    STATE_IDLE       = 0,
    ADVERTISING      = 1,
    STARTING_ADV     = 2,
    STOPPING_ADV     = 3,
    SCANNING         = 4,
    STARTING_SCAN    = 5,
    STOPPING_SCAN    = 6,
    CONNECTING       = 7,
    STARTING_CONNECT = 8,
    STOPPING_CONNECT = 9,
};

/**
 * @brief The operation type of the ble
 *
 */
enum BLE_OP_E {
    OP_IDLE       = 0,
    START_ADV     = 1,
    START_SCAN    = 2,
    START_CONNECT = 3,
    STOP_ADV      = 4,
    STOP_SCAN     = 5,
    STOP_CONNECT  = 6,
};

enum BLE_ADV_USER_E {
    USER_GFPS            = 0,
    USER_GSOUND          = 1,
    USER_AI              = 2,
    USER_INTERCONNECTION = 3,
    USER_TILE            = 4,

    BLE_ADV_USER_NUM,
};

typedef void (*BLE_DATA_FILL_FUNC_T)(void *advParam);

enum BLE_CONNECT_STATE {
    BLE_DISCONNECTED  = 0,
    BLE_DISCONNECTING = 1,
    BLE_CONNECTED     = 2,
};

typedef struct {
    uint8_t advType;
    uint16_t advInterval;
    uint8_t advDataLen;
    uint8_t advData[BLE_ADV_DATA_MAX_LEN];
    uint8_t scanRspDataLen;
    uint8_t scanRspData[BLE_SCAN_RSP_DATA_MAX_LEN];
} BLE_ADV_PARAM_T;

typedef struct {
    uint8_t scanType;
    uint16_t scanWindow;
    uint16_t scanInterval;
} BLE_SCAN_PARAM_T;

typedef struct {
    uint8_t state;
    uint8_t addr[BTIF_BD_ADDR_SIZE];
} BLE_CONNECT_PARAM_T;

typedef struct
{
    uint8_t state;
    uint8_t op;
    bool advSwitch;
    uint8_t connectNum;
    uint8_t bleAddrToConnect[BTIF_BD_ADDR_SIZE];
    BLE_DATA_FILL_FUNC_T bleDataFillFunc[BLE_ADV_USER_NUM];

    // param used for BLE adv
    BLE_ADV_PARAM_T advInfo;

    // prarm used for BLE scan
    BLE_SCAN_PARAM_T scanInfo;

    // param used for BLE connect
    BLE_CONNECT_PARAM_T connectInfo[BLE_CONNECTION_MAX];
} __attribute__((__packed__)) BLE_MODE_ENV_T;

/****************************function declearation**************************/
/*---------------------------------------------------------------------------
 *            app_ble_mode_init
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    init the bleModeEnv
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_ble_mode_init(void);

/*---------------------------------------------------------------------------
 *            app_ble_register_data_fill_handler
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    register a BLE advertisement and scan response data fill handler for a
 *    specific user(see @BLE_ADV_USER_E), so that the adv/scan response data
 *    could present in BLE adv/scan response data
 *
 * Parameters:
 *    user - see the defination in BLE_ADV_USER_E
 *    func - adv/scan response data fill handler for specific user
 *
 * Return:
 *    void
 */
void app_ble_register_data_fill_handle(uint8_t user, BLE_DATA_FILL_FUNC_T func);

bool app_ble_start_adv(uint8_t advType, uint16_t advInterval);

/*---------------------------------------------------------------------------
 *            app_ble_start_connectable_adv
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    start connetable BLE advertise
 *
 * Parameters:
 *    advertisement interval in ms
 *
 * Return:
 *    true - start BLE adv request accepted
 *    false - start BLE adv request denied
 */
bool app_ble_start_connectable_adv(uint16_t advInterval);

void app_ble_force_switch_adv(bool onOff);

void app_ble_start_scan(uint16_t scanWindow, uint16_t scanWnterval);

void app_ble_start_connect(uint8_t *bdAddrToConnect);

void app_ble_stop_activities(void);

bool app_ble_is_in_connected_state(void);

void app_start_connectable_ble_adv(uint16_t advInterval);

bool app_ble_is_in_advertising_state(void);

void app_set_interconnection_discoverable(uint8_t discoverable);

uint8_t app_get_interconnection_discoverable(void);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __APP_BLE_MODE_SWITCH_H__ */
