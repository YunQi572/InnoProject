#include "system.h"
#include "SysTick.h"
#include "lsens.h"
#include "../APP/tftlcd/tftlcd.h"
#include "../APP/dht11/dht11.h"
#include "../APP/rtc/rtc.h"
#include "../APP/key/key.h"
#include "../APP/beep/beep.h"
#include "../APP/hc05/hc05.h"
#include "../APP/hwjs/hwjs.h"
#include "../APP/pwm/pwm.h"
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

// 红外遥控按键编码
#define IR_KEY1 0x00FF30CF // 按键1的编码（根据实际遥控器可能需要修改）
#define IR_KEY2 0x00FF18E7 // 按键2的编码（根据实际遥控器可能需要修改）
#define IR_KEY3 0x00FF7A85 // 按键3的编码
#define IR_KEY4 0x00FF10EF // 按键4的编码
#define IR_KEY5 0x00FF38C7 // 按键5的编码
#define IR_KEY6 0x00FF5AA5 // 按键6的编码
#define IR_KEY7 0x00FF42BD // 按键7的编码 - 增大电机功率
#define IR_KEY8 0x00FF4AB5 // 按键8的编码 - 减小电机功率
#define IR_KEY9 0x00FF52AD // 按键9的编码

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

// BEEP设置模式
u8 beep_setting_mode = 0;	  // 0: 非设置模式, 1: 设置模式
u16 beep_setting_seconds = 0; // 设置的延时秒数

// 直流电机控制变量
u16 motor_power = 0;	   // 当前电机功率值(0-500)
u16 motor_power_max = 499; // 最大电机功率值

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

	// 显示当前模式
	LCD_Fill(10, 230, 220, 246, WHITE);
	LCD_ShowString(10, 230, 220, 16, 16, (u8 *)"Press KEY2 to BT mode");

	// 显示BEEP状态或倒计时
	LCD_Fill(10, 260, 220, 276, WHITE);
	if (beep_setting_mode)
	{
		LCD_ShowString(10, 260, 220, 16, 16, (u8 *)"BEEP: SETTING");

		// 显示当前设置的延时时间
		LCD_Fill(10, 290, 220, 306, WHITE);
		sprintf(buf, "Delay: %d seconds", beep_setting_seconds);
		LCD_ShowString(10, 290, 220, 16, 16, (u8 *)buf);
	}
	else if (beep_timer_active)
	{
		sprintf(buf, "BEEP: COUNTDOWN %d.%ds", beep_delay / 10, beep_delay % 10);
		LCD_ShowString(10, 260, 220, 16, 16, (u8 *)buf);
	}
	else
	{
		sprintf(buf, "BEEP: %s", beep_status ? "ON" : "OFF");
		LCD_ShowString(10, 260, 220, 16, 16, (u8 *)buf);
	}

	// 显示LED2状态
	LCD_Fill(10, 320, 220, 336, WHITE);
	sprintf(buf, "LED2: %s", LED2 ? "OFF" : "ON");
	LCD_ShowString(10, 320, 220, 16, 16, (u8 *)buf);

	// 显示电机功率
	LCD_Fill(10, 350, 220, 366, WHITE);
	sprintf(buf, "Motor Power: %d", motor_power);
	LCD_ShowString(10, 350, 220, 16, 16, (u8 *)buf);
}

// 显示蓝牙界面
void Show_BT_Info(void)
{
	char buf[32]; // 将变量声明移到函数开头

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
		sprintf(buf, "BEEP will ON in %d.%ds", beep_delay / 10, beep_delay % 10);
		LCD_ShowString(10, 260, 220, 16, 16, (u8 *)buf);
	}

	// 显示LED2状态
	LCD_Fill(10, 290, 220, 306, WHITE);
	sprintf(buf, "LED2: %s", LED2 ? "OFF" : "ON");
	LCD_ShowString(10, 290, 220, 16, 16, (u8 *)buf);

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

