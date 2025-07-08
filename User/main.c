#include "system.h"
#include "SysTick.h"
#include "lsens.h"
#include "../APP/tftlcd/tftlcd.h"
#include "../APP/dht11/dht11.h"
#include "../APP/rtc/rtc.h"
#include "../APP/key/key.h"
#include "../APP/beep/beep.h"
#include "../APP/hc05/hc05.h"
#include "usart.h"
#include "usart3.h"
#include "string.h"
#include "led.h"
#include "stdlib.h"

// 声明外部函数
extern void USART3_Init(u32 bound);
extern void u3_printf(char *fmt, ...);

// 声明外部变量，用于访问蜂鸣器状态和音量
extern u8 beep_status;
extern u16 beep_duty;
extern u16 beep_period;

// 当前功能模式: 0=传感器模式, 1=蓝牙模式
u8 current_mode = 0;

// 蓝牙初始化状态: 0=未初始化, 1=已初始化成功, 2=初始化失败, 3=初始化被中断
u8 bt_initialized = 0;

// 蓝牙相关变量
u8 sendmask = 0;
u8 sendcnt = 0;
u8 sendbuf[20];

// 蜂鸣器延时控制变量
u16 beep_delay = 0;		  // 延时计数器，单位：100ms
u8 beep_timer_active = 0; // 0: 未激活, 1: 已激活

// 初始化蓝牙模块，可被KEY2中断
// 返回值: 0=成功, 1=失败, 2=被中断
u8 Initialize_Bluetooth(void)
{
	// 声明变量（必须在函数开头）
	u8 retry;
	u8 key_val;

	// 如果已经初始化过且成功，直接返回
	if (bt_initialized == 1)
		return 0;

	LCD_ShowString(10, 100, 200, 16, 16, (u8 *)"Initializing HC05...");
	LCD_ShowString(10, 230, 220, 16, 16, (u8 *)"KEY2:Cancel & Return");

	// 等待蓝牙模块上电稳定
	delay_ms(500);

	// 初始化串口3
	USART3_Init(9600);

	// 尝试初始化HC05
	retry = 3; // 最多尝试3次
	while (retry--)
	{
		// 每次尝试前检查KEY2是否按下
		key_val = KEY_Scan(0);
		if (key_val == KEY2_PRESS)
		{
			LCD_ShowString(10, 100, 200, 16, 16, (u8 *)"Init Canceled!     ");
			printf("HC05 Init canceled by user\r\n");
			bt_initialized = 3; // 标记为被中断
			delay_ms(500);
			return 2; // 返回被中断状态
		}

		if (HC05_Init() == 0)
		{
			LCD_ShowString(10, 100, 200, 16, 16, (u8 *)"HC05 Init OK!      ");
			printf("HC05 OK!\r\n");
			bt_initialized = 1;
			delay_ms(500);
			return 0;
		}

		printf("HC05 Init failed, retrying...\r\n");
		LCD_ShowString(10, 100, 200, 16, 16, (u8 *)"HC05 Error!      ");
		delay_ms(300);

		// 再次检查KEY2是否按下
		key_val = KEY_Scan(0);
		if (key_val == KEY2_PRESS)
		{
			LCD_ShowString(10, 100, 200, 16, 16, (u8 *)"Init Canceled!     ");
			printf("HC05 Init canceled by user\r\n");
			bt_initialized = 3; // 标记为被中断
			delay_ms(500);
			return 2; // 返回被中断状态
		}

		LCD_ShowString(10, 100, 200, 16, 16, (u8 *)"Retrying...      ");
		delay_ms(300);

		// 再次检查KEY2是否按下
		key_val = KEY_Scan(0);
		if (key_val == KEY2_PRESS)
		{
			LCD_ShowString(10, 100, 200, 16, 16, (u8 *)"Init Canceled!     ");
			printf("HC05 Init canceled by user\r\n");
			bt_initialized = 3; // 标记为被中断
			delay_ms(500);
			return 2; // 返回被中断状态
		}
	}

	// 初始化失败
	printf("HC05 Error!\r\n");
	LCD_ShowString(10, 100, 200, 16, 16, (u8 *)"HC05 Init Failed! ");
	bt_initialized = 2;
	delay_ms(1000);
	return 1;
}

// 显示HC05模块的主从状态
void HC05_Role_Show(void)
{
	if (HC05_Get_Role() == 1)
	{
		LCD_ShowString(10, 140, 200, 16, 16, "ROLE:Master"); // 主机
	}
	else
	{
		LCD_ShowString(10, 140, 200, 16, 16, "ROLE:Slave "); // 从机
	}
}

