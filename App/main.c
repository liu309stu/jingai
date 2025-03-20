#include "sys_cfg.h"
#include "delay.h"
#include "usart.h"
#include "adc.h"
#include "muc_rtc.h"
#include "wifi.h"
#include "hcsr04.h"


int main(){
	delay_init();
	uart_init(115200);
	RTC_Init();
	delay_ms(1000);
	USART_RX_STA=0;
	
	HC_SR04_Init();
	ADC1_Init();
	WIFI_Init();
	// 使用RTC同步时钟
	WIFI_GetTime();
	WIFI_GetTime();
	int count = 0;
	while (1) {
		sonar_mm()     ; // 测量距离
		ADC_Read_All() ;
		WIFI_PostData();
		delay_s(1) ;
		if (count >= 10)
		{
			count = 0;
			WIFI_SendRetart();
		}else
		{
			count++;
		}
	}
}



