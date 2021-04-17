#include "stdint.h"
#include "stdbool.h"
#include "plat_types.h"
#include "cmsis_os.h"
#include "cmsis_nvic.h"
#include "string.h"
#include "stdio.h"
#include "crc32.h"
#include "hal_trace.h"
#include "hal_iomux.h"
#include "hal_gpio.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "hal_iomux.h"
#include "btif_sys_config.h"
#include "communication_svr.h"
#include "communication_sysapi.h"
#include"app_ibrt_peripheral_manager.h"
#include "nvrecord_env.h"
#include "med_memory.h"
#include "../../apps/main/apps.h"

#include "app_ibrt_ui.h"

extern "C" int g_bt_access_mode;
extern "C" int open_siri_flag;
extern "C" const char *BT_LOCAL_NAME;
extern "C" uint8_t box_event;

extern uint8_t AT_get_bt_status(void);
extern "C" int AT_send_key_event(enum HAL_KEY_CODE_T code, enum HAL_KEY_EVENT_T event);
extern "C" bool is_app_bt_pairing_running(void);

extern "C" void app_bt_accessmode_set(btif_accessible_mode_t);
extern "C" int nv_record_get_paired_dev_count(void);
extern "C" bt_status_t nv_record_enum_dev_records(unsigned short index,btif_device_record_t* record);
extern "C" void osapi_lock_stack(void);
extern "C" void osapi_unlock_stack(void);
//======================================================================================================

enum COMMUNICATION_MSG {
    COMMUNICATION_MSG_TX_REQ   = 0,
    COMMUNICATION_MSG_TX_DONE  = 1,
    COMMUNICATION_MSG_RX_REQ   = 2,
    COMMUNICATION_MSG_RX_DONE  = 3,
    COMMUNICATION_MSG_INIT     = 4,
    COMMUNICATION_MSG_REINIT   = 5,    
    COMMUNICATION_MSG_RESET    = 6,
    COMMUNICATION_MSG_BREAK    = 7,
};

const static uint8_t communication_process_log[8][26] = {
    "COMMUNICATION_MSG_TX_REQ",
    "COMMUNICATION_MSG_TX_DONE",
    "COMMUNICATION_MSG_RX_REQ",
    "COMMUNICATION_MSG_RX_DONE",
    "COMMUNICATION_MSG_INIT",
    "COMMUNICATION_MSG_REINIT",
    "COMMUNICATION_MSG_RESET",
    "COMMUNICATION_MSG_BREAK",
};

enum COMMUNICATION_MODE {
    COMMUNICATION_MODE_NULL,
    COMMUNICATION_MODE_TX,
    COMMUNICATION_MODE_RX,
    COMMUNICATION_MODE_ENABLE_IRQ,
    COMMUNICATION_MODE_DISABLE_IRQ,
};


#define COMMAND_BLOCK_MAX (5)  
#define COMMAND_LEN_MAX (128)  
#define COMMAND_TRANSMITTED_SIGNAL (1<<0)

typedef struct {
    uint8_t cmd_buf[COMMAND_LEN_MAX];
    uint8_t cmd_len;
} COMMAND_BLOCK;
osPoolDef (command_mempool, COMMAND_BLOCK_MAX, COMMAND_BLOCK);
osPoolId   command_mempool = NULL;


#define COMMUNICATION_MAILBOX_MAX (10)
typedef struct {
    uint32_t src_thread;
    uint32_t system_time;    
    uint32_t message;
    uint32_t parms1;
    uint32_t parms2;
} COMMUNICATION_MAIL;

osMailQDef (communication_mailbox, COMMUNICATION_MAILBOX_MAX, COMMUNICATION_MAIL);
static osMailQId communication_mailbox = NULL;
static uint8_t communication_mailbox_cnt = 0;


static osThreadId communication_tid = NULL;
static void communication_thread(void const *argument);
osThreadDef(communication_thread, osPriorityHigh, 1, 2048, "communication_server");

static bool uart_opened = false;
static int uart_error_detected = 0;

#ifdef CHIP_BEST2300P
static const enum HAL_UART_ID_T comm_uart = HAL_UART_ID_2;
#else
static const enum HAL_UART_ID_T comm_uart = HAL_UART_ID_1;
#endif

static const struct HAL_UART_CFG_T uart_cfg = {
    HAL_UART_PARITY_NONE,
    HAL_UART_STOP_BITS_1,
    HAL_UART_DATA_BITS_8,
    HAL_UART_FLOW_CONTROL_NONE,
    HAL_UART_FIFO_LEVEL_1_2,
    HAL_UART_FIFO_LEVEL_1_2,
#ifdef CHIP_BEST2300P
    1152000,
#else
    9600,//921600,
#endif
    true,
    true,
    false,
};

static bool uart_rx_dma_is_running = false;

static COMMAND_BLOCK *rx_command_block_p = NULL;

static uint32_t uart_rx_idle_counter = 0;
static void uart_rx_idle_handler(void const *param);
osTimerDef(uart_rx_idle_timer, uart_rx_idle_handler);
static osTimerId uart_rx_idle_timer_id = NULL;


static communication_receive_func_typedef communication_receive_cb = NULL;


const static struct HAL_IOMUX_PIN_FUNCTION_MAP cfg_led2 = {
    HAL_IOMUX_PIN_LED2, HAL_IOMUX_FUNC_GPIO, HAL_IOMUX_PIN_VOLTAGE_VBAT, HAL_IOMUX_PIN_PULLUP_ENALBE,
};


inline int communication_mailbox_put(COMMUNICATION_MAIL* msg_src);
static void communication_process(COMMUNICATION_MAIL* mail_p);
int communication_io_mode_switch(enum COMMUNICATION_MODE mode);


static void uart_rx_dma_stop(void)
{
    union HAL_UART_IRQ_T mask;
    
    uint32_t lock = int_lock();
//    TRACE("uart_rx_dma_stop:%d", uart_rx_dma_is_running);
    if (uart_rx_dma_is_running){        
        mask.reg = 0;
        hal_uart_irq_set_mask(comm_uart, mask);
        hal_uart_stop_dma_recv(comm_uart);
        uart_rx_dma_is_running = false;
    }
    
    int_unlock(lock);
}

static void uart_rx_dma_start(void)
{
    union HAL_UART_IRQ_T mask;
    
    uint32_t lock = int_lock();

//    TRACE("uart_rx_dma_start:%d", uart_rx_dma_is_running);

    hal_uart_flush(comm_uart, 0);
    hal_uart_dma_recv(comm_uart, rx_command_block_p->cmd_buf, COMMAND_LEN_MAX, NULL, NULL);     
    mask.reg = 0;
    mask.RT = 1;
    hal_uart_irq_set_mask(comm_uart, mask);
    uart_rx_dma_is_running = true;

    int_unlock(lock);
}

