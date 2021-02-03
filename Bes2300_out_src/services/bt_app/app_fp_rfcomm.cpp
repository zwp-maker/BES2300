#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "os_api.h"
#include "bt_if.h"
#include "app_bt.h"
#include "app_spp.h"
#include "spp_api.h"
#include "sdp_api.h"

#include "app_fp_rfcomm.h"

osMutexDef(fp_rfcomm_mutex);

#if defined(__3M_PACK__)
#define L2CAP_MTU 980
#else
#define L2CAP_MTU 672
#endif

#define SPP_MAX_PACKET_SIZE L2CAP_MTU

#define FP_RFCOMM_TX_PKT_CNT 6

/* 128 bit UUID in Big Endian df21fe2c-2515-4fdb-8886-f12c4d67927c */
static const uint8_t FP_RFCOMM_UUID_128[16] = {
    0x7C, 0x92, 0x67, 0x4D, 0x2C, 0xF1, 0x86, 0x88, 0xDB, 0x4F, 0x15, 0x25, 0x2C, 0xFE, 0x21, 0xDF};

static const U8 rfcommClassId[] = {
    SDP_ATTRIB_HEADER_8BIT(17),          /* Data Element Sequence, 17 bytes */
    SDP_UUID_128BIT(FP_RFCOMM_UUID_128), /* 128 bit UUID in Big Endian */
};

static const U8 RfcommProtoDescList[] = {
    SDP_ATTRIB_HEADER_8BIT(12), /* Data element sequence, 12 bytes */

    /* Each element of the list is a Protocol descriptor which is a
    * data element sequence. The first element is L2CAP which only
    * has a UUID element.
    */
    SDP_ATTRIB_HEADER_8BIT(3), /* Data element sequence for L2CAP, 3
                                * bytes
                                */

    SDP_UUID_16BIT(PROT_L2CAP), /* Uuid16 L2CAP */

    /* Next protocol descriptor in the list is RFCOMM. It contains two
    * elements which are the UUID and the channel. Ultimately this
    * channel will need to filled in with value returned by RFCOMM.
    */

    /* Data element sequence for RFCOMM, 5 bytes */
    SDP_ATTRIB_HEADER_8BIT(5),

    SDP_UUID_16BIT(PROT_RFCOMM), /* Uuid16 RFCOMM */

    /* Uint8 RFCOMM channel number - value can vary */
    SDP_UINT_8BIT(RFCOMM_CHANNEL_FP)};

/*
 * BluetoothProfileDescriptorList
 */
static const U8 ProfileDescList[] = {
    SDP_ATTRIB_HEADER_8BIT(8), /* Data element sequence, 8 bytes */

    /* Data element sequence for ProfileDescriptor, 6 bytes */
    SDP_ATTRIB_HEADER_8BIT(6),

    SDP_UUID_16BIT(SC_SERIAL_PORT), /* Uuid16 SPP */
    SDP_UINT_16BIT(0x0102)          /* As per errata 2239 */
};

/* SPP attributes.
 *
 * This is a ROM template for the RAM structure used to register the
 * SPP SDP record.
 */
static sdp_attribute_t fpRfcommAttributes[] = {
    SDP_ATTRIBUTE(AID_SERVICE_CLASS_ID_LIST, rfcommClassId),

    SDP_ATTRIBUTE(AID_PROTOCOL_DESC_LIST, RfcommProtoDescList),

    SDP_ATTRIBUTE(AID_BT_PROFILE_DESC_LIST, ProfileDescList),
};

typedef struct
{
    uint8_t    isConnected;
    uint8_t    isRfcommInitialized;
} FpRFcommServiceEnv_t;

typedef union
{
    struct
    {
        uint8_t     isCompanionAppInstalled :   1;
        uint8_t     isSilentModeSupported   :   1;
        uint8_t     reserve                 :   6;
    } env;
    uint8_t content;
} FpCapabilitiesEnv_t;

typedef struct
{
    uint8_t isRightRinging  :   1;
    uint8_t isLeftRinging   :   1;
    uint8_t reserve         :   6;
} FpRingStatus_t;

