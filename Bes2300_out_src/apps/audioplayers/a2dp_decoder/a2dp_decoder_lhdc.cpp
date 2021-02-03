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
// Standard C Included Files
#include "cmsis.h"
#include "plat_types.h"
#include <string.h>
#include "heap_api.h"
#include "hal_location.h"
#include "a2dp_decoder_internal.h"

typedef struct {
    uint16_t sequenceNumber;
    uint32_t timestamp;    
    uint16_t curSubSequenceNumber;
    uint16_t totalSubSequenceNumber;
    uint8_t *buffer;
    uint32_t buffer_len;
} a2dp_audio_lhdc_decoder_frame_t;

#define LHDC_MTU_LIMITER (50) /*must <= 23*/

#define A2DP_LHDC_OUTPUT_FRAME_SAMPLES   (512)
#define A2DP_LHDC_DEFAULT_LATENCY        (1)
#if defined(A2DP_LHDC_V3)
#define PACKET_BUFFER_LENGTH             (2 * 1024)
#else
#define PACKET_BUFFER_LENGTH             (4 * 1024)
#endif

#define A2DP_LHDC_HDR_F_MSK 0x80
#define A2DP_LHDC_HDR_S_MSK 0x40
#define A2DP_LHDC_HDR_L_MSK 0x20
#define A2DP_LHDC_HDR_FLAG_MSK ( A2DP_LHDC_HDR_F_MSK | A2DP_LHDC_HDR_S_MSK | A2DP_LHDC_HDR_L_MSK)

#define A2DP_LHDC_HDR_LATENCY_LOW   0x00
#define A2DP_LHDC_HDR_LATENCY_MID   0x01
#define A2DP_LHDC_HDR_LATENCY_HIGH  0x02
#define A2DP_LHDC_HDR_LATENCY_MASK  (A2DP_LHDC_HDR_LATENCY_MID | A2DP_LHDC_HDR_LATENCY_HIGH)

#define A2DP_LHDC_HDR_FRAME_NO_MASK 0xfc

typedef enum {
    ASM_PKT_WAT_STR,
    ASM_PKT_WAT_LST,
}ASM_PKT_STATUS;

/**
 * get lhdc frame header
 */

/**
    LHDC frame
*/
typedef struct _lhdc_frame_Info {
    uint32_t frame_len; //該frame的長度，若是分離壓縮，則表示單一聲道的frame長度。
    uint32_t isSplit;   //是否為分離方式厭縮
    uint32_t isLeft;    //左聲道=true，右聲道=false
    
} lhdc_frame_Info_t;

extern "C"{
    typedef struct bes_bt_local_info_t{
        uint8_t bt_addr[BTIF_BD_ADDR_SIZE];
        const char *bt_name;
        uint8_t bt_len;
        uint8_t ble_addr[BTIF_BD_ADDR_SIZE];
        const char *ble_name;
        uint8_t ble_len;
    }bes_bt_local_info;

    typedef int (*LHDC_GET_BT_INFO)(bes_bt_local_info * bt_info);

    void lhdcInit(uint32_t bitPerSample, uint32_t sampleRate, uint32_t scaleTo16Bits);
#if defined(A2DP_LHDC_V3)
    uint32_t lhdcPutData(uint8_t * pIn, uint32_t len);
#else
    int32_t lhdcPutFrame(uint8_t * pIn, int32_t len);
#endif
    uint32_t lhdcDecodeProcess(uint8_t * pOutBuf);
    void lhdcDestroy();
    bool lhdcSetLicenseKeyTable(uint8_t * licTable, LHDC_GET_BT_INFO pFunc);
    const char * getVersionCode();
}

static A2DP_AUDIO_CONTEXT_T *a2dp_audio_context_p = NULL;
static A2DP_AUDIO_DECODER_LASTFRAME_INFO_T a2dp_audio_lhdc_lastframe_info;
static A2DP_AUDIO_OUTPUT_CONFIG_T a2dp_audio_lhdc_output_config;
static uint16_t lhdc_mtu_limiter = LHDC_MTU_LIMITER;

