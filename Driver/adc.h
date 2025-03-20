#ifndef  __ADC_H_P
#define  __ADC_H_P
#include "sys_cfg.h"


extern uint32_t adc_accumulated[5]; // 累加 ADC 值
extern uint16_t adc_averaged[5];    // 平均 ADC 值

// 函数声明
void GPIO_AnalogInput_Init(void); // 配置 GPIO 为模拟输入模式
void ADC1_Init(void);             // 初始化 ADC1
uint16_t Read_ADC_Channel(uint8_t channel); // 读取指定通道的 ADC 值
void ADC_Read_All(void);          // 读取所有 ADC 通道并计算平均值
void Process_ADC_Data(void);      // 处理 ADC 数据并更新 sensor_data 结构体

#endif // __ADC_H