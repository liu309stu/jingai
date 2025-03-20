#ifndef __WIFI_C_H
#define __WIFI_C_H
#include "sys_cfg.h"
#include "usart.h"
#include <stdio.h>
#include <stdint.h>

// 传感器数据结构体
typedef struct {
    uint32_t time;
    float water_level;      // 井盖上面的沉积水位 (cm)
    double latitude;        // 北纬
    double longitude;       // 东经
    uint32_t timestamp;     // 时间戳
    uint16_t sequence_id;   // 序列号
    uint8_t cover_status;   // 井盖状态 (0: 关闭, 1: 打开)
    float distance_to_water;// 井盖到井底水面的距离 (cm)
    uint8_t motor_status;   // 电机状态 (0: 关闭, 1: 运行)
    uint8_t smoke_detected; // 是否检测到烟雾 (0: 无烟雾, 1: 有烟雾)
    float battery_level;    // 电池电量 (0~100 %)
    float rain_level;
} Data;

// 变量声明
extern Data sensor_data;
extern uint32_t timestamp;

// AT 指令
extern const unsigned char* post_head;
extern const unsigned char* get_time_head;
extern const unsigned char* get_state_head;
extern const unsigned char* wifi_link_at;
extern const unsigned char* wifi_mode;
extern const unsigned char* tcp_link_at;
extern const unsigned char* ip_mode_open;
extern const unsigned char* tcp_send;
extern const unsigned char* at_start;
extern const unsigned char* echo_off;

// 串口缓冲区
extern uint8_t  USART_RX_BUF[];
extern uint16_t USART_RX_STA;

// 函数声明
void send_cmd(const char* cmd);
uint8_t WIFI_React(uint8_t mode);
void WIFI_Init(void);
void WIFI_GetTime(void);
void WIFI_PostData(void);
void WIFI_GetMotor_State_Change(void);
void WIFI_SendRetart(void);

#endif 