static uint8_t serial_no;
static bool is_synced;
static ASM_PKT_STATUS asm_pkt_st;
static uint8_t packet_buffer[PACKET_BUFFER_LENGTH];
static uint32_t packet_buf_len = 0;
#if defined(A2DP_LHDC_V3)
uint8_t latencyUpdated = 0;
#else
static uint32_t total_frame_nb = 0;
#endif


extern const char testkey_bin[];
extern int bes_bt_local_info_get(bes_bt_local_info *local_info);


static void reset_lhdc_assmeble_packet(void)
{
    is_synced = false;
    asm_pkt_st = ASM_PKT_WAT_STR;
    packet_buf_len = 0;
}

static void initial_lhdc_assemble_packet(bool splitFlg)
{
    memset(packet_buffer, 0, PACKET_BUFFER_LENGTH);
    reset_lhdc_assmeble_packet();
    serial_no = 0xff;
}


/**
 * A2DP packet 組包
 */
 #if defined(A2DP_LHDC_V3)
  int assemble_lhdc_packet(uint8_t * input, uint32_t input_len, uint8_t** pLout, uint32_t * pLlen){
     uint8_t hdr = 0, seqno = 0xff;
     int ret = -1;
     uint32_t status = 0;
 
     hdr = (*input);
     input++;
     seqno = (*input);
     input++;
     input_len -= 2;
 
     //Check latency and update value when changed.
     status = hdr & A2DP_LHDC_HDR_LATENCY_MASK;
     if (latencyUpdated != (uint8_t)status) {
         latencyUpdated = (uint8_t)status;
         TRACE( "New latency setting = 0x%02x", latencyUpdated);
     }
 
     //Get number of frame in packet.
     status = (hdr & A2DP_LHDC_HDR_FRAME_NO_MASK) >> 2;
     if (status <= 0) {
         TRACE( "No any frame in packet.");
         return 0;
     }
     if( seqno != serial_no )
     {
         TRACE( "Packet lost! now(%d), expect(%d)", seqno, serial_no);
     }
     serial_no = seqno + 1;
 
     if (pLlen && pLout) {
         *pLlen = input_len;
         *pLout = input;
     }
     ret = 1;
 
     return ret;
 }
#else
int assemble_lhdc_packet(uint8_t * input, uint32_t input_len, uint8_t** pLout, uint32_t * pLlen){
    uint8_t hdr = 0, seqno = 0xff;
    int ret = -1;
    //uint32_t status = 0;

    hdr = (*input);
    input++;
    seqno = (*input);
    input++;
    input_len -= 2;

    if( is_synced)
    {
        if( seqno != serial_no )
        {
            reset_lhdc_assmeble_packet();
            if( (hdr & A2DP_LHDC_HDR_FLAG_MSK) == 0 ||
                (hdr & A2DP_LHDC_HDR_S_MSK) != 0 )
            {
                goto lhdc_start;
            }
            else
            TRACE( "drop packet No. %u", seqno);
            return 0;
        }
        serial_no = seqno + 1;
    }

lhdc_start:
    switch (asm_pkt_st) {
        case ASM_PKT_WAT_STR:{
                if( (hdr & A2DP_LHDC_HDR_FLAG_MSK) == 0 )
                {
                    memcpy(&packet_buffer[0], input, input_len);
                    if (pLlen && pLout) {
                        *pLlen = input_len;
                        *pLout = packet_buffer;
                    }
                    //TRACE("Single payload size = %d", *pLlen);
                    asm_pkt_st = ASM_PKT_WAT_STR;
                    packet_buf_len = 0;//= packet_buf_left_len = packet_buf_right_len = 0;
                    total_frame_nb = 0;
                    ret = 1;
                }
                else if( hdr & A2DP_LHDC_HDR_S_MSK )
                {
                    ret = 0;
                    if (packet_buf_len + input_len >= PACKET_BUFFER_LENGTH) {
                        packet_buf_len = 0;
                        asm_pkt_st = ASM_PKT_WAT_STR;
                        TRACE("ASM_PKT_WAT_STR:Frame buffer overflow!(%d)", packet_buf_len);
                        break;
                    }
                    memcpy(&packet_buffer, input, input_len);
                    packet_buf_len = input_len;
                    asm_pkt_st = ASM_PKT_WAT_LST;
                    //TRACE("multi:first payload size = %d", input_len);
                }
                else
                    ret = -1;

                if( ret >= 0 )
                {
                    if( !is_synced )
                    {
                        is_synced = true;
                        serial_no = seqno + 1;
                    }
                }
            break;
        }
        case ASM_PKT_WAT_LST:{
            if (packet_buf_len + input_len >= PACKET_BUFFER_LENGTH) {
                packet_buf_len = 0;
                asm_pkt_st = ASM_PKT_WAT_STR;
                TRACE("ASM_PKT_WAT_LST:Frame buffer overflow(%d)", packet_buf_len);
                break;
            }
            memcpy(&packet_buffer[packet_buf_len], input, input_len);
            //TRACE("multi:payload size = %d", input_len);
            packet_buf_len += input_len;
            ret = 0;

            if( hdr & A2DP_LHDC_HDR_L_MSK )
            {

                if (pLlen && pLout) {
                    *pLlen = packet_buf_len;
                    *pLout = packet_buffer;
                }
                //TRACE("multi: all payload size = %d", packet_buf_len);
                packet_buf_len = 0; //packet_buf_left_len = packet_buf_right_len = 0;
                total_frame_nb = 0;
                ret = 1;
                asm_pkt_st = ASM_PKT_WAT_STR;
            }
            break;
        }
        default:
        ret = 0;
        break;

    }
    return ret;
}
#endif

