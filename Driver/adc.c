#include "adc.h"
#include "wifi.h"
#include "math.h"

extern Data sensor_data;
// 定义全局变量用于存储 ADC 转换结果
// Arrays to store raw and averaged ADC values
uint32_t adc_accumulated[5] = {0}; // Accumulate raw ADC values
uint16_t adc_averaged[5] = {0};    // Store averaged ADC values

// 配置 GPIO 为模拟输入模式
void GPIO_AnalogInput_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;

    // 使能 GPIOA 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // 配置 PA0 - PA3 为模拟输入模式
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 |GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

// 配置 ADC1
void ADC1_Init(void) {
    GPIO_AnalogInput_Init();

    ADC_InitTypeDef ADC_InitStructure;

    // 使能 ADC1 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    // ADC1 配置
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    // 使能 ADC1
    ADC_Cmd(ADC1, ENABLE);

    // 使能 ADC1 的复位校准寄存器
    ADC_ResetCalibration(ADC1);
    // 等待复位校准寄存器完成
    while (ADC_GetResetCalibrationStatus(ADC1));
    // 开启 ADC1 的校准
    ADC_StartCalibration(ADC1);
    // 等待校准完成
    while (ADC_GetCalibrationStatus(ADC1));
}

// 读取指定通道的 ADC 值
uint16_t Read_ADC_Channel(uint8_t channel) {
    // 配置 ADC 通道
    ADC_RegularChannelConfig(ADC1, channel, 1, ADC_SampleTime_55Cycles5);

    // 启动 ADC 转换
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);

    // 等待转换完成
    while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));

    // 读取 ADC 转换结果
    return ADC_GetConversionValue(ADC1);
}


void ADC_Read_All(void) {
    int channel_index, sample_index;

    // Number of samples to average
    const int num_samples = 10;

    // Loop through each ADC channel (1 to 5)
    for (channel_index = 1; channel_index <= 5; channel_index++) {
        // Take multiple samples for each channel and accumulate the values
        for (sample_index = 0; sample_index < num_samples; sample_index++) {
            adc_accumulated[channel_index - 1] += Read_ADC_Channel(channel_index);
        }

        // Calculate the average value for the channel
        adc_averaged[channel_index - 1] = adc_accumulated[channel_index - 1] / num_samples;
    }


    // Optional: Update global variables or process the ADC values further
    // Example: Update sensor data structure or send data via communication interface

    Process_ADC_Data();
}

void Process_ADC_Data(void) {
    // 光敏电阻阈值，用于判断井盖是否关闭
    const uint16_t LIGHT_THRESHOLD = 1000; // 经验值，低于此值判断为无光
    // 雨滴传感器的阈值和覆盖率换算
    const float RAIN_COVERAGE_SCALE = 50.0 / (2450 - 36); // 线性比例换算
    const uint16_t RAIN_THRESHOLD = 500; // 雨滴的阈值，用于判断是否有雨
    // 锂电池电压比例系数
    const float BATTERY_VOLTAGE_MULTIPLIER = (3.3 / 4096) * 2; // 转换为实际电压(V)
    // 烟雾传感器阈值
    const uint16_t SMOKE_THRESHOLD = 2000; // 用于判断有无烟雾

    // 光敏电阻：井盖状态判断
    if (adc_averaged[0] < LIGHT_THRESHOLD) {
        sensor_data.cover_status = 1; // 无光：井盖关闭
    } else {
        sensor_data.cover_status = 0; // 有光：井盖打开
    }

    // 雨滴传感器：雨滴覆盖率计算
    if (adc_averaged[1] < RAIN_THRESHOLD) {
        sensor_data.rain_level = 0; // 无水
    } else {
        // 线性计算雨滴覆盖率
        sensor_data.rain_level = (adc_averaged[1] - 36) * RAIN_COVERAGE_SCALE;
        if (sensor_data.rain_level > 100) {
            sensor_data.rain_level = 100; // 限制最大值为100%
        }
    }

    // 压力传感器：水位计算（假设平方关系）
    // 水位计算公式：水位 = k * ADC^2 (k为经验系数，根据实际标定确定)
    const float PRESSURE_CONVERSION_FACTOR = 0.000001f; // 经验值，需调整
    sensor_data.water_level = PRESSURE_CONVERSION_FACTOR * pow(adc_averaged[2], 2);

    // 锂电池电量监测：电压 . 百分比
    float battery_voltage = adc_averaged[4] * BATTERY_VOLTAGE_MULTIPLIER;
    if (battery_voltage >= 4.2) {
        sensor_data.battery_level = 100; // 满电
    } else if (battery_voltage <= 3.7) {
        sensor_data.battery_level = 0; // 电量耗尽
    } else {
        sensor_data.battery_level = (battery_voltage - 3.7) / (4.2 - 3.7) * 100; // 线性比例计算
    }

    // 烟雾传感器：烟雾状态
    if (adc_averaged[3] > SMOKE_THRESHOLD) {
        sensor_data.smoke_detected = 1; // 有烟雾
    } else {
        sensor_data.smoke_detected = 0; // 无烟雾
    }

}