// 显示HC05模块的连接状态
void HC05_Sta_Show(void)
{
	if (HC05_LED)
	{
		LCD_ShowString(110, 140, 120, 16, 16, "STA:Connected "); // 连接成功
	}
	else
	{
		LCD_ShowString(110, 140, 120, 16, 16, "STA:Disconnect"); // 未连接
	}
}

// 显示温湿度和光照信息
void Show_Sensor_Info(u8 temp, u8 humi, u8 lsens_value)
{
	char buf[32];

	// 显示标题（只显示一次，不重复刷新）
	LCD_Fill(50, 10, 200, 26, WHITE);
	LCD_ShowString(50, 10, 150, 16, 16, (u8 *)"Sensor Data");

	// 温湿度 - 局部刷新
	LCD_Fill(10, 50, 220, 66, WHITE);
	sprintf(buf, "Temp: %2dC Humi: %2d%%", temp, humi);
	LCD_ShowString(10, 50, 220, 16, 16, (u8 *)buf);

	// 光敏值 - 局部刷新
	LCD_Fill(10, 80, 220, 96, WHITE);
	sprintf(buf, "Light: %3d", lsens_value);
	LCD_ShowString(10, 80, 220, 16, 16, (u8 *)buf);

	// 状态指示 - 局部刷新
	LCD_Fill(10, 110, 220, 126, WHITE);
	if (temp > 28)
	{
		LCD_ShowString(10, 110, 220, 16, 16, (u8 *)"Status: Hot");
	}
	else if (temp < 15)
	{
		LCD_ShowString(10, 110, 220, 16, 16, (u8 *)"Status: Cold");
	}
	else
	{
		LCD_ShowString(10, 110, 220, 16, 16, (u8 *)"Status: OK");
	}

	LCD_Fill(10, 140, 220, 156, WHITE);
	sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", calendar.w_year, calendar.w_month, calendar.w_date, calendar.hour, calendar.min, calendar.sec);
	LCD_ShowString(10, 140, 220, 16, 16, (u8 *)buf);

	// 显示蜂鸣器状态和音量
	LCD_Fill(10, 170, 220, 186, WHITE);
	sprintf(buf, "BEEP: %s", beep_status ? "ON" : "OFF");
	LCD_ShowString(10, 170, 220, 16, 16, (u8 *)buf);

	// 显示蜂鸣器音量百分比
	LCD_Fill(10, 200, 220, 216, WHITE);
	sprintf(buf, "Volume: %d%%", beep_duty * 100 / beep_period);
	LCD_ShowString(10, 200, 220, 16, 16, (u8 *)buf);

	// 如果蜂鸣器定时器已激活，显示倒计时
	if (beep_timer_active)
	{
		LCD_Fill(10, 260, 220, 276, WHITE);
		sprintf(buf, "BEEP will ON in %d.%ds", beep_delay / 10, beep_delay % 10);
		LCD_ShowString(10, 260, 220, 16, 16, (u8 *)buf);
	}
	else
	{
		LCD_Fill(10, 260, 220, 276, WHITE);
	}

	// 显示当前模式
	LCD_Fill(10, 230, 220, 246, WHITE);
	LCD_ShowString(10, 230, 220, 16, 16, (u8 *)"Press KEY2 to BT mode");
}

// 显示蓝牙界面
void Show_BT_Info(void)
{
	LCD_Clear(WHITE);
	FRONT_COLOR = RED;
	LCD_ShowString(10, 10, 240, 16, 16, (u8 *)"PRECHIN");
	LCD_ShowString(10, 30, 240, 16, 16, (u8 *)"www.prechin.com");
	LCD_ShowString(10, 50, 240, 16, 16, (u8 *)"BT05 BlueTooth Test");
	LCD_ShowString(10, 90, 210, 16, 16, (u8 *)"KEY_UP:ROLE   KEY1:SEND/STOP");
	LCD_ShowString(10, 110, 200, 16, 16, (u8 *)"HC05 Standby!");
	LCD_ShowString(10, 160, 200, 16, 16, (u8 *)"Send:");
	LCD_ShowString(10, 180, 200, 16, 16, (u8 *)"Receive:");
	LCD_ShowString(10, 230, 220, 16, 16, (u8 *)"Press KEY2 to Sensor mode");

	// 如果蜂鸣器定时器已激活，显示倒计时
	if (beep_timer_active)
	{
		char buf[32];
		sprintf(buf, "BEEP will ON in %d.%ds", beep_delay / 10, beep_delay % 10);
		LCD_ShowString(10, 260, 220, 16, 16, (u8 *)buf);
	}

	FRONT_COLOR = BLUE;
	HC05_Role_Show();
	HC05_Sta_Show();
}