static void uart_break_handler(void)
{
    union HAL_UART_IRQ_T mask;
    COMMUNICATION_MAIL msg;
    memset(&msg, 0, sizeof(msg));

    TRACE("UART-BREAK");
    mask.reg = 0;           
    hal_uart_irq_set_mask(comm_uart, mask);
    hal_uart_flush(comm_uart, 0);
    uart_error_detected = 1;
    msg.message = COMMUNICATION_MSG_BREAK;
    communication_mailbox_put(&msg);
}

static void uart_rx_dma_handler(uint32_t xfer_size, int dma_error, union HAL_UART_IRQ_T status)
{
    COMMUNICATION_MAIL msg;

    //TRACE("UART-RX size:%d dma_error=%d, status=0x%08x rt:%d fe:%d pe:%d be:%d oe:%d", xfer_size, dma_error, status, status.RT, status.FE, status.PE, status.BE, status.OE);

    memset(&msg, 0, sizeof(COMMUNICATION_MAIL));
    msg.message = COMMUNICATION_MSG_RX_DONE;

    if (dma_error || status.FE|| status.PE || status.BE || status.OE ) {
        uart_error_detected = 1;
        uart_rx_dma_stop();
        msg.parms1 = false;        
        msg.message = COMMUNICATION_MSG_RESET;
        communication_mailbox_put(&msg);
    }else{    
        //if (xfer_size){
		if (xfer_size > 4){
            msg.parms1 = true;            
            msg.parms2 = xfer_size;
            communication_mailbox_put(&msg);
        }else{
            uart_rx_dma_stop();
            msg.message = COMMUNICATION_MSG_RESET;
            communication_mailbox_put(&msg);
        }
    }    
}

static void uart_tx_dma_handler(uint32_t xfer_size, int dma_error)
{
    COMMUNICATION_MAIL msg;

    memset(&msg, 0, sizeof(COMMUNICATION_MAIL));

//    TRACE("UART-TX size:%d dma_error=%d", xfer_size, dma_error);
    
    osSignalSet(communication_tid, COMMAND_TRANSMITTED_SIGNAL);

    msg.message = COMMUNICATION_MSG_TX_DONE;

    communication_mailbox_put(&msg);
}

static void uart_init(void)
{
    struct HAL_UART_CFG_T comm_uart_cfg;

    if (!uart_opened) {
        memcpy(&comm_uart_cfg, &uart_cfg, sizeof(comm_uart_cfg));
        hal_uart_open(comm_uart, &comm_uart_cfg);
        hal_uart_irq_set_dma_handler(comm_uart, uart_rx_dma_handler, uart_tx_dma_handler, uart_break_handler);
        uart_opened = true;
    }

    hal_uart_flush(comm_uart, 0);
}

static void uart_deinit(void)
{
    if (uart_opened) {
        hal_uart_close(comm_uart);
        uart_opened = false;
    }
}

static void uart_rx_idle_timer_start(void)
{
    uart_rx_idle_counter = 0;
    TRACE("[%s] enter...", __func__);
    
    if (uart_rx_idle_timer_id != NULL) {
        osTimerStop(uart_rx_idle_timer_id);
        osTimerStart(uart_rx_idle_timer_id, 100);
    }
}

#if defined(CHIP_BEST1400) || defined(CHIP_BEST1402)
static void uart_rx_edge_detect_handler(enum HAL_GPIO_PIN_T pin)
{
    COMMUNICATION_MAIL msg = {0};

    //disable led2 pin external interrupt mode...
    communication_io_mode_switch(COMMUNICATION_MODE_DISABLE_IRQ);

    //post uart rx request...
    msg.message = COMMUNICATION_MSG_RX_REQ;
    communication_mailbox_put(&msg);
}
#endif

extern bool plug_inout_flag;
extern const struct HAL_IOMUX_PIN_FUNCTION_MAP app_battery_ext_charger_indicator_cfg;

static void uart_rx_idle_handler(void const *param)
{
    // const uint8_t rx_idle_indicate[2] = {0x10,0x24};

    if(uart_rx_idle_counter++ >= 150)//150 * 100 = 15s
    {
        TRACE("[%s] enter...", __func__);
        
        uart_rx_idle_counter = 0;
        
        //stop virtual timer...
        osTimerStop(uart_rx_idle_timer_id);
        
        //stop uart rx dma...
        uart_rx_dma_stop();
        
        //notify charge box uart rx dma has closed...
        // communication_io_mode_switch(COMMUNICATION_MODE_TX);
        // hal_uart_dma_send(comm_uart, rx_idle_indicate, 2, NULL, NULL);
        // osDelay(10);
		
        //config led2 pin as external interrupt mode...
        communication_io_mode_switch(COMMUNICATION_MODE_ENABLE_IRQ);
		
    }
}


int communication_io_mode_switch(enum COMMUNICATION_MODE mode)
{
    //best1400 and best1402 platform
    switch (mode)
    {
        case COMMUNICATION_MODE_TX: {
        #if defined(CHIP_BEST1400) || defined(CHIP_BEST1402)
            hal_iomux_single_wire_uart1_tx();
        #endif
        }
        break;
        case COMMUNICATION_MODE_RX: {
        #if defined(CHIP_BEST1400) || defined(CHIP_BEST1402)
            hal_iomux_single_wire_uart1_rx();
        #endif
        }
        break;
        case COMMUNICATION_MODE_ENABLE_IRQ: {
        #if defined(CHIP_BEST1400) || defined(CHIP_BEST1402)
            hal_iomux_single_wire_uart1_enable_irq(uart_rx_edge_detect_handler);
        #endif
        }
        break;
        case COMMUNICATION_MODE_DISABLE_IRQ: {
        #if defined(CHIP_BEST1400) || defined(CHIP_BEST1402)
            hal_iomux_single_wire_uart1_disable_irq();
        #endif
        }
        break;
        default:break;
    }
    return 0;
}

int communication_io_mode_init(void)
{
    communication_io_mode_switch(COMMUNICATION_MODE_RX);
    
    return 0;
}

inline int communication_mailbox_put(COMMUNICATION_MAIL* msg_src)
{
    osStatus status;

    COMMUNICATION_MAIL *mail_p = NULL;

    if (msg_src == NULL)
    {
        return -1;
    }

    mail_p = (COMMUNICATION_MAIL*)osMailAlloc(communication_mailbox, 0);
    if (!mail_p){
        osEvent evt;
        TRACE("communication_mailbox");
        for (uint8_t i=0; i<COMMUNICATION_MAILBOX_MAX; i++){
            evt = osMailGet(communication_mailbox, 0);
            if (evt.status == osEventMail) {
                TRACE("msg cnt:%d msg:%d parms:%08x/%08x", i,
                                                              ((COMMUNICATION_MAIL *)(evt.value.p))->message,
                                                              ((COMMUNICATION_MAIL *)(evt.value.p))->parms1,
                                                              ((COMMUNICATION_MAIL *)(evt.value.p))->parms2);
            }else{
                break;
            }
        }
        ASSERT(mail_p, "communication_mailbox error");
    }
    mail_p->src_thread = (uint32_t)osThreadGetId();
    mail_p->system_time = hal_sys_timer_get();
    mail_p->message = msg_src->message;
    mail_p->parms1 = msg_src->parms1;
    mail_p->parms2 = msg_src->parms2;
    status = osMailPut(communication_mailbox, mail_p);
    if (osOK == status){
        communication_mailbox_cnt++;
    }

    return (int)status;
}