static int get_lhdc_header(uint8_t * in, lhdc_frame_Info_t * h) {
    #define LHDC_HDR_LEN 4
    uint32_t hdr = 0;
    int ret = -1;
    memcpy(&hdr, in , LHDC_HDR_LEN);
    h->frame_len = ( int)( ( hdr >> 8) & 0x1fff);
    h->isSplit = ( ( hdr & 0x00600000) == 0x00600000);
    h->isLeft = ((hdr & 0xf) == 0);

    if (( hdr & 0xff000000) != 0x4c000000){
    } else {
        ret = 0;
    }
    return ret;
}

int a2dp_audio_lhdc_decode_frame(uint8_t *buffer, uint32_t buffer_bytes)
{
    list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
    list_node_t *node = NULL;
    a2dp_audio_lhdc_decoder_frame_t *lhdc_decoder_frame_p = NULL;
    
    bool cache_underflow = false;
    int output_byte = 0;
    
    uint32_t lhdc_out_len = 0;

    node = a2dp_audio_list_begin(list);    
    if (!node){
        TRACE("lhdc_decode cache underflow");
        cache_underflow = true;
        goto exit;
    }else{
        lhdc_decoder_frame_p = (a2dp_audio_lhdc_decoder_frame_t *)a2dp_audio_list_node(node);
#if defined(A2DP_LHDC_V3)
        lhdcPutData(lhdc_decoder_frame_p->buffer, lhdc_decoder_frame_p->buffer_len);
#else
        lhdcPutFrame(lhdc_decoder_frame_p->buffer, lhdc_decoder_frame_p->buffer_len);
#endif
        do {
            lhdc_out_len = lhdcDecodeProcess(buffer+output_byte);
            if (lhdc_out_len > 0){
                output_byte += lhdc_out_len;
            }else{
                break;
            }
        }while(output_byte < (int)buffer_bytes && output_byte > 0);

        if (output_byte != (int)buffer_bytes){
            TRACE_IMM("[warning] lhdc_decode_frame output_byte:%d lhdc_out_len:%d buffer_bytes:%d", output_byte, lhdc_out_len, buffer_bytes);
            TRACE_IMM("[warning] lhdc_decode_frame frame_len:%d rtp seq:%d timestamp:%d decoder_frame:%d/%d ", 
                                                                                           lhdc_decoder_frame_p->buffer_len,
                                                                                           lhdc_decoder_frame_p->sequenceNumber,
                                                                                           lhdc_decoder_frame_p->timestamp,
                                                                                           lhdc_decoder_frame_p->curSubSequenceNumber,
                                                                                           lhdc_decoder_frame_p->totalSubSequenceNumber);
            output_byte = buffer_bytes;
            int32_t dump_byte = lhdc_decoder_frame_p->buffer_len;
            int32_t dump_offset = 0;
            while(1){
                uint32_t dump_byte_output = 0;
                dump_byte_output = dump_byte>32?32:dump_byte;              
                DUMP8("%02x ", lhdc_decoder_frame_p->buffer + dump_offset, dump_byte_output);
                dump_offset += dump_byte_output;
                dump_byte -= dump_byte_output;
                if (dump_byte<=0){
                    break;
                }
            }
            ASSERT(0, "%s", __func__);
        }

        a2dp_audio_lhdc_lastframe_info.sequenceNumber = lhdc_decoder_frame_p->sequenceNumber;
        a2dp_audio_lhdc_lastframe_info.timestamp = lhdc_decoder_frame_p->timestamp;
        a2dp_audio_lhdc_lastframe_info.curSubSequenceNumber = lhdc_decoder_frame_p->curSubSequenceNumber;
        a2dp_audio_lhdc_lastframe_info.totalSubSequenceNumber = lhdc_decoder_frame_p->totalSubSequenceNumber;
        a2dp_audio_lhdc_lastframe_info.frame_samples = A2DP_LHDC_OUTPUT_FRAME_SAMPLES;
        a2dp_audio_lhdc_lastframe_info.decoded_frames++;
        a2dp_audio_lhdc_lastframe_info.undecode_frames = a2dp_audio_list_length(list)-1;
        a2dp_audio_decoder_internal_lastframe_info_set(&a2dp_audio_lhdc_lastframe_info);        
        a2dp_audio_list_remove(list, lhdc_decoder_frame_p);
    }
exit:
    if(cache_underflow){
        reset_lhdc_assmeble_packet();
        a2dp_audio_lhdc_lastframe_info.undecode_frames = 0;
        a2dp_audio_decoder_internal_lastframe_info_set(&a2dp_audio_lhdc_lastframe_info);        
        output_byte = A2DP_DECODER_CACHE_UNDERFLOW_ERROR;
    }
    return output_byte;
}

