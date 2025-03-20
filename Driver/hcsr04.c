#include "hcsr04.h"
#include "wifi.h"
void Timer_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);		//启用TIM3时钟

	TIM_InternalClockConfig(TIM3);								//设置TIM3使用内部时钟
	
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;			//定义结构体，配置定时器
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;	//设置1分频（不分频）
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;	//设置计数模式为向上计数
	TIM_TimeBaseInitStructure.TIM_Period = 10 - 1;			//设置最大计数值，达到最大值触发更新事件，因为从0开始计数，所以计数10次是10-1,每10微秒触发一次
	TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1;			//设置时钟预分频，72-1就是每 时钟频率(72Mhz)/72=1000000 个时钟周期计数器加1,每1微秒+1
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;		//重复计数器（高级定时器才有，所以设置0）
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);			//初始化TIM3定时器
	
	TIM_ClearFlag(TIM3, TIM_FLAG_Update);			//清除更新中断标志位
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);		//开启更新中断
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);				//设置中断优先级分组
	
	NVIC_InitTypeDef NVIC_InitStructure;						//定义结构体，配置中断优先级
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;				//指定中断通道
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//中断使能
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;	//设置抢占优先级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;			//设置响应优先级
	NVIC_Init(&NVIC_InitStructure);								// https://blog.zeruns.tech
	
	TIM_Cmd(TIM3, ENABLE);							//开启定时器
}

/*
void TIM3_IRQHandler(void)			//更新中断函数
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) == SET)		//获取TIM3定时器的更新中断标志位
	{
		
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);			//清除更新中断标志位
	}
}*/


#define Echo GPIO_Pin_0		//HC-SR04模块的Echo脚接GPIOB6
#define Trig GPIO_Pin_1		//HC-SR04模块的Trig脚接GPIOB5

uint64_t time=0;			//声明变量，用来计时
uint64_t time_end=0;		//声明变量，存储回波信号时间

void HC_SR04_Init(void)
{
    Timer_Init();
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);	//启用GPIOB的外设时钟	
	GPIO_InitTypeDef GPIO_InitStructure;					//定义结构体
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;		//设置GPIO口为推挽输出
	GPIO_InitStructure.GPIO_Pin = Trig;						//设置GPIO口5
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		//设置GPIO口速度50Mhz
	GPIO_Init(GPIOB,&GPIO_InitStructure);					//初始化GPIOB
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;			//设置GPIO口为下拉输入模式
	GPIO_InitStructure.GPIO_Pin = Echo;						//设置GPIO口6
	GPIO_Init(GPIOB,&GPIO_InitStructure);					//初始化GPIOB
	GPIO_WriteBit(GPIOB,GPIO_Pin_5,0);						//输出低电平
	delay_us(15);											//延时15微秒
}

int16_t sonar_mm(void)									//测距并返回单位为毫米的距离结果
{
	uint32_t Distance,Distance_mm = 0;
	GPIO_WriteBit(GPIOB,Trig,1);						//输出高电平
	delay_us(15);										//延时15微秒
	GPIO_WriteBit(GPIOB,Trig,0);						//输出低电平
	while(GPIO_ReadInputDataBit(GPIOB,Echo)==0);		//等待低电平结束
	time=0;												//计时清零
	while(GPIO_ReadInputDataBit(GPIOB,Echo)==1);		//等待高电平结束
	time_end=time;										//记录结束时的时间
	if(time_end/100<38)									//判断是否小于38毫秒，大于38毫秒的就是超时，直接调到下面返回0
	{
		Distance=(time_end*346)/2;						//计算距离，25°C空气中的音速为346m/s
		Distance_mm=Distance/100;						//因为上面的time_end的单位是10微秒，所以要得出单位为毫米的距离结果，还得除以100
	}
    sensor_data.distance_to_water = (float) (Distance_mm) / 10;
	return Distance_mm;									//返回测距结果
}

float sonar(void)										//测距并返回单位为米的距离结果
{
	uint32_t Distance,Distance_mm = 0;
	float Distance_m=0;
	GPIO_WriteBit(GPIOB,Trig,1);					//输出高电平
	delay_us(15);
	GPIO_WriteBit(GPIOB,Trig,0);					//输出低电平
	while(GPIO_ReadInputDataBit(GPIOB,Echo)==0);
	time=0;
	while(GPIO_ReadInputDataBit(GPIOB,Echo)==1);
	time_end=time;
	if(time_end/100<38)
	{
		Distance=(time_end*346)/2;
		Distance_mm=Distance/100;
		Distance_m=Distance_mm/1000;
	}
	return Distance_m;
}

void TIM3_IRQHandler(void)			//更新中断函数，用来计时，每10微秒变量time加1
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) == SET)		//获取TIM3定时器的更新中断标志位
	{
		time++;
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);			//清除更新中断标志位
	}
}