// 处理接收到的蓝牙命令
void Process_BT_Command(u8 *buf, u16 len)
{
	// 控制LED2
	if (strcmp((const char *)buf, "+LED2 ON\r\n") == 0)
	{
		LED2 = 0; // 打开LED2
	}
	else if (strcmp((const char *)buf, "+LED2 OFF\r\n") == 0)
	{
		LED2 = 1; // 关闭LED2
	}

	// 直接控制蜂鸣器
	else if (strcmp((const char *)buf, "+BEEP ON\r\n") == 0)
	{
		BEEP_On();
	}
	else if (strcmp((const char *)buf, "+BEEP OFF\r\n") == 0)
	{
		BEEP_Off();
	}

	// 延时控制蜂鸣器
	else if (strncmp((const char *)buf, "+BEEP ", 6) == 0)
	{
		// 解析延时时间（秒）
		u8 seconds = atoi((const char *)&buf[6]);
		if (seconds > 0)
		{
			// 设置延时
			beep_delay = seconds * 10; // 转换为100ms单位
			beep_timer_active = 1;	   // 激活定时器

			// 反馈信息
			if (bt_initialized == 1)
			{
				u3_printf("Will turn on BEEP in %d seconds\r\n", seconds);
			}
			printf("Will turn on BEEP in %d seconds\r\n", seconds);

			// 更新显示
			if (current_mode == 0)
			{
				char buf[32];
				LCD_Fill(10, 260, 220, 276, WHITE);
				sprintf(buf, "BEEP will ON in %d.%ds", beep_delay / 10, beep_delay % 10);
				LCD_ShowString(10, 260, 220, 16, 16, (u8 *)buf);
			}
			else
			{
				// 在蓝牙模式下，刷新整个界面来显示倒计时
				Show_BT_Info();
			}
		}
	}
}

