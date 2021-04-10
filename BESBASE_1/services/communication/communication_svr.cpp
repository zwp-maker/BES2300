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

extern "C" int open_siri_flag;
extern "C" const char *BT_LOCAL_NAME;
extern "C" uint8_t box_event;

extern uint8_t AT_get_bt_status(void);
extern "C" int AT_send_key_event(enum HAL_KEY_CODE_T code, enum HAL_KEY_EVENT_T event);


extern "C" void app_bt_accessmode_set(btif_accessible_mode_t);
extern "C" int nv_record_get_paired_dev_count(void);
extern "C" bt_status_t nv_record_enum_dev_records(unsigned short index,btif_device_record_t* record);
extern "C" void osapi_lock_stack(void);
extern "C" void osapi_unlock_stack(void);

extern "C" uint8_t Charge_detect;
extern uint8_t old_capacity;
uint8_t save_old_capacity = 0;//设置模拟电量保存上次电量
unsigned int save_current_capacity = 0;//设置模拟电量保存当前电量
AT_SOC_LIMIT_MODE at_soc_limit_mode = AT_UNKNOWN_SOC_LIMIT;//设置模拟电量（电量管控模式）
enum APP_BATTERY_CHARGER_T
{
    APP_BATTERY_CHARGER_PLUGOUT = 0,
    APP_BATTERY_CHARGER_PLUGIN,
    APP_BATTERY_CHARGER_QTY,
};
enum APP_BATTERY_BOX_STATUS
{
	APP_BATTERY_BOX_CLOSE =0 ,
	APP_BATTERY_BOX_OPEN

};
extern enum APP_BATTERY_BOX_STATUS box_opcl ;
extern enum APP_BATTERY_CHARGER_T at_charger;
bool b_set_test_capacity = false;//设置模拟电量标记
uint16_t sim_volt = 0;
extern bool low_battery_flag;
extern void AT_app_battery_set_current_volt(int16_t volt);
extern int16_t AT_app_battery_get_current_volt(void);
extern void AT_app_gpio_event_process(enum APP_BATTERY_CHARGER_T status);
extern void AT_set_capacity(unsigned int currvolt,bool ret);

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
        if (xfer_size){
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

static void uart_rx_idle_handler(void const *param)
{
    // const uint8_t rx_idle_indicate[2] = {0x10,0x24};
	if(uart_rx_idle_counter++ >= 900)//900 * 100 = 90s
    //if(uart_rx_idle_counter++ >= 150)//150 * 100 = 15s
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
            break;
        case COMMUNICATION_MSG_TX_DONE:            
            break;
        case COMMUNICATION_MSG_RX_REQ:
            communication_io_mode_init();
            uart_init();
            uart_rx_dma_start();
            uart_rx_idle_timer_start();
            break;
        case COMMUNICATION_MSG_RX_DONE:
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
            lock = int_lock();
            uart_rx_dma_stop();
            communication_io_mode_switch(COMMUNICATION_MODE_RX);
            hal_uart_flush(comm_uart, 0);
            uart_rx_dma_start();
            uart_error_detected = 0;            
            int_unlock(lock);
            uart_rx_idle_timer_start();
            break;

        default:
            break;
    }

}

void communication_init(void)
{
    COMMUNICATION_MAIL msg;
    COMMAND_BLOCK *cmd_blk;

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

    communication_command_block_alloc(&cmd_blk);
    cmd_blk->cmd_buf[0] = 0xff;
    cmd_blk->cmd_len = 1;
    communication_send_command(cmd_blk);
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

static void crc16_maxim_send(uint8_t length,single_st *single_data,uint8_t data)
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
}



void single_uart_data_packet_send(uint8_t cmd,uint8_t ear_type,uint8_t *payload,uint16_t len)
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
}

