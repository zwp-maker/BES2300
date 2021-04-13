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
#ifndef __SYS_BT_CFG_H__
#define __SYS_BT_CFG_H__

#define BTIF_DISABLED  0
#define BTIF_ENABLED 1

#define BTIF_AVRCP_ADVANCED_CONTROLLER

#define BTIF_AV_WORKER  BTIF_ENABLED


// #if defined(A2DP_LHDC_ON)
//          #define BTIF_A2DP_LHDC_ON 1
// #else
//          #define BTIF_A2DP_LHDC_ON 0
// #endif

// #if defined(MASTER_USE_OPUS) || defined(ALL_USE_OPUS)
//          #define BTIF_A2DP_OPUS_ON 1
// #else
//          #define BTIF_A2DP_OPUS_ON 0
// #endif

// #if defined(A2DP_AAC_ON)
//          #define BTIF_A2DP_AAC_ON  1
// #else
//          #define BTIF_A2DP_AAC_ON  0
// #endif

// #if defined(A2DP_LDAC_ON)
//          #define BTIF_A2DP_LDAC_ON 1
// #else
//          #define BTIF_A2DP_LDAC_ON 0
// #endif

// #define BTIF_A2DP_SBC_ON  1


// #if defined(A2DP_SCALABLE_ON)
//          #define BTIF_A2DP_SCALABLE_ON  1
// #else
//          #define BTIF_A2DP_SCALABLE_ON  0
// #endif

// #if defined(__BT_ONE_BRING_TWO__)
//          #define BTIF_STREAM_SETS 2
// #else
//          #define BTIF_STREAM_SETS 1
// #endif

//#define SYS_MAX_A2DP_STREAMS   (BTIF_STREAM_SETS*(BTIF_A2DP_AAC_ON + BTIF_A2DP_OPUS_ON + BTIF_A2DP_LHDC_ON  + BTIF_A2DP_SCALABLE_ON + BTIF_A2DP_SBC_ON + BTIF_A2DP_LDAC_ON +1))
#define SYS_MAX_A2DP_STREAMS 14

#define BTIF_SBC_ENCODER   BTIF_ENABLED
#define BTIF_SBC_DECODER   BTIF_ENABLED

#define SYS_MAX_AVRCP_CHNS  2

#define BTIF_AVRCP_NUM_PLAYER_SETTINGS 4

#define BTIF_AVRCP_NUM_MEDIA_ATTRIBUTES        7

#define  BTIF_AVRCP_VERSION_1_3_ONLY   BTIF_DISABLED

#define  BTIF_L2CAP_PRIORITY BTIF_DISABLED

#define  BTIF_XA_STATISTICS

#define  BTIF_L2CAP_NUM_ENHANCED_CHANNELS 0

#define  BTIF_BT_BEST_SYNC_CONFIG   BTIF_ENABLED

#define  BTIF_HCI_HOST_FLOW_CONTROL BTIF_ENABLED

#define  BTIF_DEFAULT_ACCESS_MODE_PAIR   BTIF_BAM_GENERAL_ACCESSIBLE

#define BTIF_BT_DEFAULT_PAGE_SCAN_TYPE      1


#define BTIF_BT_DEFAULT_PAGE_SCAN_WINDOW 0x12

/*---------------------------------------------------------------------------
 * BT_DEFAULT_PAGE_SCAN_INTERVAL constant
 *
 *     See BT_DEFAULT_PAGE_SCAN_WINDOW.
 */
#define BTIF_BT_DEFAULT_PAGE_SCAN_INTERVAL 0x800

/*---------------------------------------------------------------------------
 * BT_DEFAULT_INQ_SCAN_WINDOW constant
 *
 *     See BT_DEFAULT_PAGE_SCAN_WINDOW.
 */
#define BTIF_BT_DEFAULT_INQ_SCAN_WINDOW 0x12

/*---------------------------------------------------------------------------
 * BT_DEFAULT_INQ_SCAN_INTERVAL constant
 *
 *     See BT_DEFAULT_PAGE_SCAN_WINDOW.
 */
#define BTIF_BT_DEFAULT_INQ_SCAN_INTERVAL 0x800

