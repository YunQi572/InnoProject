#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "lsens.h"
#include "../APP/tftlcd/tftlcd.h"
#include "../APP/rtc/rtc.h"
#include "../APP/dht11/dht11.h"
#include "../APP/beep/beep.h"
#include "../APP/ws2812/ws2812.h"
#include "../APP/key/key.h"
#include "../APP/hwjs/hwjs.h"
#include "../APP/pwm/pwm.h"

// 预留：电机、蓝牙相关头文件
// #include "motor.h"
// #include "bluetooth.h"

u8 alarm_on = 0;         // 闹钟使能标志
u8 alarm_hour = 7;       // 默认闹钟小时
u8 alarm_min = 0;        // 默认闹钟分钟
u8 alarm_triggered = 0;  // 闹钟触发标志

void Show_Main_Info(u8 temp, u8 humi, u8 lsens_value) {
	char buf[32];
	//LCD_Clear(GREEN);
	// 时间
	sprintf(buf, "时间: %02d:%02d:%02d", calendar.hour, calendar.min, calendar.sec);
	LCD_ShowString(10, 10, 220, 32, 16, (u8*)buf);
	// 日期
	sprintf(buf, "日期: %04d-%02d-%02d", calendar.w_year, calendar.w_month, calendar.w_date);
	LCD_ShowString(10, 30, 220, 32, 16, (u8*)buf);
	// 温湿度
	sprintf(buf, "温度: %2dC 湿度: %2d%%", temp, humi);
	LCD_ShowString(10, 50, 220, 32, 16, (u8*)buf);
	// 光敏
	sprintf(buf, "光敏: %3d", lsens_value);
	LCD_ShowString(10, 70, 220, 32, 16, (u8*)buf);
	// 闹钟状态
	if(alarm_on)
		sprintf(buf, "闹钟: %02d:%02d ON", alarm_hour, alarm_min);
	else
		sprintf(buf, "闹钟: OFF");
	LCD_ShowString(10, 90, 220, 32, 16, (u8*)buf);
}

void Alarm_Action(u8 lsens_value) {
	static u8 rgb_bright = 0;
	// 蜂鸣器响
	BEEP = 1;
	// RGB灯渐亮（日出模拟）
	if(rgb_bright < 255) rgb_bright += 5;
	RGB_LED_Write_24Bits(rgb_bright, rgb_bright, rgb_bright); // 白色渐亮
	// 预留：电机震动
	// Motor_On();
	// 可根据光敏值调整亮度
	// ...
}

void Alarm_Stop() {
	BEEP = 0;
	RGB_LED_Clear();
	// 预留：电机停止
	// Motor_Off();
	alarm_triggered = 0;
}

int main()
{
	SysTick_Init(72);
	LED_Init();
	USART1_Init(115200);
	TFTLCD_Init();
	LCD_Clear(GREEN);
	FRONT_COLOR = BLACK;
	LCD_ShowString(10, 10, 220, 32, 16, (u8*)"Hello LCD!");
	printf("main start!\r\n");
	while(1) {
		LED1 = !LED1;
		delay_ms(500);
	}
}
