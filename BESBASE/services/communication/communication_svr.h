#ifndef __COMMUNICATION_SVR_H__
#define __COMMUNICATION_SVR_H__

#ifdef __cplusplus
extern "C" {
#endif


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

typedef struct single_communication
{
    uint16_t head;
    SINGLE_CMD cmd;
    uint8_t which;  //11 :left, 22:right
    uint8_t len;
    uint8_t *data;
    uint8_t checksum;
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