int a2dp_audio_lhdc_preparse_packet(btif_media_header_t * header, uint8_t *buffer, uint32_t buffer_bytes)
{
    a2dp_audio_lhdc_lastframe_info.sequenceNumber = header->sequenceNumber;
    a2dp_audio_lhdc_lastframe_info.timestamp = header->timestamp;
    a2dp_audio_lhdc_lastframe_info.curSubSequenceNumber = 0;
    a2dp_audio_lhdc_lastframe_info.totalSubSequenceNumber = 0;
    a2dp_audio_lhdc_lastframe_info.frame_samples = A2DP_LHDC_OUTPUT_FRAME_SAMPLES;
    a2dp_audio_lhdc_lastframe_info.list_samples = A2DP_LHDC_OUTPUT_FRAME_SAMPLES;
    a2dp_audio_lhdc_lastframe_info.decoded_frames = 0;
    a2dp_audio_lhdc_lastframe_info.undecode_frames = 0;
    a2dp_audio_decoder_internal_lastframe_info_set(&a2dp_audio_lhdc_lastframe_info);

    TRACE("%s seq:%d timestamp:%08x", __func__, header->sequenceNumber, header->timestamp);

    return A2DP_DECODER_NO_ERROR;
}

void *a2dp_audio_lhdc_frame_malloc(uint32_t packet_len)
{
    a2dp_audio_lhdc_decoder_frame_t *decoder_frame_p = NULL;
    uint8_t *buffer = NULL;

    buffer = (uint8_t *)a2dp_audio_heap_malloc(packet_len);
    decoder_frame_p = (a2dp_audio_lhdc_decoder_frame_t *)a2dp_audio_heap_malloc(sizeof(a2dp_audio_lhdc_decoder_frame_t));
    decoder_frame_p->buffer = buffer;
    decoder_frame_p->buffer_len = packet_len;
    return (void *)decoder_frame_p;
}

void a2dp_audio_lhdc_free(void *packet)
{
    a2dp_audio_lhdc_decoder_frame_t *decoder_frame_p = (a2dp_audio_lhdc_decoder_frame_t *)packet; 
    a2dp_audio_heap_free(decoder_frame_p->buffer);
    a2dp_audio_heap_free(decoder_frame_p);
}