FpRFcommServiceEnv_t fp_rfcomm_service = {false, false};

static FpCapabilitiesEnv_t fp_capabilities = {false, false, 0};

static __attribute__((unused)) FpRingStatus_t fp_ring_status = {false, false, 0};

struct spp_device fpSppDev;
struct spp_service fpSppService;
btif_sdp_record_t* fpSppSdpRecord;

extern "C" void     app_gfps_get_battery_levels(uint8_t *pCount, uint8_t *pBatteryLevel);
extern "C" uint8_t *appm_get_current_ble_addr(void);
extern "C" int app_bt_start_custom_function_in_bt_thread(
    uint32_t param0, uint32_t param1, uint32_t funcPtr);

static int fp_rfcomm_data_received(void *pDev, uint8_t process, uint8_t *pData, uint16_t dataLen);
static void app_fp_msg_send_active_components_rsp(void);
static void app_fp_msg_send_message_ack(uint8_t msgGroup, uint8_t msgCode);
static void app_fp_msg_send_message_nak(uint8_t reason, uint8_t msgGroup, uint8_t msgCode);

// update this value if the maximum possible tx data size is bigger than current value
#define FP_RFCOMM_TX_BUF_CHUNK_SIZE 64
#define FP_RFCOMM_TX_BUF_CHUNK_CNT FP_RFCOMM_TX_PKT_CNT
#define FP_RFCOMM_TX_BUF_SIZE (FP_RFCOMM_TX_BUF_CHUNK_CNT * FP_RFCOMM_TX_BUF_CHUNK_SIZE)

#define RFCOMM_NORMAL_RECV_BUFFER_SIZE 800

static uint32_t fp_rfcomm_tx_buf_next_allocated_chunk = 0;
static uint32_t fp_rfcomm_tx_buf_allocated_chunk_cnt  = 0;
static uint8_t  fp_rfcomm_tx_buf[FP_RFCOMM_TX_BUF_CHUNK_CNT][FP_RFCOMM_TX_BUF_CHUNK_SIZE];

static uint8_t fp_rfcomm_rx_buf[RFCOMM_NORMAL_RECV_BUFFER_SIZE];

static uint8_t *fp_rfcomm_tx_buf_addr(uint32_t chunk)
{
    return fp_rfcomm_tx_buf[chunk];
}

static int32_t fp_rfcomm_alloc_tx_chunk(void)
{
    uint32_t lock = int_lock_global();
    
    if (fp_rfcomm_tx_buf_allocated_chunk_cnt >= FP_RFCOMM_TX_BUF_CHUNK_CNT)
    {
        int_unlock_global(lock);
        return -1;
    }

    uint32_t returnedChunk = fp_rfcomm_tx_buf_next_allocated_chunk; 
    
    fp_rfcomm_tx_buf_allocated_chunk_cnt++;
    fp_rfcomm_tx_buf_next_allocated_chunk++;
    if (FP_RFCOMM_TX_BUF_CHUNK_CNT == fp_rfcomm_tx_buf_next_allocated_chunk)
    {
        fp_rfcomm_tx_buf_next_allocated_chunk = 0;
    }

    int_unlock_global(lock);
    return returnedChunk;
}

static bool fp_rfcomm_free_tx_chunk(void)
{
    uint32_t lock = int_lock_global();
    if (0 == fp_rfcomm_tx_buf_allocated_chunk_cnt)
    {
        int_unlock_global(lock);
        return false;
    }

    fp_rfcomm_tx_buf_allocated_chunk_cnt--;
    int_unlock_global(lock);
    return true;
}

static void fp_rfcomm_reset_tx_buf(void)
{
    uint32_t lock = int_lock_global();
    fp_rfcomm_tx_buf_allocated_chunk_cnt = 0;
    fp_rfcomm_tx_buf_next_allocated_chunk = 0;
    int_unlock_global(lock);
}

