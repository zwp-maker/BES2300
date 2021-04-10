#ifndef __COMMUNICATION_SVR_H__
#define __COMMUNICATION_SVR_H__

#ifdef __cplusplus
extern "C" {
#endif

#if 0
enum SINGLE_CMD
{
    OPEN_BOX         = 0x01,    
    CLOSE_BOX        = 0x02,
    RESET_EAR        = 0x03,
    SHUTDOWN_EAR     = 0x04,
    TWS_PAIRING      = 0x05,    // tws enter pairing mode.
    FREEMAN_MODE     = 0x06,    // ear enter freeman mode.
    GET_EAR_INFO     = 0x07,    // get ear inbformation, reserved.
    EXCHANGE_PWR     = 0x08,    // box and ear exchang power information.
    CLEAR_PHONE_INFO = 0x09,    // ear clear phone paired information.
    RESET_FACTORY    = 0x0A,    // reset to factory.
    GET_EAR_MAC      = 0x20,    // get ear mac address.
    SET_EAR_MAC      = 0x21,    // set ear mac address
};
#else
enum SINGLE_CMD
{
    POWERON          = 0x01,    
    CLOSE_BOX        = 0x02,
    POWEROFF         = 0x03,
    PAIRING          = 0x04,    // tws enter pairing mode
    CLEAR_PHONE_INFO = 0x05,    // ear clear phone paired information.
    SINGLE_DLD_MODE  = 0x06,    // ear enter single_dld mode.
    DUT_MODE         = 0x07,    // ear enter dut mode.
    FREEMAN_PAIRING  = 0x08,    // ear enter freeman pairing.
    TWS_PAIRING      = 0x09,    // tws enter pairing mode.
    RESET_FACTORY    = 0x0A,    // reset to factory.
    SHUTDOWN_EAR     = 0x0B,
};
#endif

enum SINGLE_AT
{
    CHECK             = 0x01,    // check AT status
    VERSION           = 0x02,	 // check version
    TBATLVL           = 0x03,	 // battery status
    SET_CANDISCOVERY  = 0x04,    // set TWS candiscover
    CANDISCOVERY      = 0x05,    // cheak TWS candiscover status
    CANCONNECT		  = 0x06,	 // cheak TWS canconnect status
    HISTORYLIST       = 0x07,    // cheak TWS-Phone history
    BTNAME            = 0x08,    // cheak  BT name
    ENTERSOCLIMIT     = 0x09,    // cheak simulation battery status
    SETTESTBATVALUE   = 0x0A,    // set   simulation battery status
    LOWBATTERY        = 0x0B,    // cheak battery low flag
    BTNSTATUS         = 0x0C,    // set key events
    BTCONNECTACTIVE   = 0x0D,    // cheak BT connect status
    PAIR              = 0x0E,	 // cheak BT pair status
	BTSTATUS          = 0x0F,	 // cheak BT event status
	PHYNUM            = 0x10,	 // cheak TWS MAC address
	BTSIRI            = 0x11,    // cheak SIRI status
	TBATVOLT          = 0x12,	 // cheak battery volt
	SET_TCHRENABLE    = 0x13,	 // set charge dis/enable
	BOXSTATUS         = 0x14,	 // cheak box  status
	SET_BOXSTATUS     = 0x15,    // set   box  status
	TEMPINFO		  = 0x16,    // cheak board temperature
	INBOX    		  = 0x17,    // cheak earphone in/out box status
	SET_INBOX         = 0x18,	 // set earphone in box status
};


enum USER_CMD_KEY_TYPE {
	KEY_TYPE_NONE = 0,
	KEY_TYPE_1,
	KEY_TYPE_2
};

typedef enum
{
	AT_ENTER_SOC_LIMIT,
	AT_EXIT_SOC_LIMIT,
	AT_UNKNOWN_SOC_LIMIT,
}AT_SOC_LIMIT_MODE;
	
typedef struct single_communication
{
    uint8_t head;
    SINGLE_CMD cmd;
    uint8_t *data;
    uint16_t checksum;
}single_st;

typedef void (*communication_receive_func_typedef)(uint8_t *buf, uint8_t len);

void communication_init(void);
int communication_receive_register_callback(communication_receive_func_typedef p);
int communication_send_buf(uint8_t * buf, uint8_t len);
void single_uart_receive_data_process(uint8_t *buf, uint8_t len);
int app_ibrt_single_communication_response(uint8_t *buffer, uint32_t length);

#ifdef KNOWLES_UART_DATA
void uart_audio_init();
void uart_audio_deinit();
#endif


#ifdef __cplusplus
}
#endif

#endif

