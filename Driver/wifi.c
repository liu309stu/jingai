#include "wifi.h"
#include "string.h"
#include "delay.h"
#include "muc_rtc.h"

uint8_t WIFI_Init_State = 0;

// 初始化结构体对象
Data sensor_data = {
    .water_level = 12.5,         // 沉积水位
    .latitude = 39.9042,         // 纬度
    .longitude = 116.4074,       // 经度
    .timestamp = 1742370890,     // 时间戳
    .sequence_id = 1001,         // 序列号
    .cover_status = 0,           // 是否井盖是否盖上
    .distance_to_water = 250.0,  // 距离井下水面的距离(cm)
    .motor_status = 0,     // 电机状态
    .smoke_detected = 0,   // 默认无烟雾
    .battery_level = 85.0, // 默认电池电量 85%
    .rain_level = 0        // 雨滴覆盖率
};

const unsigned char *post_head = "POST /submit HTTP/1.1\r\nHost: your_server_ip:your_server_port\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: ";
const unsigned char *get_time_head = "GET /time HTTP/1.1\r\nHost: your_server_ip:5000\r\n\r\n";
const unsigned char *get_state_head = "GET /action HTTP/1.1\r\nHost: your_server_ip:5000\r\n";
const unsigned char *wifi_link_at = "AT+CWJAP=\"XIAOKANG\",\"12345678\"\r\n";
const unsigned char *wifi_mode = "AT+CWMODE=1\r\n";
const unsigned char *tcp_link_at = "AT+CIPSTART=\"TCP\",\"10.70.60.87\",5000\r\n";
const unsigned char *ip_mode_open = "AT+CIPMODE=1\r\n";
const unsigned char *tcp_send = "AT+CIPSEND\r\n";
const unsigned char *at_start = "AT\r\n";   // 检测模块是否响应
const unsigned char *echo_off = "ATE0\r\n"; // 关闭回显
const unsigned char *sleep_mode_on = "AT+SLEEP=2\r\n";
const unsigned char *sleep_mode_off = "AT+SLEEP=0\r\n";

uint32_t timestamp = 0; // 全局变量存储时间戳
extern uint8_t USART_RX_BUF[USART_REC_LEN];
extern uint16_t USART_RX_STA;

void send_cmd(const char *cmd)
{
    printf("%s", cmd); // 实际应替换为串口发送函数
}

uint8_t WIFI_React(uint8_t mode)
{
    // while ((USART_RX_STA & 0x8000) == 0) { __ASM("NOP"); }
    delay_ms(40);
    int message_len = USART_RX_STA & 0X3FFF;
    if (message_len <= 0)
        return -1;

    int valid = 0;
    switch (mode)
    {
    case 0:
    case 1:
    {
        const char *expected = (mode == 0) ? "OK\r\n" : "Ok\r\n";
        valid = (strncmp((char *)&USART_RX_BUF[message_len - strlen(expected)], expected, strlen(expected)) == 0);
    }
    break;
    case 2:
    {
        const char *prefix = "{'time': ";
        char *time_start = strstr((char *)USART_RX_BUF, prefix);
        if (time_start)
        {
            time_start += strlen(prefix);
            timestamp = strtoul(time_start, NULL, 10); // 提取时间
            valid = 1;
        }
    }
    break;
    case 3:
    {
        const char *expected1 = "OOK\r\n";
        const char *expected2 = "FOK\r\n";
        valid = (strncmp((char *)&USART_RX_BUF[message_len - strlen(expected1)], expected1, strlen(expected1)) == 0) ||
                (strncmp((char *)&USART_RX_BUF[message_len - strlen(expected2)], expected2, strlen(expected2)) == 0);
        if (valid)
        {
            if (strncmp((char *)&USART_RX_BUF[message_len - 6], "OOK\r\n", 5) == 0)
            {
                sensor_data.motor_status = 1;
            }
            else if (strncmp((char *)&USART_RX_BUF[message_len - 6], "FOK\r\n", 5) == 0)
            {
                sensor_data.motor_status = 0;
            }
        }
    }
    break;
    default:
        return -1;
    }

    if (valid)
    {
        USART_RX_STA = 0; // 释放缓冲区状态
        return 0;
    }

    return -1; // 不匹配
}

void WIFI_Init(void)
{
    send_cmd(at_start);
    if (WIFI_React(0) != 0)
        return;

    send_cmd(echo_off);
    if (WIFI_React(0) != 0)
        return;

    send_cmd(wifi_mode);
    if (WIFI_React(0) != 0)
        return;

    send_cmd(wifi_link_at);
    delay_s(20);
    if (WIFI_React(0) != 0)
        return;

    send_cmd(tcp_link_at);
    delay_s(5);
    if (WIFI_React(0) != 0)
        return;

    send_cmd(ip_mode_open);
    if (WIFI_React(0) != 0)
        return;
    WIFI_Init_State = 1;

    send_cmd(tcp_send);
    delay_ms(10);
    USART_RX_STA = 0;
}

void WIFI_GetTime(void)
{
    if (WIFI_Init_State)
    {
        /* code */
        send_cmd(get_time_head);
        WIFI_React(2);
        RTC_Set_UnixTime(timestamp);

    }
}

void WIFI_PostData(void)
{
    if (WIFI_Init_State)
    {
        sensor_data.timestamp = RTC_GetCounter();
        char data_buffer[250];
        snprintf(data_buffer, sizeof(data_buffer),
                 "water_level=%.2f&latitude=%.6f&longitude=%.6f&timestamp=%lu&sequence_id=%d&cover_status=%d&distance_to_water=%.2f&motor_status=%d&smoke_detected=%d&battery_level=%.2f&rain_level=%.2f",
                 sensor_data.water_level, sensor_data.latitude, sensor_data.longitude,
                 sensor_data.timestamp, sensor_data.sequence_id, sensor_data.cover_status,
                 sensor_data.distance_to_water, sensor_data.motor_status,
                 sensor_data.smoke_detected, sensor_data.battery_level, sensor_data.rain_level);

        char length_buffer[20];
        snprintf(length_buffer, sizeof(length_buffer), "%d\r\n", (int)strlen(data_buffer));

        send_cmd(post_head);
        send_cmd(length_buffer);
        send_cmd("\r\n");
        send_cmd(data_buffer);
        WIFI_React(1);

    }
}

void WIFI_GetMotor_State_Change(void)
{
    if (WIFI_Init_State)
    {
        send_cmd(get_state_head);
        WIFI_React(3);
    }
}

int WIFI_Sleep(void)
{
    if (WIFI_Init_State)
    {
        /* code */
        send_cmd("+++");
        delay_ms(20);
        USART_RX_STA = 0;
        send_cmd(sleep_mode_on);
        if (WIFI_React(0) != 0)
            return 1;
    }
}

int WIFI_Restart(void)
{
    if (WIFI_Init_State)
    {
        /* code */
        delay_ms(20);
        USART_RX_STA = 0;
        send_cmd(sleep_mode_off);
        send_cmd(tcp_send);
    }
}


void WIFI_SendRetart(void){
    if (WIFI_Init_State)
    {
        /* code */
        delay_ms(20);
        send_cmd("+++");
        delay_s(10);
        send_cmd(tcp_send);
        delay_s(1);
        USART_RX_STA = 0;
    }
}