extern int battery_charger_status;
extern void app_gpio_simulate_charger_plug_in_test(void);
extern void app_gpio_simulate_charger_plug_out_test(void);
bool factory_cdc_handle(int8_t AT,uint8_t length,single_st *single_data,uint8_t data)
{
	uint8_t AT_ret_array[30];
	int paired_dev_count = nv_record_get_paired_dev_count();
	//btif_device_record_t   record[paired_dev_count];
	btif_device_record_t   record;
	memset(AT_ret_array,0, sizeof(AT_ret_array));
	TRACE("THIS IS POWERON COMMAND %x ", AT);
	
	ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
	switch (AT)
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
			sprintf((char *)AT_ret_array,"OK/nTBATLVL=%d,%d %%",battery_charger_status,(AT_app_battery_get_current_volt()/4200)*100);
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));
           	break;
			
		case SET_CANDISCOVERY:
			TRACE("AT^CANDISCOVERY=%d",data);
			if(data){
				#ifdef IBRT
					//app_tws_ibrt_set_access_mode(BTIF_BAM_DISCOVERABLE_ONLY);
					app_tws_ibrt_set_access_mode(BTIF_BAM_GENERAL_ACCESSIBLE);
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
			//0:no discoverable mode 1:discoverable mode
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
			for(unsigned short i = paired_dev_count - 1 ;i >= 0; --i){
				memset(AT_ret_array,0, sizeof(AT_ret_array));
				nv_record_enum_dev_records(i, &record);
				sprintf((char *)AT_ret_array,"OK/nHISTORYLIST:%02x %02x %02x %02x %02x %02x",
					record.bdAddr.address[0],record.bdAddr.address[1],record.bdAddr.address[2],
                    record.bdAddr.address[3],record.bdAddr.address[4],record.bdAddr.address[5]);
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
			if(data){
				at_soc_limit_mode = AT_ENTER_SOC_LIMIT;
			}else{
				at_soc_limit_mode = AT_EXIT_SOC_LIMIT;
				sim_volt = 0xFFFF;
				old_capacity = save_old_capacity;
				AT_app_battery_set_current_volt(save_current_capacity);
			}
			sprintf((char *)AT_ret_array,"OK/nAT^ENTERSOCLIMIT=%d",data);
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));
           	break;
			
		case SETTESTBATVALUE:
			TRACE("AT^SETTESTBATVALUE");
			if(at_soc_limit_mode == AT_ENTER_SOC_LIMIT){
				b_set_test_capacity=true;
				sim_volt = 3100 + 1100*data;
				AT_app_battery_set_current_volt(sim_volt);//设置模拟电量
			}else{
				sim_volt = 0xFFFF;
				TRACE("SET TEST BATVALUE ERROR!!");
			}
			sprintf((char *)AT_ret_array,"OK/nAT^SETTESTBATVALUE=%d",sim_volt);
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));
           	break;
			
		case LOWBATTERY:
			TRACE("AT^LOWBATTERY?");
			sprintf((char *)AT_ret_array,"OK/nLOWBATTERY:%d",low_battery_flag);
			communication_send_buf(AT_ret_array,(uint8_t)strlen((const char*)AT_ret_array));
           	break;
		
		case BTNSTATUS:
			uint8_t data_0,data_1;
			data_0 = data>>4;//LR_flag
			data_1 = data&0x0F;//key_event
			TRACE("AT^BTNSTATUS=%d:%d",data_0,data_1);
			if(data_0 != LR_flags){
				TRACE("this AT command is sent to peer eir");
				return false;
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
				return false;
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
			sprintf((char *)AT_ret_array,"OK/nTBATVOLT=%d",AT_app_battery_get_current_volt());
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
			if(box_event==IBRT_OPEN_BOX_EVENT || box_event==IBRT_PUT_IN_EVENT){
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
				//app_gpio_simulate_charger_plug_in_test();
			}else{
				app_ibrt_ui_event_entry(IBRT_FETCH_OUT_EVENT);
				//app_gpio_simulate_charger_plug_out_test();
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
    return TRUE;
}
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
bool cmd_flag = false;
void single_uart_receive_data_process(uint8_t *buf, uint8_t len)
{

    single_st *single_data = NULL;
	int8_t cmd = 0;
	enum USER_CMD_KEY_TYPE cmd_type = KEY_TYPE_NONE;
    // search head
    

	TRACE("%s", __func__);
	DUMP8("%02x", buf, len);
	
    //if (len < 4 || len > 128 || (buf[0] !=0x55 && buf[0] !=0xBB && (buf[0] != 0x41 || buf[1] != 0x54 || buf[2] != 0x3f) ) )
	if ((len < 4 || len > 128)\
		/*|| (buf[0] !=0x55 && buf[0] !=0xBB )\*/
		|| (buf[0] != 0x41 || buf[1] != 0x54 || buf[2] != 0x5E))
    {
        TRACE("INVALID PACKET DATA:buf[0]:%d,len: %d!!!",buf[0],len);
        return;
    }
	cmd_type = buf[0] == 0x55 ? KEY_TYPE_1 : KEY_TYPE_2;
	cmd_flag = cmd_type == KEY_TYPE_1 ? true : false;
	//invalid_len = buf[1]+4;
	cmd = buf[1];
	if (crc16_maxim(buf,len,0))
	{
		TRACE("RCV UART CRC16_maxim failed");
	}

	if(cmd_type == KEY_TYPE_2 && cmd_flag == true )
	{
		TRACE("%s : box cmd has not been completed ", __func__);
		return;
	}
	if ((buf[0] == 0x41 && buf[1] ==0x54 && buf[2] == 0x5E))
	{
		bool ret = false;
		cmd = buf[3];
		uint8_t data = buf[4];
		ret = factory_cdc_handle(cmd,len,single_data,data);
		osDelay(20);
		if (ret && single_data)
    	{
        	free(single_data);
    	}
		return;
	}
	
    {    
        switch (cmd)
        {

        case POWERON:
			TRACE("THIS IS POWERON COMMAND %x ", single_data->cmd);
            app_ibrt_ui_event_entry(IBRT_OPEN_BOX_EVENT);
            break;
		
		case CLOSE_BOX:
			TRACE("THIS IS CLOSE_BOX COMMAND %x", single_data->cmd);
            app_ibrt_ui_event_entry(IBRT_CLOSE_BOX_EVENT);
            break;
		
		case POWEROFF:
			TRACE("THIS IS POWEROFF COMMAND %x ", single_data->cmd);
            break;
		
		case PAIRING:
			TRACE("THIS IS PAIRING COMMAND %x", single_data->cmd);
            app_ibrt_ui_set_enter_pairing_mode(IBRT_NOTIFY_PAIRING);
            break;	
		
		case CLEAR_PHONE_INFO:
			TRACE("THIS IS CLEAR_PHONE_INFO COMMAND %x ", single_data->cmd);
            break;
		
		case SINGLE_DLD_MODE:
			TRACE("THIS IS SINGLE_DLD_MODE COMMAND %x", single_data->cmd);
            break;
		
		case DUT_MODE:
			TRACE("THIS IS DUT_MODE COMMAND %x", single_data->cmd);
            break;
		
		case FREEMAN_PAIRING:
			TRACE("THIS IS FREEMAN_PAIRING COMMAND %x", single_data->cmd);
            break;
		
		case TWS_PAIRING:
			TRACE("THIS IS TWS_PAIRING COMMAND %x", single_data->cmd);
        	break;
		
		case RESET_FACTORY:
			TRACE("THIS IS RESET_FACTORY COMMAND %x", single_data->cmd);
        	break;
		
		case SHUTDOWN_EAR:
			TRACE("THIS IS SHUTDOWN_EAR COMMAND %x", single_data->cmd);
        	break;


        default:
            TRACE("INVALID COMMAND");
			uint8_t si_data = 0x74;
			crc16_maxim_send(len,single_data,si_data);
			
            break;
        }
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