static void fp_rfcomm_ring_request_handling(uint8_t request)
{
    // TODO: implement the continuous audio prompt playing state machine
#if 0
    uint8_t isAllowed = false;
    uint8_t isChangePeerDevStatus = false;
    switch (request)
    {
        case 3:
        {
             if (IS_CONNECTED_WITH_TWS() && 
                ((0 == fp_ring_status.isLeftRinging) && 
                    (0 == fp_ring_status.isRightRinging)))
            {
                // TODO: start ringing on both sides
                fp_ring_status.isLeftRinging = true;
                fp_ring_status.isRightRinging = true;

                isAllowed = true;
            }
            break;
        }
        case 1:            
            if (0 == fp_ring_status.isRightRinging)
            {
                if (((fp_ring_status.isLeftRinging) && app_tws_is_right_side()) ||
                    app_tws_is_left_side())
                {
                    isChangePeerDevStatus = true;
                }

                if (isChangePeerDevStatus && !IS_CONNECTED_WITH_TWS())
                {
                    break;
                }

                // start right ring
                if (app_tws_is_left_side())
                {
                    // TODO: start ringing on peer device
                }
                else if (app_tws_is_right_side())
                {
                    // TODO: start local ringing
                }

                if (fp_ring_status.isLeftRinging)
                {
                    // stop left ring
                    if (app_tws_is_right_side())
                    {
                        // TODO: stop ringing on peer device
                    }
                    else if (app_tws_is_left_side())
                    {
                        // TODO: stop local ringing
                    }                  
                }
                
                fp_ring_status.isLeftRinging = false;
                fp_ring_status.isRightRinging = true;

                isAllowed = true;
            }
            break;
        case 2:
            if (0 == fp_ring_status.isLeftRinging)
            {
                if (((fp_ring_status.isRightRinging) && app_tws_is_left_side()) ||
                    app_tws_is_right_side())
                {
                    isChangePeerDevStatus = true;
                }

                if (isChangePeerDevStatus && !IS_CONNECTED_WITH_TWS())
                {
                    break;
                }

                // start left ring
                if (app_tws_is_right_side())
                {
                    // TODO: start ringing on peer device
                }
                else if (app_tws_is_left_side())
                {
                    // TODO: start local ringing
                }

                if (fp_ring_status.isRightRinging)
                {
                    // stop left ring
                    if (app_tws_is_left_side())
                    {
                        // TODO: stop ringing on peer device
                    }
                    else if (app_tws_is_right_side())
                    {
                        // TODO: stop local ringing
                    }                  
                }

                fp_ring_status.isLeftRinging = true;
                fp_ring_status.isRightRinging = false;
                isAllowed = true;
            }
            break;
        case 0:
             if (IS_CONNECTED_WITH_TWS() && 
                ((1 == fp_ring_status.isLeftRinging) && 
                    (1 == fp_ring_status.isRightRinging)))
            {
                // TODO: stop ringing on both sides
                fp_ring_status.isLeftRinging = false;
                fp_ring_status.isRightRinging = false;

                isAllowed = true;
            }
            else
            {
                if (((fp_ring_status.isLeftRinging) && app_tws_is_right_side()) ||
                    ((fp_ring_status.isRightRinging) && app_tws_is_left_side()))
                {
                    isChangePeerDevStatus = true;
                }

                if (isChangePeerDevStatus && !IS_CONNECTED_WITH_TWS())
                {
                    break;
                }

                // stop right ring
                if (fp_ring_status.isRightRinging)
                {
                    if (app_tws_is_left_side())
                    {
                        // TODO: stop ringing on peer device
                    }
                    else if (app_tws_is_right_side())
                    {
                        // TODO: stop local ringing
                    }                  
                }

                if (fp_ring_status.isLeftRinging)
                {
                    // stop left ring
                    if (app_tws_is_right_side())
                    {
                        // TODO: stop ringing on peer device
                    }
                    else if (app_tws_is_left_side())
                    {
                        // TODO: stop local ringing
                    }                  
                }
                
                fp_ring_status.isLeftRinging = false;
                fp_ring_status.isRightRinging = false;

                isAllowed = true;
            }
            break;
        default:
            isAllowed = false;
            break;
    }

    if (isAllowed)
    {
        app_fp_msg_send_message_ack(FP_MSG_GROUP_DEVICE_ACTION, FP_MSG_DEVICE_ACTION_RING);
    }
    else
    {
        app_fp_msg_send_message_nak(FP_MSG_NAK_REASON_NOT_ALLOWED, 
            FP_MSG_GROUP_DEVICE_ACTION, FP_MSG_DEVICE_ACTION_RING);        
    }
#endif
}