// 处理红外遥控器输入
void Process_IR_Command(void)
{
	char buf[32]; // 将变量声明移到函数开头

	if (hw_jsbz)
	{										   // 有红外数据接收到
		printf("IR Code: 0x%08X\r\n", hw_jsm); // 打印接收到的红外代码

		// 根据红外代码控制LED2
		if (hw_jsm == IR_KEY1)
		{			  // 按键1
			LED2 = 0; // 开LED2
			printf("IR Key 1: LED2 ON\r\n");

			// 更新显示
			if (current_mode == 0)
			{
				LCD_Fill(10, 320, 220, 336, WHITE);
				sprintf(buf, "LED2: %s", LED2 ? "OFF" : "ON");
				LCD_ShowString(10, 320, 220, 16, 16, (u8 *)buf);
			}
			else
			{
				LCD_Fill(10, 320, 220, 336, WHITE);
				sprintf(buf, "LED2: %s", LED2 ? "OFF" : "ON");
				LCD_ShowString(10, 320, 220, 16, 16, (u8 *)buf);
			}
		}
		else if (hw_jsm == IR_KEY2)
		{			  // 按键2
			LED2 = 1; // 关LED2
			printf("IR Key 2: LED2 OFF\r\n");

			// 更新显示
			if (current_mode == 0)
			{
				LCD_Fill(10, 320, 220, 336, WHITE);
				sprintf(buf, "LED2: %s", LED2 ? "OFF" : "ON");
				LCD_ShowString(10, 320, 220, 16, 16, (u8 *)buf);
			}
			else
			{
				LCD_Fill(10, 320, 220, 336, WHITE);
				sprintf(buf, "LED2: %s", LED2 ? "OFF" : "ON");
				LCD_ShowString(10, 320, 220, 16, 16, (u8 *)buf);
			}
		}
		else if (hw_jsm == IR_KEY3)
		{ // 按键3 - 进入BEEP设置模式
			beep_setting_mode = 1;
			printf("IR Key 3: BEEP SETTING MODE\r\n");

			// 更新显示
			LCD_Fill(10, 260, 220, 276, WHITE);
			LCD_ShowString(10, 260, 220, 16, 16, (u8 *)"BEEP: SETTING");

			// 显示当前设置的延时时间
			LCD_Fill(10, 290, 220, 306, WHITE);
			sprintf(buf, "Delay: %d seconds", beep_setting_seconds);
			LCD_ShowString(10, 290, 220, 16, 16, (u8 *)buf);
		}
		else if (hw_jsm == IR_KEY4)
		{ // 按键4 - 增加延时时间
			if (beep_setting_mode)
			{
				beep_setting_seconds++;
				printf("IR Key 4: Increase delay to %d seconds\r\n", beep_setting_seconds);

				// 更新显示
				LCD_Fill(10, 290, 220, 306, WHITE);
				sprintf(buf, "Delay: %d seconds", beep_setting_seconds);
				LCD_ShowString(10, 290, 220, 16, 16, (u8 *)buf);
			}
		}
		else if (hw_jsm == IR_KEY5)
		{ // 按键5 - 减少延时时间
			if (beep_setting_mode && beep_setting_seconds > 0)
			{
				beep_setting_seconds--;
				printf("IR Key 5: Decrease delay to %d seconds\r\n", beep_setting_seconds);

				// 更新显示
				LCD_Fill(10, 290, 220, 306, WHITE);
				sprintf(buf, "Delay: %d seconds", beep_setting_seconds);
				LCD_ShowString(10, 290, 220, 16, 16, (u8 *)buf);
			}
		}
		else if (hw_jsm == IR_KEY6)
		{ // 按键6 - 开始倒计时
			if (beep_setting_mode && beep_setting_seconds > 0)
			{
				beep_setting_mode = 0;					// 退出设置模式
				beep_delay = beep_setting_seconds * 10; // 转换为100ms单位
				beep_timer_active = 1;					// 激活定时器

				printf("IR Key 6: Start countdown %d seconds\r\n", beep_setting_seconds);

				// 更新显示
				LCD_Fill(10, 260, 220, 276, WHITE);
				sprintf(buf, "BEEP: COUNTDOWN %d.%ds", beep_delay / 10, beep_delay % 10);
				LCD_ShowString(10, 260, 220, 16, 16, (u8 *)buf);

				// 清除设置时间显示
				LCD_Fill(10, 290, 220, 306, WHITE);
			}
		}
		else if (hw_jsm == IR_KEY7)
		{ // 按键7 - 增大电机功率
			if (motor_power < motor_power_max)
			{
				motor_power += 50;
				if (motor_power > motor_power_max)
					motor_power = motor_power_max;

				// 设置电机功率 - 由于TIM_OCPolarity_Low设置，需要反转PWM逻辑
				// 功率越大，比较值应该越小
				TIM_SetCompare2(TIM3, motor_power_max - motor_power);
				printf("IR Key 7: Increase motor power to %d\r\n", motor_power);

				// 更新显示
				LCD_Fill(10, 350, 220, 366, WHITE);
				sprintf(buf, "Motor Power: %d", motor_power);
				LCD_ShowString(10, 350, 220, 16, 16, (u8 *)buf);
			}
		}
		else if (hw_jsm == IR_KEY8)
		{ // 按键8 - 减小电机功率
			if (motor_power > 0)
			{
				motor_power -= 50;
				if (motor_power < 0)
					motor_power = 0;

				// 设置电机功率 - 由于TIM_OCPolarity_Low设置，需要反转PWM逻辑
				// 功率越小，比较值应该越大
				TIM_SetCompare2(TIM3, motor_power_max - motor_power);
				printf("IR Key 8: Decrease motor power to %d\r\n", motor_power);

				// 更新显示
				LCD_Fill(10, 350, 220, 366, WHITE);
				sprintf(buf, "Motor Power: %d", motor_power);
				LCD_ShowString(10, 350, 220, 16, 16, (u8 *)buf);
			}
		}

		hw_jsbz = 0; // 处理完成，清除标志
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
	char buf[32]; // 显示缓冲区

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

	// 电机PWM初始化 - 使用原始示例中的方法
	TIM3_CH2_PWM_Init(motor_power_max, 72 - 1);			  // 频率约2KHz
	TIM_SetCompare2(TIM3, motor_power_max - motor_power); // 初始功率为0，反转PWM逻辑

	// 红外接收初始化
	Hwjs_Init();

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
						// 更新BEEP状态显示
						LCD_Fill(10, 260, 220, 276, WHITE);
						LCD_ShowString(10, 260, 220, 16, 16, (u8 *)"BEEP: ON");

						Show_Sensor_Info(temp, humi, lsens_value);
					}
					else
					{
						char buf[32];
						LCD_Fill(10, 260, 220, 276, WHITE);
						LCD_ShowString(10, 260, 220, 16, 16, (u8 *)"BEEP: ON");
					}

					// 反馈信息
					if (bt_initialized == 1)
					{
						u3_printf("BEEP is now ON!\r\n");
					}
					printf("BEEP is now ON!\r\n");
				}
				else if (beep_delay % 10 == 0)
				{ // 每秒更新一次显示
					// 更新倒计时显示
					char buf[32];
					LCD_Fill(10, 260, 220, 276, WHITE);
					sprintf(buf, "BEEP: COUNTDOWN %d.%ds", beep_delay / 10, beep_delay % 10);
					LCD_ShowString(10, 260, 220, 16, 16, (u8 *)buf);
				}
			}
		}

		// 处理红外遥控器输入
		Process_IR_Command();

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