int a2dp_audio_lhdc_store_packet(btif_media_header_t * header, uint8_t *buffer, uint32_t buffer_bytes)
{
    list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;    
    int nRet = A2DP_DECODER_NO_ERROR;
    uint32_t frame_num = 0;    
    uint32_t frame_cnt = 0;
    uint32_t lSize = 0;
    uint8_t *lPTR = NULL;
    lhdc_frame_Info_t lhdc_frame_Info;
    uint32_t ptr_offset = 0;

    if (assemble_lhdc_packet(buffer, buffer_bytes, &lPTR, &lSize) > 0){
        if (lPTR != NULL && lSize != 0){
            ptr_offset = 0;
            while (get_lhdc_header(lPTR+ptr_offset, &lhdc_frame_Info) == 0 && ptr_offset < lSize){
                ptr_offset += lhdc_frame_Info.frame_len;
                frame_num++;
            }
            ptr_offset = 0;
            while (get_lhdc_header(lPTR+ptr_offset, &lhdc_frame_Info) == 0 && ptr_offset < lSize){
                a2dp_audio_lhdc_decoder_frame_t *decoder_frame_p = NULL;
                if (!lhdc_frame_Info.frame_len){
                    DUMP8("%02x ", lPTR+ptr_offset, 32);
                    ASSERT(0, "lhdc_frame_Info error frame_len:%d offset:%d ptr:%08x/%08x", lhdc_frame_Info.frame_len, ptr_offset, buffer, lPTR);
                }
                ASSERT(lhdc_frame_Info.frame_len <= (lSize - ptr_offset), "%s frame_len:%d ptr_offset:%d buffer_bytes:%d",
                                                                                   __func__, lhdc_frame_Info.frame_len, ptr_offset, lSize);
                if (a2dp_audio_list_length(list) < lhdc_mtu_limiter){
                    decoder_frame_p = (a2dp_audio_lhdc_decoder_frame_t *)a2dp_audio_lhdc_frame_malloc(lhdc_frame_Info.frame_len);
                }else{
                    nRet = A2DP_DECODER_MTU_LIMTER_ERROR;
                    break;
                }
                frame_cnt++;
                decoder_frame_p->sequenceNumber = header->sequenceNumber;
                decoder_frame_p->timestamp = header->timestamp;
                decoder_frame_p->curSubSequenceNumber = frame_cnt;
                decoder_frame_p->totalSubSequenceNumber = frame_num;
                memcpy(decoder_frame_p->buffer, lPTR+ptr_offset, lhdc_frame_Info.frame_len);
                decoder_frame_p->buffer_len = lhdc_frame_Info.frame_len;
                a2dp_audio_list_append(list, decoder_frame_p);
                ptr_offset += lhdc_frame_Info.frame_len;
#if 0                
                TRACE("lhdc_store_packet save seq:%d timestamp:%d len:%d lSize:%d frame_len:%d Split:%d/%d", 
                                                                                                             header->sequenceNumber,
                                                                                                             header->timestamp,
                                                                                                             buffer_bytes,
                                                                                                             lSize,
                                                                                                             lhdc_frame_Info.frame_len,
                                                                                                             lhdc_frame_Info.isSplit, 
                                                                                                             lhdc_frame_Info.isLeft);
#endif
            }
        }
    }else{        
//        TRACE("lhdc_store_packet skip seq:%d timestamp:%d len:%d l:%d", header->sequenceNumber, header->timestamp,buffer_bytes, lSize);
    }

    return nRet;
}

int a2dp_audio_lhdc_discards_packet(uint32_t packets)
{
    int nRet = a2dp_audio_context_p->audio_decoder.audio_decoder_synchronize_dest_packet_mut(a2dp_audio_context_p->dest_packet_mut);

    reset_lhdc_assmeble_packet();

    return nRet;
}