int main()
{
	u8 temp = 0, humi = 0;
	u8 lsens_value = 0;
	u8 key_val = 0;
	u8 t = 0;
	u8 reclen = 0;
	u8 bt_init_result = 0;

	// 系统初始化
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); // 中断优先级分组 分2组

	// 外设初始化
	USART1_Init(115200);
	TFTLCD_Init();
	LCD_Clear(WHITE);
	FRONT_COLOR = BLACK;

	// 传感器初始化
	DHT11_Init();
	Lsens_Init();
	RTC_Init();
	RTC_Get(); // 初始化calendar结构体

	// 按键、LED和蜂鸣器初始化
	KEY_Init();
	LED_Init();
	BEEP_Init();

	// 显示启动信息
	LCD_ShowString(60, 60, 200, 24, 24, (u8 *)"Starting...");
	delay_ms(1000); // 延长显示时间，确保用户能看到

	// 清屏并显示初始界面（传感器模式）
	LCD_Clear(WHITE);

	// 首次读取传感器数据，确保有初始值
	DHT11_Read_Data(&temp, &humi);
	lsens_value = Lsens_Get_Val();

	// 显示初始数据
	Show_Sensor_Info(temp, humi, lsens_value);

	while (1)
	{
		// 处理蜂鸣器定时器
		if (beep_timer_active)
		{
			if (beep_delay > 0)
			{
				beep_delay--;
				if (beep_delay == 0)
				{
					BEEP_On(); // 延时结束，打开蜂鸣器
					beep_timer_active = 0;

					// 更新显示
					if (current_mode == 0)
					{
						Show_Sensor_Info(temp, humi, lsens_value);
					}
					else
					{
						char buf[32];
						LCD_Fill(10, 260, 220, 276, WHITE);
						LCD_ShowString(10, 260, 220, 16, 16, (u8 *)"BEEP is now ON!");
					}

					// 反馈信息
					if (bt_initialized == 1)
					{
						u3_printf("BEEP is now ON!\r\n");
					}
					printf("BEEP is now ON!\r\n");
				}
				else if (beep_delay % 10 == 0 && (current_mode == 0 || current_mode == 1))
				{
					// 每秒更新一次显示
					char buf[32];
					LCD_Fill(10, 260, 220, 276, WHITE);
					sprintf(buf, "BEEP will ON in %d.%ds", beep_delay / 10, beep_delay % 10);
					LCD_ShowString(10, 260, 220, 16, 16, (u8 *)buf);
				}
			}
		}

		// 扫描按键
		key_val = KEY_Scan(0);

		// 模式切换（KEY2按键）
		if (key_val == KEY2_PRESS)
		{
			if (current_mode == 0)
			{
				// 正在从传感器模式切换到蓝牙模式

				// 尝试初始化蓝牙模块(如果还没初始化)
				bt_init_result = Initialize_Bluetooth();

				if (bt_init_result == 0)
				{
					// 初始化成功，切换到蓝牙模式
					current_mode = 1;
					Show_BT_Info();
				}
				else if (bt_init_result == 2)
				{
					// 初始化被用户中断，保持在传感器模式
					LCD_Clear(WHITE);
					Show_Sensor_Info(temp, humi, lsens_value);
				}
				else
				{
					// 初始化失败，保持在传感器模式
					LCD_ShowString(10, 230, 220, 16, 16, (u8 *)"BT Init Failed!         ");
					delay_ms(1000);
					LCD_ShowString(10, 230, 220, 16, 16, (u8 *)"Press KEY2 to try again");
				}
			}
			else
			{
				// 从蓝牙模式切换到传感器模式
				current_mode = 0;
				LCD_Clear(WHITE);
				Show_Sensor_Info(temp, humi, lsens_value);
			}
		}

		if (current_mode == 0)
		{ // 传感器模式
			if (key_val == KEY_UP_PRESS)
			{
				// KEY_UP: 开关蜂鸣器
				BEEP_Toggle();
				// 立即更新显示
				Show_Sensor_Info(temp, humi, lsens_value);
			}
			else if (key_val == KEY1_PRESS)
			{
				// KEY1: 增大音量
				BEEP_Volume_Increase();
				// 立即更新显示
				Show_Sensor_Info(temp, humi, lsens_value);
			}

			RTC_Get();
			// 读取DHT11温湿度数据
			if (DHT11_Read_Data(&temp, &humi) == 0)
			{
				// DHT11读取成功，更新显示
				Show_Sensor_Info(temp, humi, lsens_value);
			}

			// 读取光敏传感器数据
			lsens_value = Lsens_Get_Val();

			// 更新显示（减少刷新频率）
			if (t % 10 == 0)
			{
				Show_Sensor_Info(temp, humi, lsens_value);
			}
		}
		else
		{ // 蓝牙模式
			if (key_val == KEY_UP_PRESS)
			{
				// 切换模块主从设置
				key_val = HC05_Get_Role();
				if (key_val != 0XFF)
				{
					key_val = !key_val; // 状态取反
					if (key_val == 0)
						HC05_Set_Cmd("AT+ROLE=0");
					else
						HC05_Set_Cmd("AT+ROLE=1");
					HC05_Role_Show();
					HC05_Set_Cmd("AT+RESET"); // 复位HC05模块
					delay_ms(200);
				}
			}
			else if (key_val == KEY1_PRESS)
			{
				sendmask = !sendmask; // 发送/停止发送
				if (sendmask == 0)
					LCD_Fill(10 + 40, 160, 240, 160 + 16, WHITE); // 清除显示
			}

			if (t % 50 == 0)
			{
				if (sendmask)
				{ // 定时发送
					sprintf((char *)sendbuf, "PREHICN HC05 %d\r\n", sendcnt);
					LCD_ShowString(10 + 40, 160, 200, 16, 16, sendbuf); // 显示发送数据
					printf("%s\r\n", sendbuf);
					u3_printf("PREHICN HC05 %d\r\n", sendcnt); // 发送到蓝牙模块
					sendcnt++;
					if (sendcnt > 99)
						sendcnt = 0;
				}
				HC05_Sta_Show();
				LED1 = !LED1;
			}

			if (USART3_RX_STA & 0X8000)
			{										// 接收到一次数据了
				LCD_Fill(10, 200, 240, 320, WHITE); // 清除显示
				reclen = USART3_RX_STA & 0X7FFF;	// 得到数据长度
				USART3_RX_BUF[reclen] = '\0';		// 加入结束符
				printf("reclen=%d\r\n", reclen);
				printf("USART3_RX_BUF=%s\r\n", USART3_RX_BUF);

				// 处理接收到的命令
				Process_BT_Command(USART3_RX_BUF, reclen);

				LCD_ShowString(10, 200, 209, 119, 16, USART3_RX_BUF); // 显示接收到的数据
				USART3_RX_STA = 0;
			}
		}

		// 延时100ms后再次读取
		delay_ms(100);
		t++;
	}
}