static void fp_rfcomm_data_handler(uint8_t* ptr, uint16_t len)
{
    FP_MESSAGE_STREAM_T* pMsg = (FP_MESSAGE_STREAM_T *)ptr;
    TRACE("fp rfcomm receives msg group %d code %d", 
        pMsg->messageGroup, pMsg->messageCode);
        
    switch (pMsg->messageGroup)
    {
        case FP_MSG_GROUP_DEVICE_INFO:
        {
            switch (pMsg->messageCode)
            {
                case FP_MSG_DEVICE_INFO_ACTIVE_COMPONENTS_REQ:
                    app_fp_msg_send_active_components_rsp();
                    break;
                case FP_MSG_DEVICE_INFO_TELL_CAPABILITIES:
                    fp_capabilities.content = pMsg->data[0];
                    TRACE("cap 0x%x isCompanionAppInstalled %d isSilentModeSupported %d",
                        fp_capabilities.content, fp_capabilities.env.isCompanionAppInstalled,
                        fp_capabilities.env.isSilentModeSupported);
                    break;
                default:
                    break;
            }
        }
        case FP_MSG_GROUP_DEVICE_ACTION:
        {
            switch (pMsg->messageCode)
            {
                case FP_MSG_DEVICE_ACTION_RING:
                    fp_rfcomm_ring_request_handling(pMsg->data[0]);
                    break;
                default:
                    break;
            }
        }
        default:
            break;
    }
}

static int fp_rfcomm_data_received(void *pDev, uint8_t process, uint8_t *pData, uint16_t dataLen)
{
    fp_rfcomm_data_handler(pData, dataLen);
    return 0;
}

static void fp_rfcomm_connected_handler(void)
{
    if (!fp_rfcomm_service.isConnected)
    {
        fp_rfcomm_service.isConnected = true;

        app_fp_msg_send_model_id();
        app_fp_msg_send_updated_ble_addr();
        app_fp_msg_send_battery_levels();
    }
}

static void fp_rfcomm_callback(struct spp_device *locDev, struct spp_callback_parms *Info)
{
    struct spp_tx_done *pTxDone = NULL;

    switch (Info->event)
    {
    case BTIF_SPP_EVENT_REMDEV_CONNECTED:
        TRACE("::RFCOMM_CONNECTED");
        fp_rfcomm_connected_handler();
        break;

    case BTIF_SPP_EVENT_REMDEV_DISCONNECTED:
        TRACE("::RFCOMM_DISCONNECTED");
        fp_rfcomm_service.isConnected = false;

        fp_rfcomm_reset_tx_buf();
        break;

    case BTIF_SPP_EVENT_DATA_SENT:
        TRACE("::RFCOMM_DATA_SENT");
        pTxDone = ( struct spp_tx_done * )(Info->p.other);
        TRACE("Rfcomm dataLen %d sent out", pTxDone->tx_data_length);
        fp_rfcomm_free_tx_chunk();
        break;

    case BTIF_SPP_EVENT_REMDEV_DISCONNECTED_IND:
        TRACE("::RFCOMM_DISCONNECTED_IND");
        break;

    case BTIF_SPP_EVENT_REMDEV_CONNECTED_IND:
        TRACE("::RFCOMM_CONNECTED_IND");
        fp_rfcomm_connected_handler();
        break;

    default:
        TRACE("Unkown rfcomm event %d", Info->event);
        break;
    }

    return;
}