inline int communication_mailbox_free(COMMUNICATION_MAIL* mail_p)
{
    osStatus status;

    status = osMailFree(communication_mailbox, mail_p);
    if (osOK == status){
        communication_mailbox_cnt--;
    }
    return (int)status;
}

inline int communication_mailbox_get(COMMUNICATION_MAIL** mail_p)
{
    osEvent evt;
    evt = osMailGet(communication_mailbox, osWaitForever);
    if (evt.status == osEventMail) {
        *mail_p = (COMMUNICATION_MAIL *)evt.value.p;
        return 0;
    }
    return -1;
}

static void communication_thread(void const *argument)
{
    while(1){
        COMMUNICATION_MAIL *mail_p = NULL;
        if (!communication_mailbox_get(&mail_p)){
            communication_process(mail_p);
            communication_mailbox_free(mail_p);
        }
    }
}

int communication_command_block_alloc(COMMAND_BLOCK **cmd_blk)
{
    *cmd_blk = (COMMAND_BLOCK *)osPoolCAlloc (command_mempool);
    ASSERT(*cmd_blk,  "%s error", __func__);
    return 0;
}

void communication_command_block_free(COMMAND_BLOCK *cmd_blk)
{
    osPoolFree(command_mempool, cmd_blk);
}

void communication_send_command(COMMAND_BLOCK *cmd_blk)
{
    COMMUNICATION_MAIL mail;
    memset(&mail, 0, sizeof(mail));
    ASSERT(cmd_blk->cmd_len <=COMMAND_LEN_MAX, "%s len error", __func__);
    mail.message = COMMUNICATION_MSG_TX_REQ;
    mail.parms2 = (uint32_t)cmd_blk;
    communication_mailbox_put(&mail);
}

extern void start_plug_inout_timer(int32_t ms);
extern void stop_plug_inout_timer(void);
extern void app_ibrt_simulate_charger_plug_in_test(void);
extern void app_ibrt_simulate_charger_plug_out_test(void);


#define MSG_MAX_REQ_CNT_MAX 4
static uint8_t MSG_RX_REQ_CNT;

static void communication_process(COMMUNICATION_MAIL* mail_p)
{
    osEvent evt;
    COMMAND_BLOCK *command_block_p;
    COMMUNICATION_MAIL msg;
    uint32_t lock;
	
    TRACE("[%s]: %s", __func__, communication_process_log[mail_p->message]);
    
    memset(&msg, 0, sizeof(COMMUNICATION_MAIL));
    switch (mail_p->message) {
        case COMMUNICATION_MSG_TX_REQ:
			stop_plug_inout_timer();
	
            command_block_p = (COMMAND_BLOCK *)mail_p->parms2;           
            TRACE("UART TX:%d", command_block_p->cmd_len);
            DUMP8("%02x ", command_block_p->cmd_buf, command_block_p->cmd_len);
            uart_rx_dma_stop();
            communication_io_mode_switch(COMMUNICATION_MODE_TX);
            osSignalClear(communication_tid, COMMAND_TRANSMITTED_SIGNAL);          
            hal_uart_dma_send(comm_uart, command_block_p->cmd_buf, command_block_p->cmd_len, NULL, NULL);
            evt = osSignalWait(COMMAND_TRANSMITTED_SIGNAL, 1000);
            if (evt.status== osEventTimeout){
                ASSERT(0, "%s osEventTimeout", __func__);
            }
            communication_command_block_free(command_block_p);
            while (!hal_uart_get_flag(comm_uart).TXFE || hal_uart_get_flag(comm_uart).BUSY){
                osThreadYield();
            }
            communication_io_mode_switch(COMMUNICATION_MODE_RX);
            if (!uart_error_detected){
                uart_rx_dma_start();
                uart_rx_idle_timer_start();
            }
			
			start_plug_inout_timer(500);
            break;
        case COMMUNICATION_MSG_TX_DONE:
			stop_plug_inout_timer();
			start_plug_inout_timer(500);
            break;
        case COMMUNICATION_MSG_RX_REQ:
			MSG_RX_REQ_CNT++;
			if(MSG_RX_REQ_CNT>MSG_MAX_REQ_CNT_MAX){
				return;
			}
			stop_plug_inout_timer();
			
            communication_io_mode_init();
            uart_init();
            uart_rx_dma_start();
            uart_rx_idle_timer_start();

			start_plug_inout_timer(500);
            break;
        case COMMUNICATION_MSG_RX_DONE:
			stop_plug_inout_timer();
			
			MSG_RX_REQ_CNT = 0;
			if(!plug_inout_flag){
				plug_inout_flag = true;
				cosonic_trace(plug_in,true,"the earphone plug in");
				app_ibrt_simulate_charger_plug_in_test();
			}
			
            TRACE("UART RX status:%d len:%d", mail_p->parms1, mail_p->parms2);
            DUMP8("%02x ", rx_command_block_p->cmd_buf, mail_p->parms2);
            TRACE("%s", rx_command_block_p->cmd_buf);
			
            if(communication_receive_cb != NULL)
                (*communication_receive_cb)(rx_command_block_p->cmd_buf, mail_p->parms2);
            
            memset(rx_command_block_p->cmd_buf, 0, mail_p->parms2);
            
            if (!uart_error_detected){
                uart_rx_dma_start();
                uart_rx_idle_timer_start();
            }
			
			start_plug_inout_timer(500);
            break;
        case COMMUNICATION_MSG_REINIT:            
            uart_deinit();
        case COMMUNICATION_MSG_INIT:
            uart_error_detected = 0;
            communication_io_mode_init();
            uart_init();            
            uart_rx_dma_start();
            uart_rx_idle_timer_id = osTimerCreate(osTimer(uart_rx_idle_timer), osTimerPeriodic, NULL);
            uart_rx_idle_timer_start();
            break;
        case COMMUNICATION_MSG_RESET:
        case COMMUNICATION_MSG_BREAK:
			stop_plug_inout_timer();
		
            lock = int_lock();
            uart_rx_dma_stop();
            communication_io_mode_switch(COMMUNICATION_MODE_RX);
            hal_uart_flush(comm_uart, 0);
            uart_rx_dma_start();
            uart_error_detected = 0;            
            int_unlock(lock);
            uart_rx_idle_timer_start();
			
			start_plug_inout_timer(500);
			break;

        default:
            break;
    }

}

