#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "lsens.h"


/*******************************************************************************
* 函 数 名         : main
* 函数功能		   : 主函数
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
int main()
{
	u8 i=0;
	u8 lsens_value=0;
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(115200);
	Lsens_Init();
	
	while(1)
	{
		i++;
		if(i%20==0)
		{
			LED1=!LED1;
		}
		
		if(i%50==0)
		{
			lsens_value=Lsens_Get_Val();
			printf("光照强度：%d\r\n",lsens_value);
		}
		delay_ms(10);	
	}
}