static void app_fp_disconnect_rfcomm_handler(void)
{
    if (fp_rfcomm_service.isConnected)
    {
        btif_spp_close(&fpSppDev);
    }
}

void app_fp_disconnect_rfcomm(void)
{
    app_bt_start_custom_function_in_bt_thread(0,
                                           0,
                                           ( uint32_t )app_fp_disconnect_rfcomm_handler);
}

static void app_fp_rfcomm_send_handler(uint8_t *ptrData, uint32_t length)
{
    uint16_t len = length;
    if (BT_STS_SUCCESS != btif_spp_write(&fpSppDev, ( char * )ptrData, &len))
    {
        TRACE("fp_rfcomm send data failed.");
    }
}

static void app_fp_rfcomm_send(uint8_t *ptrData, uint32_t length)
{
    if (!fp_rfcomm_service.isConnected)
    {
        return;
    }

    int32_t chunk = fp_rfcomm_alloc_tx_chunk();
    if (-1 == chunk)
    {
        TRACE("Fast pair rfcomm tx buffer used out!");
        return;
    }

    ASSERT(length < FP_RFCOMM_TX_BUF_CHUNK_SIZE,
           "FP_RFCOMM_TX_BUF_CHUNK_SIZE is %d which is smaller than %d, need to increase!",
           FP_RFCOMM_TX_BUF_CHUNK_SIZE,
           length);

    uint8_t *txBufAddr = fp_rfcomm_tx_buf_addr(chunk);
    memcpy(txBufAddr, ptrData, length);
    app_bt_start_custom_function_in_bt_thread(( uint32_t )txBufAddr,
                                           ( uint32_t )length,
                                           ( uint32_t )app_fp_rfcomm_send_handler);
}

bt_status_t app_fp_rfcomm_init(void)
{
    bt_status_t stat = BT_STS_FAILED;

    if (!fp_rfcomm_service.isRfcommInitialized)
    {
        osMutexId mid;
        mid = osMutexCreate(osMutex(fp_rfcomm_mutex));
        if (!mid) {
            ASSERT(0, "cannot create mutex");
        }

        fp_rfcomm_service.isRfcommInitialized = true;
        fpSppDev.portType                     = BTIF_SPP_SERVER_PORT;
        fpSppDev.app_id = BTIF_APP_SPP_SERVER_FP_RFCOMM_ID;
        fpSppDev.spp_handle_data_event_func = fp_rfcomm_data_received;
        btif_spp_init_rx_buf(&fpSppDev,
                        fp_rfcomm_rx_buf,
                        ARRAY_SIZE(fp_rfcomm_rx_buf));

        fpSppSdpRecord = btif_sdp_create_record();

        btif_sdp_record_param_t param;
        param.attrs = fpRfcommAttributes,
        param.attr_count = ARRAY_SIZE(fpRfcommAttributes);
        param.COD = BTIF_COD_MAJOR_PERIPHERAL;
        btif_sdp_record_setup(fpSppSdpRecord, &param); 
  
        fpSppService.rf_service.serviceId = RFCOMM_CHANNEL_FP;
        fpSppService.numPorts             = 0;

        btif_spp_service_setup(&fpSppDev, &fpSppService, fpSppSdpRecord);
        btif_spp_init_device(&fpSppDev, FP_RFCOMM_TX_PKT_CNT, mid);

        stat = btif_spp_open(&fpSppDev, NULL, fp_rfcomm_callback);

        if (BT_STS_SUCCESS != stat)
        {
            TRACE("fast pair rfcomm open failed");
        }

        fp_rfcomm_service.isConnected = false;
    }
    else
    {
        TRACE("already initialized.");
    }

    return stat;
}

bool app_is_fp_rfcomm_connected(void)
{
    return fp_rfcomm_service.isConnected;
}