void communication_init(void)
{
    COMMUNICATION_MAIL msg;
    //COMMAND_BLOCK *cmd_blk;

    memset(&msg, 0, sizeof(COMMUNICATION_MAIL));
    TRACE("%s", __func__);

    if (command_mempool == NULL){
        command_mempool = osPoolCreate(osPool(command_mempool));
    }

    if (communication_mailbox == NULL){
        communication_mailbox = osMailCreate(osMailQ(communication_mailbox), NULL);
    }
    
    if (communication_tid == NULL){
        communication_tid = osThreadCreate(osThread(communication_thread), NULL);
    }

    if (rx_command_block_p == NULL){
        communication_command_block_alloc(&rx_command_block_p);
        memset(rx_command_block_p->cmd_buf, 0, COMMAND_LEN_MAX);
        rx_command_block_p->cmd_len = 0;
    }

    msg.message = COMMUNICATION_MSG_INIT;
    communication_mailbox_put(&msg);

    /*communication_command_block_alloc(&cmd_blk);
    cmd_blk->cmd_buf[0] = 0xff;
    cmd_blk->cmd_len = 1;
    communication_send_command(cmd_blk);*/
}

int communication_receive_register_callback(communication_receive_func_typedef p)
{
    if(p == NULL)
        return -1;
    
    communication_receive_cb = p;
    
    TRACE("[%s] register receive callback success\n", __func__);
    
    return 0;
}

int communication_send_buf(uint8_t * buf, uint8_t len)
{
    COMMAND_BLOCK *cmd_blk;
    
    communication_command_block_alloc(&cmd_blk);
    memcpy(cmd_blk->cmd_buf, buf, len);
    cmd_blk->cmd_len = len;
    communication_send_command(cmd_blk);
    return 0;
}

