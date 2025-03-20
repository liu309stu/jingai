#include "usart.h"	  

#if 1
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 

}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
void _sys_exit(int x) 
{ 
	x = x; 
} 
//重定义fputc函数 
int fputc(int ch, FILE *f)
{      
	while((USART1->SR&0X40)==0);//循环发送,直到发送完毕   
    USART1->DR = (u8) ch;      
	return ch;
}
#endif 


 
#if EN_USART1_RX   //如果使能了接收
//串口1中断服务程序
//注意,读取USARTx->SR能避免莫名其妙的错误   	
u8 USART_RX_BUF[USART_REC_LEN];     //接收缓冲,最大USART_REC_LEN个字节.
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
u16 USART_RX_STA=0;       //接收状态标记	  
  
void uart_init(u32 bound){
	 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);	//使能USART1，GPIOA时钟
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC->APB2ENR|=1<<2;   //使能PORTA口时钟  
	RCC->APB2ENR|=1<<14;  //使能串口时钟 
	
	GPIOA->CRH&=0XFFFFF00F;//IO状态设置 配置A9A10
	GPIOA->CRH|=0X000008B0;//IO状态设置 
	
	RCC->APB2RSTR|=1<<14;   //复位串口1
	RCC->APB2RSTR&=~(1<<14);//停止复位	   	   
	//波特率设置
 	USART1->BRR=0X0271; // 波特率设置	 
	USART1->CR1|=0X202C;  //1位停止,无校验位. 接收缓冲区非空中断使能	
	
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;  /* 配置中断源*/
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;  /* 配置抢占优先级 */
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  /* 配置子优先级 */
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;  /* 使能中断通道 */
	NVIC_Init(&NVIC_InitStructure);      //F0
	
	// 初始化 ADC1
	printf("+++");
}

void USART1_IRQHandler(void)                	//串口1中断服务程序
	{
	u8 Res;
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)  //接收中断(接收到的数据必须是0x0d 0x0a结尾)
		{
		Res =USART_ReceiveData(USART1);	//读取接收到的数据

		if ((USART_RX_STA & 0x8000) == 0) // 接收未完成
		{

			if (Res == 0x00)
				USART_RX_STA |= 0x8000;
			else
			{
				USART_RX_BUF[USART_RX_STA & 0X3FFF] = Res;
				USART_RX_STA++;
				if (USART_RX_STA > (USART_REC_LEN - 1))
					USART_RX_STA = 0; // 接收数据错误,重新开始接收
			}
		}
	 } 
} 
#endif	

