#include "system.h"
#include "SysTick.h"
#include "lsens.h"
#include "../APP/tftlcd/tftlcd.h"
#include "../APP/dht11/dht11.h"
#include "../APP/rtc/rtc.h"

// 显示温湿度和光照信息
void Show_Sensor_Info(u8 temp, u8 humi, u8 lsens_value) {
	char buf[32];
	
	// 显示标题（只显示一次，不重复刷新）
	LCD_Fill(50, 10, 200, 26, WHITE);
	LCD_ShowString(50, 10, 150, 16, 16, (u8*)"Sensor Data");
	
	// 温湿度 - 局部刷新
	LCD_Fill(10, 50, 220, 66, WHITE);
	sprintf(buf, "Temp: %2dC Humi: %2d%%", temp, humi);
	LCD_ShowString(10, 50, 220, 16, 16, (u8*)buf);
	
	// 光敏值 - 局部刷新
	LCD_Fill(10, 80, 220, 96, WHITE);
	sprintf(buf, "Light: %3d", lsens_value);
	LCD_ShowString(10, 80, 220, 16, 16, (u8*)buf);
	
	// 状态指示 - 局部刷新
	LCD_Fill(10, 110, 220, 126, WHITE);
	if(temp > 28) {
		LCD_ShowString(10, 110, 220, 16, 16, (u8*)"Status: Hot");
	} else if(temp < 15) {
		LCD_ShowString(10, 110, 220, 16, 16, (u8*)"Status: Cold");
	} else {
		LCD_ShowString(10, 110, 220, 16, 16, (u8*)"Status: OK");
	}
	
	
	 LCD_Fill(10, 140, 220, 156, WHITE);
  sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", calendar.w_year, calendar.w_month, calendar.w_date, calendar.hour, calendar.min, calendar.sec);
  LCD_ShowString(10, 140, 220, 16, 16, (u8*)buf);
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
	
	// 传感器初始化
	DHT11_Init();
	
	Lsens_Init();

	RTC_Init();
  RTC_Get(); // 初始化calendar结构体
	// 显示启动信息
	LCD_ShowString(60, 60, 200, 24, 24, (u8*)"Starting...");
	delay_ms(2000);  // 延长显示时间，确保用户能看到
	LCD_Clear(WHITE);
	
	// 显示固定标题
	LCD_ShowString(50, 10, 150, 16, 16, (u8*)"Sensor Data");
	
	// 首次读取传感器数据，确保有初始值
	DHT11_Read_Data(&temp, &humi);
	lsens_value = Lsens_Get_Val();
	
	// 显示初始数据
	Show_Sensor_Info(temp, humi, lsens_value);
	
	while(1) {
		RTC_Get();
		// 读取DHT11温湿度数据
		if(DHT11_Read_Data(&temp, &humi) == 0) {
			// DHT11读取成功，更新显示
			Show_Sensor_Info(temp, humi, lsens_value);
		}
		
		// 读取光敏传感器数据
		lsens_value = Lsens_Get_Val();
		
		// 更新显示（减少刷新频率）
		Show_Sensor_Info(temp, humi, lsens_value);
		
		// 延时2秒后再次读取，减少闪烁
		delay_ms(2000);
	}
}