#define SINGLE_COM
#ifdef SINGLE_COM
#if 0
static uint8_t crc8_maxim(uint8_t *buff, uint8_t length)
{
    uint8_t i;
    uint8_t crc= 0;

    while (length--)
    {
        crc ^= *buff++;

        for (i = 0; i < 8; i++)
        {
            if (crc & 1)
            {
                crc = (crc >> 1)^0x8C;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
	TRACE("CRC: %d",crc);
    return crc;
}
#else

static uint16_t crc16_maxim(uint8_t *buff, uint8_t length,bool net)//net =1 �������CRC
{
	uint16_t crc = 0;
	uint16_t crc1 = 0;
	uint8_t i;
	length=length - 2;
	
	
	while (length--) 	
	{
		crc ^= *buff++;
		for( i = 0;i < 8;++i)
		{
			if(crc & 1)
				crc = (crc >> 1) ^0xA001;
			else
				crc = (crc >> 1);
		}
	}
	//TRACE("CRC: %x",crc);	
	crc=~crc;
	//TRACE("buff:%x\n",*buff);
	crc1 = (((*buff++)&0x00ff)<<8);
	crc1 = crc1+*buff++;
	TRACE("CRC: %x CRC1:%x ",crc,crc1);
	if(net == 0&&crc1 == crc)
		{
			return 0;
		}
	else 
		{
			return crc;
		}
}
#endif

/*static void crc16_maxim_send(uint8_t length,single_st *single_data,uint8_t data)
{
uint8_t test[6]= {0xbb, 0x00, 0x00, 0x00, 0x00};
//memcpy(single_data, buff, length);
single_data->data = &data;
test[1] = 2 + sizeof(*single_data->data);
test[2] = single_data->cmd;
//test[3] = single_data->which;
test[3] = *single_data->data;
//memcpy(, *single_data->data, sizeof(*single_data->data));

test[4] = ((crc16_maxim(test, sizeof(test), 1)&0xff00)	>> 8);
test[5] = crc16_maxim(test, sizeof(test), 1);
communication_send_buf(test, sizeof(test));

}*/



/*void single_uart_data_packet_send(uint8_t cmd,uint8_t ear_type,uint8_t *payload,uint16_t len)
{
   uint8_t total_len = 0;
   uint8_t *packet = NULL;
   uint16_t crc16val = 0;
	if(NULL == payload || len > 128)
	{
		TRACE("DATA PARAM INVALID !!!");
		return;
	}

	total_len = len + 6;
	packet = (uint8_t *)malloc(total_len);
	if(NULL == packet)
	{
		TRACE("MALLOC ERR !!!");
		return;
	}
	
	packet[0] = 0xBB;
	packet[1] = len + 2;
	packet[2] = cmd;
	packet[3] = ear_type;
	memcpy(packet + 4,payload,len);
	crc16val = crc16_maxim(packet,total_len,1);
	packet[total_len-2] = (crc16val>>8);
	packet[total_len-1] = (crc16val & 0xFF);
	communication_send_buf(packet,total_len);
	free(packet);
}*/

#if 0
void single_uart_receive_data_process(uint8_t *buf, uint8_t len)
{
    uint8_t i = 0;
    uint8_t buff[7] = {0xaa, 0x00, 0x00, 0x00};
    single_st *single_data = NULL;

    // search head
    if (len < 7)
    {
        TRACE("error len!!!");
        return;
    }
    else
    {
        do
        {
            // TRACE("buf[i] %xbuf[i+1] %x", buf[i], buf[i+1]);
            if ((buf[i] == 0xaa)) //&& (buf[i + 1] == 0xAA))
            {
                // single_data = (single_st *)buf;
                if (single_data == NULL)
                {
                    TRACE("find header %d, %d!!!", i, len);
                    //single_data = (single_st *)malloc(len);
                    single_data = (single_st *)malloc(sizeof(single_st));
                }
                memcpy(single_data, buf, len);

                break;
            }
            else
            {
                len--;
                buf++;
            }
        } while(len >= 7); //packet min size is 6 bytes  
    }

    TRACE("%s", __func__);
    DUMP8("%02x", buf, len);
	TRACE("%x",single_data->data);
#if 0
    if (crc8_maxim(buf, len))
    {
        TRACE("crc8_maxim failed");
    }
#else

	if (crc16_maxim(buf,len,0))
	{
		TRACE("crc16_maxim failed");
	}
#endif	
    else
    {
        switch (single_data->cmd)
        {
        case OPEN_BOX:
        {
            TRACE("THIS IS OPEN BOX COMMAND %x %x", single_data->cmd, single_data->which);
            app_ibrt_peripheral_manage_entry((app_ibrt_peripheral_manager_cmd_code_e)OPEN_BOX, NULL, 0);                
            buff[2] = single_data->cmd;
            buff[3] = single_data->which;
            buff[5] = crc16_maxim(buff, 5, 0);
            communication_send_buf(buff, sizeof(buff));
        }
            break;
        case CLOSE_BOX:
        {
            TRACE("THIS IS CLOSE BOX COMMAND");
            app_ibrt_peripheral_manage_entry((app_ibrt_peripheral_manager_cmd_code_e)CLOSE_BOX, NULL, 0);
            buff[2] = single_data->cmd;
            buff[3] = single_data->which;
            buff[5] = crc16_maxim(buff, 5, 0);
            communication_send_buf(buff, sizeof(buff)); 
        }
            break;
        case RESET_EAR:
            app_ibrt_peripheral_manage_entry((app_ibrt_peripheral_manager_cmd_code_e)RESET_EAR, NULL, 0);
            break;
        case SHUTDOWN_EAR:
        {
            TRACE("THIS IS shutdown COMMAND");
            buff[2] = single_data->cmd;
            buff[3] = single_data->which;
            buff[5] = crc16_maxim(buff, 5, 0);
            communication_send_buf(buff, sizeof(buff));
            app_ibrt_peripheral_manage_entry((app_ibrt_peripheral_manager_cmd_code_e)SHUTDOWN_EAR, NULL, 0);
        }
            break;
        case TWS_PAIRING:
            app_ibrt_peripheral_manage_entry((app_ibrt_peripheral_manager_cmd_code_e)TWS_PAIRING, (uint8_t *)single_data, len);
            break;
        case FREEMAN_MODE:
            app_ibrt_peripheral_manage_entry((app_ibrt_peripheral_manager_cmd_code_e)FREEMAN_MODE, NULL, 0);
            break;
        case GET_EAR_INFO:
            TRACE("GET EAR INFORMATION!!! no function");
            break;
        case EXCHANGE_PWR:
        {
            app_ibrt_peripheral_manage_entry((app_ibrt_peripheral_manager_cmd_code_e)EXCHANGE_PWR, (uint8_t *)single_data, len);
        }
            break;
        case CLEAR_PHONE_INFO:
            app_ibrt_peripheral_manage_entry((app_ibrt_peripheral_manager_cmd_code_e)CLEAR_PHONE_INFO, NULL, 0);
            break;
        case RESET_FACTORY:
            app_ibrt_peripheral_manage_entry((app_ibrt_peripheral_manager_cmd_code_e)RESET_FACTORY, NULL, 0);
            break;
        case GET_EAR_MAC:
            TRACE("GET EAR MAC");
            app_ibrt_peripheral_manage_entry((app_ibrt_peripheral_manager_cmd_code_e)GET_EAR_MAC, (uint8_t *)single_data, len);
            break;
        case SET_EAR_MAC:
            TRACE("SET EAR MAC");
            app_ibrt_peripheral_manage_entry((app_ibrt_peripheral_manager_cmd_code_e)SET_EAR_MAC, (uint8_t *)single_data, len);
            break;

        default:
            TRACE("INVALID COMMAND");
			uint8_t name = 0x74;
			//TRACE("single:%x  name:%d",*single_data->data,name );
			crc16_maxim_send(len,single_data,name);
#if 0				

			test[5] = ((crc16_maxim(test, sizeof(test), 1)&0xff00)  >> 8);
			test[6] = crc16_maxim(test, sizeof(test), 1);
			communication_send_buf(test, sizeof(test));
#endif			
            break;
        }
    }

    if (single_data)
    {
        free(single_data);
    }
}
#else
//岍丞充电盒通讯格式,可修改
typedef struct{
	uint8_t header[2];
	uint8_t check;
	uint8_t side;
	uint8_t cmd;
	uint8_t r_w_flag;//读写标志
	uint8_t data_length;
	uint8_t data;
}box_communication_data_format;

void parse_box_communication_data(box_communication_data_format *data,uint8_t *buf)
{
	data->header[0]	= buf[0];
	data->header[1]	= buf[1];
	data->check		= buf[2];
	data->side			= buf[3];
	data->cmd			= buf[4];
	data->r_w_flag		= buf[5];
	data->data_length	= buf[6];
	if(data->data_length){
		data->data			= buf[7];
	}
}

bool check_box_data(uint8_t *buf)
{
	uint8_t check = buf[2];
	uint8_t data_length = buf[6];
	int sum=0;
	for(uint8_t i=3;i<(data_length+7);i++){
		sum += buf[i];
	}
	TRACE("sum=%x",sum);
	return (check == sum)?true:false;
}
bool cmd_flag = false;

extern uint8_t LR_flags;
extern uint8_t bt_addr[6];
void single_uart_send_data_process(box_communication_data_format *data,uint8_t *buf,uint8_t buff_len)
{
	uint8_t *single_uart_send_buf;
	single_uart_send_buf =  (uint8_t *)malloc(buff_len+7);
	memset(single_uart_send_buf,0,buff_len+7);
	single_uart_send_buf[0]	= data->header[1];
	single_uart_send_buf[1]	= data->header[0];
	if(data->side == 0x02){
		single_uart_send_buf[3] = LR_flags;
	}else{
		single_uart_send_buf[3] = data->side;
	}
	single_uart_send_buf[4]	= data->cmd;
	single_uart_send_buf[5] = data->r_w_flag;
	single_uart_send_buf[6] = buff_len;
	if(buff_len){
		memcpy(single_uart_send_buf+7,buf,buff_len);
	}
	for(uint8_t i=3;i<buff_len+7;i++){
		single_uart_send_buf[2] += single_uart_send_buf[i];
	}
	communication_send_buf(single_uart_send_buf,buff_len+7);
	free(single_uart_send_buf);
}

extern int AT_app_bt_stream_volumeset(int8_t vol);
static bool enter_key_test_mode = false;
extern void app_factorymode_enter(void);
extern uint16_t AT_app_battery_get_current_volt(void);
extern void app_ibrt_remove_history_paired_device(void);
extern void app_ibrt_remove_phone_paired_device(void);
void box_command_handler(uint8_t *buf);
void factory_command_handler(uint8_t *buf);
void AT_command_handler(uint8_t *buf);

void single_uart_receive_data_process(uint8_t *buf, uint8_t len)
{
    single_st *single_data = NULL;
	//enum USER_CMD_KEY_TYPE cmd_type = KEY_TYPE_NONE;
    // search head
    
	TRACE("%s", __func__);
	DUMP8("%02x", buf, len);
	
    if (len < 4 || len > 128 \
		||((buf[0] != 0x04 && buf[1] != 0xFF && !check_box_data(buf))//产测
		&&(buf[0] != 0x41 && buf[1] != 0x54 && buf[2] != 0x5E)//自动化
		&&(buf[0] != 0x05 && buf[1] != 0x0A && !check_box_data(buf)))//充电盒指令
		)
    {
        TRACE("INVALID PACKET DATA:buf[0]:%d,len: %d!!!",buf[0],len);
        return;
    }
	/*cmd_type = buf[0] == 0x55 ? KEY_TYPE_1 : KEY_TYPE_2;
	cmd_flag = cmd_type == KEY_TYPE_1 ? true : false;
	invalid_len = buf[1]+4;
	cmd = buf[1];
	if (crc16_maxim(buf,len,0))
	{
		TRACE("RCV UART CRC16_maxim failed");
	}

	if(cmd_type == KEY_TYPE_2 && cmd_flag == true )
	{
		TRACE("%s : box cmd has not been completed ", __func__);
		return;
	}*/

	if(buf[0] == 0x41 && buf[1] ==0x54 && buf[2] == 0x5E)
	{
		AT_command_handler(buf);
		osDelay(20);
		return;
	}
    if(buf[0] == 0x04 && buf[1] == 0xFF)
	{
		factory_command_handler(buf);
		osDelay(20);
		return;
    }
	if(buf[0] == 0x05 && buf[1] == 0x0A)
	{
		box_command_handler(buf);
		osDelay(20);
		return;
    }
    if (single_data)
    {
        free(single_data);
    }
}
#endif

int app_ibrt_single_communication_response(uint8_t *buffer, uint32_t length)
{
    TRACE("%s!!!", __func__);
    int retval = -1;
    uint8_t buff[12] = {0x55, 0xaa, 0x00, 0x00, 0x00};
    uint8_t buff_len = 0;
    uint8_t command_id = buffer[1];
    buff[3] = buffer[0];
    DUMP8("%02x", buffer, length);
    
    free(buffer);

    switch (command_id)
    {
    case APP_IBRT_PERIPHERAL_MANAGER_TWS_EXC_BATT_CMD:
        buff[2] = APP_IBRT_PERIPHERAL_MANAGER_TWS_EXC_BATT_CMD;
        buff[4] = 1;
        buff[5] = buffer[4] * 10;
        buff[6] = crc16_maxim(buff, 6, 0);
        buff_len = 7;
        retval = 0;
        break;
    
    case APP_IBRT_PERIPHERAL_MANAGER_TWS_GET_BTADDRESS_CMD:
        buff[2] = APP_IBRT_PERIPHERAL_MANAGER_TWS_GET_BTADDRESS_CMD;
        buff[4] = 6;
        memcpy(&buff[5], &buffer[2], 6);
        buff[11] = crc16_maxim(buff, 11, 0);
        buff_len = 12;
        retval = 0;
        break;

    case APP_IBRT_PERIPHERAL_MANAGER_TWS_SET_PEER_BTADDRESS_CMD:
        buff[2] = APP_IBRT_PERIPHERAL_MANAGER_TWS_SET_PEER_BTADDRESS_CMD;
        buff[4] = 0x01;
        buff[5] = 0x01;
        buff[6] = crc16_maxim(buff, 6, 0);
        buff_len = 7;
        retval = 0;
        break;
    
    default:
		retval = 0;
        break;
    }

    // DUMP8("%02x", buff, buff_len);
    communication_send_buf(buff, buff_len);

    return retval;
}
#endif

#ifdef KNOWLES_UART_DATA
#include "knowles_uart.h"

void uart_audio_init()
{
    TRACE("%s run!!!", __func__);
    init_transport();
}

void uart_audio_deinit()
{
    deinit_transport();
    reopen_uart();
}

#endif
void open_close_box(uint8_t box_status);
void box_command_handler(uint8_t *buf)
{
	box_communication_data_format box_data;
	parse_box_communication_data(&box_data,buf);
	if(box_data.side != 0x02){
		if(((box_data.side == 0x01) ^ (LR_flags == 0))//this command is sent to R_EARPHONE
		||((box_data.side == 0x00) ^ (LR_flags == 1))){//this command is sent to L_EARPHONE
			TRACE("this command is not sent to earphone");
			return;
		}
	}
	
	switch (box_data.cmd)
	{
		case OPEN_BOX:{
			TRACE("this command is OPEN_BOX:%x",box_data.cmd);
			TRACE("The box current volt is %d", box_data.data);
			open_close_box(IBRT_OPEN_BOX_EVENT);
			single_uart_send_data_process(&box_data,NULL,0);
			TRACE("send complete");
		}break;
		
		case CLOSE_BOX:{
			TRACE("this command is CLOSE_BOX:%x",box_data.cmd);
			TRACE("The box current volt is %d", box_data.data);
			open_close_box(IBRT_CLOSE_BOX_EVENT);
			single_uart_send_data_process(&box_data,NULL,0);
			TRACE("send complete");
		}break;
		
		case GET_BATTERY_VOLT:{
			TRACE("this command is GET_BATTERY_VOLT:%x",box_data.cmd);
			TRACE("The box current volt is %d", box_data.data);
			box_data.data = 0x01;
			single_uart_send_data_process(&box_data,&box_data.data,(uint8_t)sizeof(box_data.data));
			TRACE("send complete");
		}break;
		
		case SHUTDOWM_EAR:{
			TRACE("this command is SHUTDOWM_EAR:%x",box_data.cmd);
			single_uart_send_data_process(&box_data,NULL,0);
			TRACE("send complete");
		}break;
		
		case GET_SOFT_VERSION:{
			TRACE("this command is GET_SOFT_VERSION:%x",box_data.cmd);
			single_uart_send_data_process(&box_data,(uint8_t *)firmware_version,(uint8_t)strlen(firmware_version));
			TRACE("send complete");	
		}break;
		
		default:{
		}break;
	}
}

extern void factory_cmd_enter_tws_pairing(void);
void factory_command_handler(uint8_t *buf)
{
    box_communication_data_format box_data;
	parse_box_communication_data(&box_data,buf);
	if(box_data.side != 0x02){
		if(((box_data.side == 0x01) ^ (LR_flags == 0))//this command is sent to R_EARPHONE
			||((box_data.side == 0x00) ^ (LR_flags == 1))){//this command is sent to L_EARPHONE
			TRACE("this command is not sent to earphone");
			return;
		}
	}
	
	switch (box_data.cmd)
	{
		case FACTORY_GET_SOFT_VERSION:{
			TRACE("this command is GET_SOFT_VERSION:%x",box_data.cmd);
			single_uart_send_data_process(&box_data,(uint8_t *)firmware_version,(uint8_t)strlen(firmware_version));
			TRACE("send complete");
		}break;
		
		case FACTORY_GET_BT_ADDRESS:{
			TRACE("this command is GET_BT_ADDRESS:%x",box_data.cmd);
			single_uart_send_data_process(&box_data,bt_addr,(uint8_t)sizeof(bt_addr));
			TRACE("send complete");
		}break;
		
		case FACTORY_SET_VOLUME:{
			TRACE("this command is SET_VOLUME:%x",box_data.cmd);
			//app_bt_stream_volumeset(box_data.data);
			 AT_app_bt_stream_volumeset(box_data.data);
			single_uart_send_data_process(&box_data,&box_data.data,(uint8_t)sizeof(box_data.data));
			TRACE("send complete");
				
		}break;

		case FACTORY_CLAER_RECORD:{
			TRACE("this command is CLAER_RECORD:%x",box_data.cmd);
			app_ibrt_remove_history_paired_device();
			app_ibrt_remove_phone_paired_device();
			single_uart_send_data_process(&box_data,NULL,0);
			TRACE("send complete");	
		}break;

		case FACTORY_ENTER_PAIR:{
			TRACE("this command is ENTER_PAIR:%x",box_data.cmd);
			app_ibrt_ui_judge_scan_type(IBRT_ENTER_PAIRING_MODE_TRIGGER,NO_LINK_TYPE,IBRT_UI_NO_ERROR);
        	app_ibrt_ui_event_entry(IBRT_TWS_PAIRING_EVENT);
			single_uart_send_data_process(&box_data,NULL,0);
			TRACE("send complete");	
		}break;
		
		case FACTORY_SHUTDOWM_EAR:{
			TRACE("this command is SHUTDOWM_EAR:%x",box_data.cmd);
			single_uart_send_data_process(&box_data,NULL,0);
			app_shutdown();
			TRACE("send complete");
		}break;

		case FACTORY_ENTER_DUT:{
			TRACE("this command is ENTER_DUT:%x",box_data.cmd);
			single_uart_send_data_process(&box_data,NULL,0);
			app_factorymode_enter();
			TRACE("send complete");
		}break;
		
		case FACTORY_KEY_TEST:{
			TRACE("this command is KEY_TEST:%x",box_data.cmd);
			if(box_data.data == 0x00){
				enter_key_test_mode = false;
			}else if(box_data.data == 0x01){
				enter_key_test_mode = true;
			}
			single_uart_send_data_process(&box_data,&box_data.data,(uint8_t)sizeof(box_data.data));
			if(enter_key_test_mode){
				//Single click
				AT_send_key_event(HAL_KEY_CODE_PWR,HAL_KEY_EVENT_CLICK);
				box_data.data = 0x02;
				single_uart_send_data_process(&box_data,&box_data.data,(uint8_t)sizeof(box_data.data));

				//Double click
				AT_send_key_event(HAL_KEY_CODE_PWR,HAL_KEY_EVENT_DOUBLECLICK);
				box_data.data = 0x03;
				single_uart_send_data_process(&box_data,&box_data.data,(uint8_t)sizeof(box_data.data));

				//Triple click
				AT_send_key_event(HAL_KEY_CODE_PWR,HAL_KEY_EVENT_TRIPLECLICK);
				box_data.data = 0x04;
				single_uart_send_data_process(&box_data,&box_data.data,(uint8_t)sizeof(box_data.data));

				//Longpress 2s click
				AT_send_key_event(HAL_KEY_CODE_PWR,HAL_KEY_EVENT_CLICK);
				box_data.data = 0x05;
				single_uart_send_data_process(&box_data,&box_data.data,(uint8_t)sizeof(box_data.data));
				
				TRACE("send complete");
				enter_key_test_mode = false;
			}
		}break;
			
		case FACTORY_GET_BATTERY_VOLT:{
			TRACE("this command is GET_BATTERY_VOLT:%x",box_data.cmd);
			box_data.data = (AT_app_battery_get_current_volt()*100)/4200;
			single_uart_send_data_process(&box_data,&box_data.data,(uint8_t)sizeof(box_data.data));
			TRACE("send complete");
		}break;

		case FACTORY_TWS_PAIR:{
			TRACE("this command is TWS_PAIR:%x",box_data.cmd);
			factory_cmd_enter_tws_pairing();	
			box_data.data = 0x01;
			for(uint8_t i=0;i<25;i++){
				single_uart_send_data_process(&box_data,&box_data.data,(uint8_t)sizeof(box_data.data));
			}
			TRACE("send complete");
		}break;

		default:{
			TRACE("INVALID COMMAND");
			//uint8_t si_data = 0x74;
			//crc16_maxim_send(len,single_data,si_data);
		}break;
	}
}

void AT_command_handler(uint8_t *buf)
{
	uint8_t AT_cmd;
	uint8_t data;
	uint8_t AT_ret_array[30];
	int paired_dev_count = nv_record_get_paired_dev_count();
	btif_device_record_t   record[paired_dev_count];
	memset(AT_ret_array,0, sizeof(AT_ret_array));
	ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

	AT_cmd	= buf[3];
	data	= buf[4];	
	switch(AT_cmd)
    {
    	case CHECK:
			TRACE("AT^");
			memcpy(AT_ret_array,"OK/n",sizeof("OK/n"));
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));
           	break;
			
		case VERSION:
			TRACE("AT^VERSION?");
            TRACE("firmware_version is %s",firmware_version);
			sprintf((char *)AT_ret_array,"OK/nVERSION:%s",firmware_version);
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));
           	break;
			
		case TBATLVL:
			TRACE("AT^TBATLVL?");
			sprintf((char *)AT_ret_array,"OK/nTBATLVL=%d,%d",data,data);
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));
           	break;
			
		case SET_CANDISCOVERY:
			TRACE("AT^CANDISCOVERY=%d",data);
			if(data){
				#ifdef IBRT
					app_tws_ibrt_set_access_mode(BTIF_BAM_DISCOVERABLE_ONLY);
				#else
					app_bt_accessmode_set(BTIF_BAM_DISCOVERABLE_ONLY);
				#endif
			}else{
				#ifdef IBRT
					app_tws_ibrt_set_access_mode(BTIF_BAM_NOT_ACCESSIBLE);
				#else
					app_bt_accessmode_set(BTIF_BAM_NOT_ACCESSIBLE);
				#endif
			}
			//0:No discoverable mode 1:discoverable mode
			sprintf((char *)AT_ret_array,"OK/nAT^CANDISCOVERY=%d",data);
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));	
           	break;
			
		case CANDISCOVERY:
			TRACE("AT^CANDISCOVERY?");
		 	TRACE("current access mode is %x",p_ibrt_ctrl->access_mode);
			if( p_ibrt_ctrl->access_mode==BTIF_BAM_DISCOVERABLE_ONLY||
				p_ibrt_ctrl->access_mode==BTIF_BAM_GENERAL_ACCESSIBLE||
				p_ibrt_ctrl->access_mode==BTIF_BAM_LIMITED_ACCESSIBLE)
			{
				data=1;
			}else{
				data=0;
			}
			//0:No discoverable mode 1:discoverable mode
			sprintf((char *)AT_ret_array,"OK/nCANDISCOVERY:%d",data);
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));
           	break;
		
		case CANCONNECT:
			TRACE("AT^CANCONN?");
			TRACE("current access mode is %x",p_ibrt_ctrl->access_mode);
			if(p_ibrt_ctrl->access_mode==BTIF_BAM_CONNECTABLE_ONLY|| 
				p_ibrt_ctrl->access_mode==BTIF_BAM_GENERAL_ACCESSIBLE||
				p_ibrt_ctrl->access_mode==BTIF_BAM_LIMITED_ACCESSIBLE)
			{
				data=1;
			}else{
				data=0;
			}
			//0:No connectable mode 1:connectable mode
			sprintf((char *)AT_ret_array,"OK/nCANCONNECT:%d",data);
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));	
			break;
			
		case HISTORYLIST:
            TRACE("AT^HISTORYLIST?");
			osapi_lock_stack();
			for(int j = paired_dev_count - 1 ;j >= 0; --j){
				memset(AT_ret_array,0, sizeof(AT_ret_array));
				sprintf((char *)AT_ret_array,"OK/nHISTORYLIST:%02x %02x %02x %02x %02x %02x",
					record[j].bdAddr.address[0],record[j].bdAddr.address[1],record[j].bdAddr.address[2],
                    record[j].bdAddr.address[3],record[j].bdAddr.address[4],record[j].bdAddr.address[5]);
				communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));
  			}	
            osapi_unlock_stack();
			break;

		case BTNAME:
			TRACE("AT^BTNAME?");
			TRACE("%s",BT_LOCAL_NAME);
			sprintf((char *)AT_ret_array,"OK/nBT_NAME:%s",BT_LOCAL_NAME);
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));
           	break;
			
		case ENTERSOCLIMIT:
			TRACE("AT^ENTERSOCLIMIT");
			sprintf((char *)AT_ret_array,"OK/nAT^ENTERSOCLIMIT=%d",data);
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));
           	break;
			
		case SETTESTBATVALUE:
			TRACE("AT^SETTESTBATVALUE");
			sprintf((char *)AT_ret_array,"OK/nAT^SETTESTBATVALUE=%d",data);
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));
           	break;
			
		case LOWBATTERY:
			TRACE("AT^LOWBATTERY?");
			sprintf((char *)AT_ret_array,"OK/nLOWBATTERY:%d",data);
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));
           	break;
		
		case BTNSTATUS:
			uint8_t data_0,data_1;
			data_0 = data>>4;//LR_flag
			data_1 = data&0x0F;//key_event
			TRACE("AT^BTNSTATUS=%d:%d",data_0,data_1);
			if(data_0 != LR_flags){
				TRACE("this AT command is sent to peer eir");
				return;
			}
			HAL_KEY_EVENT_T AT_key_event;
			switch(data_1){
				case 0:AT_key_event = HAL_KEY_EVENT_CLICK;break;
				case 1:AT_key_event = HAL_KEY_EVENT_DOUBLECLICK;break;
				case 2:AT_key_event = HAL_KEY_EVENT_TRIPLECLICK;break;
				case 3:AT_key_event = HAL_KEY_EVENT_LONGPRESS;break;
				case 4:AT_key_event = HAL_KEY_EVENT_LONGLONGPRESS;break;
				default:AT_key_event = HAL_KEY_EVENT_NUM;break;
			}
			if(AT_key_event==HAL_KEY_EVENT_NUM){
				return;
			}
			AT_send_key_event(HAL_KEY_CODE_PWR,AT_key_event);//simulate key event
			sprintf((char *)AT_ret_array,"OK/nAT^BTNSTATUS=%d:%d",data_0,data_1);
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));
           	break;

		case BTCONNECTACTIVE:
			TRACE("AT^BTCONNECTACTIVE?");
			data = app_tws_ibrt_mobile_link_connected()?1:0;
			TRACE("mobile link is %d",data);
			//0:No BT_CONNECTED 1:BT_CONNECTED
			sprintf((char *)AT_ret_array,"OK/nBTCONNECTACTIVE:%d",data);
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));
           	break;
			
		case PAIR:
			TRACE("AT^PAIR?");
           	data = is_app_bt_pairing_running()?1:0;
			TRACE("the bt pairing is %d",data);
			//0:idle 1:pairing
			sprintf((char *)AT_ret_array,"OK/nPAIR:%d",data);
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));
           	break;
		case BTSTATUS:
			TRACE("AT^BTSTATUS?");
			//0:idle	1:来电	2:去电	3:通话中		4:三方通话  
			//5:音乐播放		6:SPP	7:配对
			data = AT_get_bt_status();
			sprintf((char *)AT_ret_array,"OK/nBTSTATUS:%d",data);
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));
           	break;
			
		case PHYNUM:
			TRACE("AT^PHYNUM?");
			sprintf((char *)AT_ret_array,"OK/nPHYNUM:%d",data);
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));
           	break;
		
		case BTSIRI:
			TRACE("AT^BTSIRI?");
			TRACE("the siri flag is %d",open_siri_flag);
			//0:the siri is sniff 1:the siri is active
			sprintf((char *)AT_ret_array,"OK/nBTSIRI:%d",open_siri_flag);
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));
           	break;
			
		case TBATVOLT:
			TRACE("AT^TBATVOLT?");
			sprintf((char *)AT_ret_array,"OK/nTBATVOLT=%d",data);
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));
           	break;
		
		case SET_TCHRENABLE:
			TRACE("AT^TCHRENABLE=");
			struct HAL_IOMUX_PIN_FUNCTION_MAP battery_charger_enable_cfg;
			battery_charger_enable_cfg.pin=HAL_IOMUX_PIN_P1_1;
			battery_charger_enable_cfg.function=HAL_IOMUX_FUNC_AS_GPIO;
			battery_charger_enable_cfg.volt=HAL_IOMUX_PIN_VOLTAGE_VIO;
			//1:enable 0:disable
			battery_charger_enable_cfg.pull_sel=data?HAL_IOMUX_PIN_PULLUP_ENALBE:HAL_IOMUX_PIN_NOPULL;
			hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&battery_charger_enable_cfg, 1);

			sprintf((char *)AT_ret_array,"OK/nAT^TCHRENABLE=%d",data);
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));
           	break;
		
		case BOXSTATUS:
			TRACE("AT^BOXSTATUS?");
			if(box_event==IBRT_OPEN_BOX_EVENT){
				data = 0;
			}else if(box_event==IBRT_CLOSE_BOX_EVENT){
				data = 1;
			}
			//0:open_box 1:close_box
			sprintf((char *)AT_ret_array,"OK/nBOXSTATUS:%d",data);
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));
           	break;
		
		case SET_BOXSTATUS:
			TRACE("AT^BOXSTATUS=%d",data);
			if(data){
				app_ibrt_ui_event_entry(IBRT_CLOSE_BOX_EVENT);
			}else{
				app_ibrt_ui_event_entry(IBRT_OPEN_BOX_EVENT);
			}
			//0:open_box 1:close_box
			sprintf((char *)AT_ret_array,"OK/nAT^BOXSTATUS=%d",data);
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));
           	break;
		
		case TEMPINFO:
			TRACE("AT^TEMPINFO?");
			sprintf((char *)AT_ret_array,"OK/nTEMPINFO=%d",data);
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));
           	break;
		
		case INBOX:
			TRACE("AT^INBOX?");
			if(box_event==IBRT_PUT_IN_EVENT){
				data = 0;
			}else if(box_event==IBRT_FETCH_OUT_EVENT){
				data = 1;
			}
			sprintf((char *)AT_ret_array,"OK/nINBOX:%d",data);
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));
           	break;
		
		case SET_INBOX:
			TRACE("AT^INBOX=%d",data);
			if(data){
				app_ibrt_ui_event_entry(IBRT_PUT_IN_EVENT);
			}else{
				app_ibrt_ui_event_entry(IBRT_FETCH_OUT_EVENT);
			}
			sprintf((char *)AT_ret_array,"OK/nAT^INBOX=%d",data);
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));
           	break;

		default:
			TRACE("INVALID AT COMMAND");
			memcpy(AT_ret_array,"this is invalid command",sizeof("this is invalid command"));
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));
			break;
	}
}

void plug_inout_timer_handler(void const* parm)
{
	uint32_t lock;
	lock = int_lock();
	uart_rx_dma_stop();
	hal_uart_flush(comm_uart,0);
	uart_rx_dma_start();
	int_unlock(lock);

	if(plug_inout_flag || MSG_RX_REQ_CNT || uart_error_detected){
		plug_inout_flag = false;
		cosonic_trace(plug_out,true,"the earphone plug out");
		app_ibrt_simulate_charger_plug_out_test();
	}
	//communication_io_mode_switch(COMMUNICATION_MODE_ENABLE_IRQ);
	communication_io_mode_init();
}