#define BTIF_BT_DEFAULT_PAGE_TIMEOUT_IN_MS				5000

#define BTIF_SPP_CLIENT BTIF_DISABLED

#define BTIF_SPP_SERVER BTIF_DISABLED

#define BTIF_RF_SEND_CONTROL  BTIF_DISABLED

#define BTIF_MULTITASKING

#define BTIF_SECURITY

#ifdef __INTERCONNECTION__
#else
#define BTIF_BLE_APP_DATAPATH_SERVER
#endif

#ifdef __AI_VOICE__
#define BTIF_DIP_DEVICE
#endif

#ifdef __KNOWLES
#define DIG_MIC_WORKAROUND
#define KNOWLES_UART_DATA
#endif

#ifdef __AMA_VOICE__
//#define KEYWORD_WAKEUP_ENABLED
//#define PUSH_AND_HOLD_ENABLED
//#define AI_32KBPS_VOICE
//#define NO_LOCAL_START_TONE
#endif

#ifdef __DMA_VOICE__
//#define KEYWORD_WAKEUP_ENABLED
//#define PUSH_AND_HOLD_ENABLED
//#define AI_32KBPS_VOICE

#define __BES__
#define BAIDU_DATA_SN_LEN   16
#define FLOW_CONTROL_ON_UPLEVEL
#define ASSAM_PKT_ON_UPLEVEL
#define BAIDU_DATA_RAND_LEN 8
#define CLOSE_BLE_ADV_WHEN_VOICE_CALL
#define CLOSE_BLE_ADV_WHEN_SPP_CONNECTED
#define BAIDU_RFCOMM_DIRECT_CONN
#define BYPASS_SLOW_ADV_MODE

#define NVREC_BAIDU_DATA_SECTION
#define FM_MIN_FREQ             875
#define FM_MAX_FREQ             1079
#define BAIDU_DATA_DEF_FM_FREQ  893
#define BAIDU_DATA_RAND_LEN     8
#define BAIDU_DATA_SN_LEN       16
#endif

#ifdef __SMART_VOICE__
//#define KEYWORD_WAKEUP_ENABLED
#define PUSH_AND_HOLD_ENABLED
//#define AI_32KBPS_VOICE
#endif

#ifdef __TENCENT_VOICE__
//#define KEYWORD_WAKEUP_ENABLED
#define PUSH_AND_HOLD_ENABLED
//#define AI_32KBPS_VOICE
#endif

//#define HF_CUSTOM_FEATURE_RESERVED          (0x01 << 0)
#define BTIF_HF_CUSTOM_FEATURE_BATTERY_REPORT    (0x03 << 0)
#define BTIF_HF_CUSTOM_FEATURE_DOCK              (0x01 << 2)
#define BTIF_HF_CUSTOM_FEATURE_SIRI_REPORT       (0x01 << 3)
#define BTIF_HF_CUSTOM_FEATURE_NR_REPORT         (0x01 << 4)


#ifndef BTIF_SUPPORT_SIRI
#define BTIF_SUPPORT_SIRI
#endif
//#define HF_CUSTOM_FEATURE_SUPPORT           (HF_CUSTOM_FEATURE_BATTERY_REPORT | HF_CUSTOM_FEATURE_SIRI_REPORT)
#ifndef BTIF_HF_CUSTOM_FEATURE_SUPPORT
#ifdef BTIF_SUPPORT_SIRI
#define BTIF_HF_CUSTOM_FEATURE_SUPPORT           (BTIF_HF_CUSTOM_FEATURE_BATTERY_REPORT | BTIF_HF_CUSTOM_FEATURE_SIRI_REPORT)
#else
#define BTIF_HF_CUSTOM_FEATURE_SUPPORT           (BTIF_HF_CUSTOM_FEATURE_BATTERY_REPORT)
#endif /*SUPPORT_SIRI*/
#endif /*HFt _CUSTOM_FEATURE_SUPPORT*/



/*
  *  default product  features
*/

#define  __BTIF_EARPHONE__

#define  __BTIF_AUTOPOWEROFF__

#define  __BTIF_BT_RECONNECT__

#define __BTIF_SNIFF__

#endif /*__SYS_BT_CFG_H__*/

