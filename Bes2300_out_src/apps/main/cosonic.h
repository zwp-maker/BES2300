/*
 * @Description: 
 * @Version: 
 * @Autor: jevin
 * @Date: 2020-12-21 10:17:20
 * @LastEditors: Jevin
 * @LastEditTime: 2021-01-18 17:02:57
 */
#include "cmsis_os.h"
#ifdef __cplusplus
extern "C" {
#endif
extern osTimerId timer0_handle;
extern osTimerId timer1_handle;
extern osTimerId timer2_handle;
extern osTimerId pwroff_cmd_timer_handle;
void led_delay_start(APP_STATUS_INDICATION_T,APP_STATUS_INDICATION_T);
int app_status_indication_scanner(void);
extern int e_wdt;
extern int delay_flag;
void app_pwrkey_irq_close(void);
void app_pwrkey_irq_open(void);
void synchronized_power_off(void);
extern uint8_t tws_pairing_timeout_cnt;
#ifdef __cplusplus
}
#endif