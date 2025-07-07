#include "system.h"
#include "SysTick.h"
#include "lsens.h"
#include "../APP/tftlcd/tftlcd.h"
#include "../APP/dht11/dht11.h"

// 显示温湿度和光照信息
void Show_Sensor_Info(u8 temp, u8 humi, u8 lsens_value) {
	char buf[32];
	
	// 显示标题
	LCD_Fill(10, 10, 230, 30, WHITE);
	LCD_ShowString(50, 10, 200, 16, 16, (u8*)"Sensor Data");
	
	// 温湿度
	LCD_Fill(10, 50, 230, 70, WHITE);
	sprintf(buf, "Temp: %2dC Humi: %2d%%", temp, humi);
	LCD_ShowString(10, 50, 220, 16, 16, (u8*)buf);
	
	// 光敏值
	LCD_Fill(10, 80, 230, 100, WHITE);
	sprintf(buf, "Light: %3d", lsens_value);
	LCD_ShowString(10, 80, 220, 16, 16, (u8*)buf);
	
	// 状态指示
	LCD_Fill(10, 110, 230, 130, WHITE);
	if(temp > 28) {
		LCD_ShowString(10, 110, 220, 16, 16, (u8*)"Status: Hot");
	} else if(temp < 15) {
		LCD_ShowString(10, 110, 220, 16, 16, (u8*)"Status: Cold");
	} else {
		LCD_ShowString(10, 110, 220, 16, 16, (u8*)"Status: OK");
	}
}

int main()
{
	u8 temp = 0, humi = 0;
	u8 lsens_value = 0;
	
	// 系统初始化
	SysTick_Init(72);
	
	// 外设初始化
	TFTLCD_Init();
	LCD_Clear(WHITE);
	FRONT_COLOR = BLACK;
	
	DHT11_Init();
	Lsens_Init();
	
	// 显示启动信息
	LCD_ShowString(60, 60, 200, 24, 24, (u8*)"Starting...");
	delay_ms(1000);
	LCD_Clear(WHITE);
	
	while(1) {
		// 读取DHT11温湿度数据
		if(DHT11_Read_Data(&temp, &humi) != 0) {
			// 如果读取失败，保持上次的值
		}
		
		// 读取光敏传感器数据
		lsens_value = Lsens_Get_Val();
		
		// 更新显示
		Show_Sensor_Info(temp, humi, lsens_value);
		
		// 延时1秒后再次读取
		delay_ms(1000);
	}
}