// use cases for fp message stream
void app_fp_msg_enable_bt_silence_mode(bool isEnable)
{
    if (fp_capabilities.env.isSilentModeSupported)
    {
        FP_MESSAGE_STREAM_T req = {FP_MSG_GROUP_BLUETOOTH_EVENT, 0, 0, 0};
        if (isEnable)
        {
            req.messageCode = FP_MSG_BT_EVENT_ENABLE_SILENCE_MODE;
        }
        else
        {
            req.messageCode = FP_MSG_BT_EVENT_DISABLE_SILENCE_MODE;
        }

        app_fp_rfcomm_send((uint8_t *)&req, FP_MESSAGE_RESERVED_LEN);
    }
    else
    {
        TRACE("fp silence mode is not supported.");
    }
}

void app_fp_msg_send_model_id(void)
{
    uint32_t model_id = 0x000104;
    uint8_t  modelID[3];
    modelID[0] = (model_id >> 16) & 0xFF;
    modelID[1] = (model_id >> 8) & 0xFF;
    modelID[2] = ( model_id )&0xFF;

    uint16_t rawDataLen = sizeof(modelID);

    FP_MESSAGE_STREAM_T req =
        {FP_MSG_GROUP_DEVICE_INFO,
         FP_MSG_DEVICE_INFO_MODEL_ID,
         (uint8_t)(rawDataLen >> 8),
         (uint8_t)(rawDataLen & 0xFF)};
    memcpy(req.data, modelID, sizeof(modelID));

    app_fp_rfcomm_send(( uint8_t * )&req, FP_MESSAGE_RESERVED_LEN + rawDataLen);
}

void app_fp_msg_send_updated_ble_addr(void)
{
    FP_MESSAGE_STREAM_T req =
        {FP_MSG_GROUP_DEVICE_INFO,
         FP_MSG_DEVICE_INFO_BLE_ADD_UPDATED,
         0,
         6};

    uint8_t *ptr = appm_get_current_ble_addr();

    for (uint8_t index = 0; index < 6; index++)
    {
        req.data[index] = ptr[5 - index];
    }

    app_fp_rfcomm_send(( uint8_t * )&req, FP_MESSAGE_RESERVED_LEN + 6);
}

void app_fp_msg_send_battery_levels(void)
{
    FP_MESSAGE_STREAM_T req =
        {FP_MSG_GROUP_DEVICE_INFO,
         FP_MSG_DEVICE_INFO_BATTERY_UPDATED,
         0,
         3};

    uint8_t batteryLevelCount = 0;
    app_gfps_get_battery_levels(&batteryLevelCount, req.data);

    app_fp_rfcomm_send(( uint8_t * )&req, FP_MESSAGE_RESERVED_LEN + 3);
}

static __attribute__((unused)) void app_fp_msg_send_active_components_rsp(void)
{
    FP_MESSAGE_STREAM_T req = {FP_MSG_GROUP_DEVICE_INFO, FP_MSG_DEVICE_INFO_ACTIVE_COMPONENTS_RSP, 0, 1};

    // TODO: handle tws case
    req.data[0] = FP_MSG_BOTH_BUDS_ACTIVE;

    app_fp_rfcomm_send((uint8_t *)&req, FP_MESSAGE_RESERVED_LEN+1);
}

static __attribute__((unused)) void app_fp_msg_send_message_ack(uint8_t msgGroup, uint8_t msgCode)
{
    FP_MESSAGE_STREAM_T req = {FP_MSG_GROUP_ACKNOWLEDGEMENT, FP_MSG_ACK, 0, 2};

    req.data[0] = msgGroup;
    req.data[1] = msgCode;

    app_fp_rfcomm_send((uint8_t *)&req, FP_MESSAGE_RESERVED_LEN+2);
}

static __attribute__((unused)) void app_fp_msg_send_message_nak(uint8_t reason, uint8_t msgGroup, uint8_t msgCode)
{
    FP_MESSAGE_STREAM_T req = {FP_MSG_GROUP_ACKNOWLEDGEMENT, FP_MSG_NAK, 0, 3};

    req.data[0] = reason;
    req.data[1] = msgGroup;
    req.data[2] = msgCode;

    app_fp_rfcomm_send((uint8_t *)&req, FP_MESSAGE_RESERVED_LEN+3);
}