static int a2dp_audio_lhdc_headframe_info_get(A2DP_AUDIO_HEADFRAME_INFO_T* headframe_info)
{
    list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
    list_node_t *node = NULL;
    a2dp_audio_lhdc_decoder_frame_t *decoder_frame_p = NULL;

    if (a2dp_audio_list_length(list)){
        node = a2dp_audio_list_begin(list);                
        decoder_frame_p = (a2dp_audio_lhdc_decoder_frame_t *)a2dp_audio_list_node(node);
        headframe_info->sequenceNumber = decoder_frame_p->sequenceNumber;
        headframe_info->timestamp = decoder_frame_p->timestamp;
        headframe_info->curSubSequenceNumber = 0;
        headframe_info->totalSubSequenceNumber = 0;
    }else{
        memset(headframe_info, 0, sizeof(A2DP_AUDIO_HEADFRAME_INFO_T));
    }

    return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_lhdc_info_get(void *info)
{
    return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_lhdc_init(A2DP_AUDIO_OUTPUT_CONFIG_T *config, void *context)
{   
    TRACE("%s %s ch:%d freq:%d bits:%d", __func__, getVersionCode(), 
                                                          config->num_channels,
                                                          config->sample_rate, 
                                                          config->bits_depth);

    a2dp_audio_context_p = (A2DP_AUDIO_CONTEXT_T *)context;

    memset(&a2dp_audio_lhdc_lastframe_info, 0, sizeof(A2DP_AUDIO_DECODER_LASTFRAME_INFO_T));    
    memcpy(&a2dp_audio_lhdc_output_config, config, sizeof(A2DP_AUDIO_OUTPUT_CONFIG_T));
    a2dp_audio_lhdc_lastframe_info.stream_info = a2dp_audio_lhdc_output_config;
    a2dp_audio_lhdc_lastframe_info.frame_samples = A2DP_LHDC_OUTPUT_FRAME_SAMPLES;
    a2dp_audio_lhdc_lastframe_info.list_samples = A2DP_LHDC_OUTPUT_FRAME_SAMPLES;
    a2dp_audio_decoder_internal_lastframe_info_set(&a2dp_audio_lhdc_lastframe_info);

    lhdcSetLicenseKeyTable((uint8_t *)testkey_bin, bes_bt_local_info_get);
    lhdcInit(config->bits_depth, config->sample_rate, 0);
    initial_lhdc_assemble_packet(false);    
    
    return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_lhdc_deinit(void)
{
    lhdcDestroy();
    return A2DP_DECODER_NO_ERROR;
}

int  a2dp_audio_lhdc_synchronize_packet(A2DP_AUDIO_SYNCFRAME_INFO_T *sync_info, uint32_t mask)
{
    return A2DP_DECODER_NOT_SUPPORT;
}


int a2dp_audio_lhdc_synchronize_dest_packet_mut(uint16_t packet_mut)
{
    list_node_t *node = NULL;
    uint32_t list_len = 0;
    list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
    a2dp_audio_lhdc_decoder_frame_t *lhdc_decoder_frame_p = NULL;

    list_len = a2dp_audio_list_length(list);
    if (list_len > packet_mut){
        do{        
            node = a2dp_audio_list_begin(list);            
            lhdc_decoder_frame_p = (a2dp_audio_lhdc_decoder_frame_t *)a2dp_audio_list_node(node);
            a2dp_audio_list_remove(list, lhdc_decoder_frame_p);
        }while(a2dp_audio_list_length(list) > packet_mut);
    }

    TRACE("%s list:%d", __func__, a2dp_audio_list_length(list));
    return A2DP_DECODER_NO_ERROR;
}


A2DP_AUDIO_DECODER_T a2dp_audio_lhdc_decoder_config = {
                                                        {96000, 2, 24},
                                                        0,
                                                        a2dp_audio_lhdc_init,
                                                        a2dp_audio_lhdc_deinit,
                                                        a2dp_audio_lhdc_decode_frame,
                                                        a2dp_audio_lhdc_preparse_packet,
                                                        a2dp_audio_lhdc_store_packet,
                                                        a2dp_audio_lhdc_discards_packet,
                                                        a2dp_audio_lhdc_synchronize_packet,
                                                        a2dp_audio_lhdc_synchronize_dest_packet_mut,
                                                        a2dp_audio_lhdc_headframe_info_get,
                                                        a2dp_audio_lhdc_info_get,
                                                        a2dp_audio_lhdc_free,
                                                     } ;

