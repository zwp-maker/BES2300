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

#ifndef __APP_TWS_IF_H__
#define __APP_TWS_IF_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EAR_SIDE_UNKNOWN = 0,
    EAR_SIDE_LEFT    = 1,
    EAR_SIDE_RIGHT   = 2,
    EAR_SIDE_NUM,
} APP_TWS_SIDE_T;

/*****************************header include********************************/

/******************************macro defination*****************************/

/******************************type defination******************************/
typedef bool (*ROLE_SWITCH_FUNC_T)(void);

/****************************function declearation**************************/
/*---------------------------------------------------------------------------
 *            app_tws_if_role_switch_started_handler
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    handler for role switch started event of tws system
 *
 * Parameters:
 *    void
 *
 * Return:
 *    voide
 */
void app_tws_if_role_switch_started_handler(void);

/*---------------------------------------------------------------------------
 *            app_tws_if_tws_role_switch_complete_handler
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    callback function of role switch complete event for tws system
 *    NOTE: tws system include relay_tws and IBRT
 *
 * Parameters:
 *    newRole - current role of device after role switch complete
 *
 * Return:
 *    void
 */
void app_tws_if_tws_role_switch_complete_handler(uint8_t newRole);

/*---------------------------------------------------------------------------
 *            app_tws_if_tws_disconnected_handler
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    handler of tws disconnected event
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_tws_if_tws_disconnected_handler(void);

#ifdef __cplusplus
}
#endif

void app_tws_set_side(APP_TWS_SIDE_T side);
bool app_tws_is_left_side(void);
bool app_tws_is_right_side(void);
bool app_tws_is_unknown_side(void);
void app_tws_set_side_from_addr(uint8_t *addr);

#endif /* #ifndef __APP_TWS_IF_H__ */
