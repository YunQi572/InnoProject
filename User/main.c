#include "system.h"
#include "SysTick.h"
#include "lsens.h"
#include "../APP/tftlcd/tftlcd.h"
#include "../APP/dht11/dht11.h"
#include "../APP/rtc/rtc.h"
#include "../APP/key/key.h"
#include "../APP/beep/beep.h"
#include "../APP/beep/music.h"
#include "../APP/hc05/hc05.h"
#include "../APP/hwjs/hwjs.h"
#include "../APP/pwm/pwm.h"
#include "../APP/ws2812/ws2812.h"
#include "../APP/ws2812/rgb_display.h"
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
#define IR_PREV 0x00FF02FD // 上一首按钮
#define IR_NEXT 0x00FFC23D // 下一首按钮

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

// RGB爱心显示控制变量
u16 heart_display_timer = 0; // 爱心显示计时器，单位：100ms
u8 heart_display_active = 0; // 0: 未激活, 1: 已激活

// BEEP设置模式
u8 beep_setting_mode = 0;	  // 0: 非设置模式, 1: 设置模式
u16 beep_setting_seconds = 0; // 设置的延时秒数

// 直流电机控制变量
u16 motor_power = 0;	   // 当前电机功率值(0-500)
u16 motor_power_max = 499; // 最大电机功率值

// 光敏控制电机联动变量
u8 motor_auto_control = 0; // 0: 手动控制, 1: 自动控制
u16 motor_timer = 0;	   // 电机运行计时器，单位：100ms
u8 motor_running = 0;	   // 0: 电机停止, 1: 电机运行
u8 last_light_status = 0;  // 上次光照状态，0: 正常, 1: 暗

// 显示页面控制变量
u8 display_page = 0; // 0: 传感器数据页面, 1: 操作说明页面

char buf[32];

// 数字0-9对应的颜色数组（每个数字都有不同的颜色）
const u32 digit_colors[10] = {
	RGB_COLOR_RED,	   // 0 - 红色
	RGB_COLOR_GREEN,   // 1 - 绿色
	RGB_COLOR_BLUE,	   // 2 - 蓝色
	RGB_COLOR_YELLOW,  // 3 - 黄色
	RGB_COLOR_PURPLE,  // 4 - 紫色
	RGB_COLOR_CYAN,	   // 5 - 青色
	RGB_COLOR_ORANGE,  // 6 - 橙色
	RGB_COLOR_PINK,	   // 7 - 粉色
	RGB_COLOR_MAGENTA, // 8 - 品红色
	RGB_COLOR_LIME	   // 9 - 青柠色
};

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

// 初始化传感器数据显示页面的布局
void Init_Sensor_Info_Layout(void)
{
	// 清屏并显示标题
	LCD_Clear(WHITE);
	LCD_ShowString(50, 10, 150, 16, 16, (u8 *)"Sensor Data");

	// 显示固定的文本标签
	LCD_ShowString(10, 50, 60, 16, 16, (u8 *)"Temp:");
	LCD_ShowString(120, 50, 60, 16, 16, (u8 *)"Humi:");
	LCD_ShowString(10, 80, 60, 16, 16, (u8 *)"Light:");
	LCD_ShowString(10, 110, 70, 16, 16, (u8 *)"Status:");
	LCD_ShowString(10, 140, 100, 16, 16, (u8 *)"Time:");
	LCD_ShowString(10, 170, 60, 16, 16, (u8 *)"BEEP:");
	LCD_ShowString(10, 200, 80, 16, 16, (u8 *)"Volume:");
	LCD_ShowString(10, 230, 220, 16, 16, (u8 *)"Press KEY2 to BT mode");
	LCD_ShowString(10, 260, 60, 16, 16, (u8 *)"BEEP:");
	LCD_ShowString(10, 290, 60, 16, 16, (u8 *)"Delay:");
	LCD_ShowString(10, 320, 60, 16, 16, (u8 *)"LED2:");
	LCD_ShowString(10, 350, 100, 16, 16, (u8 *)"Motor Power:");
	LCD_ShowString(10, 380, 70, 16, 16, (u8 *)"Music:");
	LCD_ShowString(10, 410, 220, 16, 16, (u8 *)"KEY_UP: Switch Page");
}

// 只更新传感器数据页面的动态数值
void Update_Sensor_Values(u8 temp, u8 humi, u8 lsens_value)
{
	char buf[32];

	// 更新温湿度值 - 只刷新数值部分
	LCD_Fill(70, 50, 110, 66, WHITE);
	sprintf(buf, "%2dC", temp);
	LCD_ShowString(70, 50, 40, 16, 16, (u8 *)buf);

	LCD_Fill(180, 50, 220, 66, WHITE);
	sprintf(buf, "%2d%%", humi);
	LCD_ShowString(180, 50, 40, 16, 16, (u8 *)buf);

	// 更新光敏值 - 只刷新数值部分
	LCD_Fill(70, 80, 120, 96, WHITE);
	sprintf(buf, "%3d", lsens_value);
	LCD_ShowString(70, 80, 50, 16, 16, (u8 *)buf);

	// 更新状态指示 - 只刷新状态文本部分
	LCD_Fill(80, 110, 220, 126, WHITE);
	if (temp > 28)
	{
		LCD_ShowString(80, 110, 140, 16, 16, (u8 *)"Hot");
	}
	else if (temp < 15)
	{
		LCD_ShowString(80, 110, 140, 16, 16, (u8 *)"Cold");
	}
	else
	{
		LCD_ShowString(80, 110, 140, 16, 16, (u8 *)"Normal");
	}

	// 更新时间
	LCD_Fill(70, 140, 220, 156, WHITE);
	sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", calendar.w_year, calendar.w_month, calendar.w_date, calendar.hour, calendar.min, calendar.sec);
	LCD_ShowString(70, 140, 150, 16, 16, (u8 *)buf);

	// 更新蜂鸣器状态
	LCD_Fill(70, 170, 120, 186, WHITE);
	sprintf(buf, "%s", beep_status ? "ON" : "OFF");
	LCD_ShowString(70, 170, 50, 16, 16, (u8 *)buf);

	// 更新蜂鸣器音量百分比
	LCD_Fill(90, 200, 150, 216, WHITE);
	sprintf(buf, "%d%%", beep_duty * 100 / beep_period);
	LCD_ShowString(90, 200, 60, 16, 16, (u8 *)buf);

	// 更新BEEP状态或倒计时
	LCD_Fill(70, 260, 220, 276, WHITE);
	if (beep_setting_mode)
	{
		LCD_ShowString(70, 260, 150, 16, 16, (u8 *)"SETTING");

		// 更新当前设置的延时时间
		LCD_Fill(70, 290, 220, 306, WHITE);
		sprintf(buf, "%d seconds", beep_setting_seconds);
		LCD_ShowString(70, 290, 150, 16, 16, (u8 *)buf);
	}
	else if (beep_timer_active)
	{
		sprintf(buf, "COUNTDOWN %d.%ds", beep_delay / 10, beep_delay % 10);
		LCD_ShowString(70, 260, 150, 16, 16, (u8 *)buf);
	}
	else
	{
		sprintf(buf, "%s", beep_status ? "ON" : "OFF");
		LCD_ShowString(70, 260, 150, 16, 16, (u8 *)buf);
	}

	// 更新LED2状态
	LCD_Fill(70, 320, 120, 336, WHITE);
	sprintf(buf, "%s", LED2 ? "OFF" : "ON");
	LCD_ShowString(70, 320, 50, 16, 16, (u8 *)buf);

	// 更新电机功率
	LCD_Fill(110, 350, 170, 366, WHITE);
	sprintf(buf, "%d", motor_power);
	LCD_ShowString(110, 350, 60, 16, 16, (u8 *)buf);

	// 更新音乐播放状态
	LCD_Fill(80, 380, 220, 396, WHITE);
	if (music_status == MUSIC_PLAY)
	{
		extern const char *song_names[];
		sprintf(buf, "%s", song_names[current_song]);
	}
	else if (music_status == MUSIC_PAUSE)
	{
		extern const char *song_names[];
		sprintf(buf, "%s (Paused)", song_names[current_song]);
	}
	else
	{
		sprintf(buf, "Stopped");
	}
	LCD_ShowString(80, 380, 140, 16, 16, (u8 *)buf);

	// 更新光敏控制状态
	if (motor_auto_control)
	{
		LCD_Fill(140, 350, 220, 366, WHITE);
		if (motor_running)
		{
			sprintf(buf, "(AUTO %d.%ds)", motor_timer / 10, motor_timer % 10);
			LCD_ShowString(140, 350, 80, 16, 16, (u8 *)buf);
		}
		else
		{
			LCD_ShowString(140, 350, 80, 16, 16, (u8 *)"(AUTO)");
		}
	}
	else if (motor_running)
	{
		LCD_Fill(140, 350, 220, 366, WHITE);
		LCD_ShowString(140, 350, 80, 16, 16, (u8 *)"(MANUAL ON)");
	}
	else
	{
		LCD_Fill(140, 350, 220, 366, WHITE);
		LCD_ShowString(140, 350, 80, 16, 16, (u8 *)"(OFF)");
	}
}

// 更新修改Show_Sensor_Info函数，使其调用上面两个函数
void Show_Sensor_Info(u8 temp, u8 humi, u8 lsens_value)
{
	// 初始化布局
	Init_Sensor_Info_Layout();

	// 更新数值
	Update_Sensor_Values(temp, humi, lsens_value);
}

// 显示操作说明页面
void Show_Help_Page(void)
{
	// 清屏并显示标题
	LCD_Clear(WHITE);
	LCD_ShowString(50, 10, 150, 16, 16, (u8 *)"Operation Guide");

	// 显示按键操作说明
	LCD_ShowString(10, 40, 220, 16, 16, (u8 *)"KEY0: Toggle Auto Control");
	LCD_ShowString(10, 320, 220, 16, 16, (u8 *)"KEY0 (Long): RGB LED Demo");
	LCD_ShowString(10, 60, 220, 16, 16, (u8 *)"KEY1: Music Play/Pause");
	LCD_ShowString(10, 80, 220, 16, 16, (u8 *)"KEY2: Switch Mode");
	LCD_ShowString(10, 100, 220, 16, 16, (u8 *)"KEY_UP: Switch Page");

	// 显示红外遥控操作说明
	LCD_ShowString(10, 130, 220, 16, 16, (u8 *)"IR3: Toggle Auto Control");
	LCD_ShowString(10, 150, 220, 16, 16, (u8 *)"IR4: Manual Motor Control");
	LCD_ShowString(10, 170, 220, 16, 16, (u8 *)"IR5: Decrease BEEP Delay");
	LCD_ShowString(10, 190, 220, 16, 16, (u8 *)"IR6: Start BEEP+RGB Timer");
	LCD_ShowString(10, 210, 220, 16, 16, (u8 *)"IR7/8: Adjust Motor Power");
	LCD_ShowString(10, 230, 220, 16, 16, (u8 *)"IR9: Emergency Stop");
	LCD_ShowString(10, 250, 220, 16, 16, (u8 *)"IR_PREV/NEXT: Music Control");

	// 显示功能说明
	LCD_ShowString(10, 270, 220, 16, 16, (u8 *)"Auto Control: Light < 20");
	LCD_ShowString(10, 290, 220, 16, 16, (u8 *)"Motor runs for 5 seconds");
	LCD_ShowString(10, 310, 220, 16, 16, (u8 *)"RGB shows countdown+heart");

	// 显示页面切换提示
	LCD_ShowString(10, 340, 220, 16, 16, (u8 *)"KEY_UP: Back to Data Page");
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
	if (hw_jsbz)
	{										   // 有红外数据接收到
		printf("IR Code: 0x%08X\r\n", hw_jsm); // 打印接收到的红外代码

		// 根据红外代码控制电机
		if (hw_jsm == IR_KEY1)
		{ // 按键1 - 增大电机功率
			if (motor_power < motor_power_max)
			{
				motor_power += 50;
				if (motor_power > motor_power_max)
					motor_power = motor_power_max;
				TIM_SetCompare2(TIM3, motor_power_max - motor_power);
				printf("Motor power increased to %d\r\n", motor_power);
			}
		}
		else if (hw_jsm == IR_KEY2)
		{ // 按键2 - 减小电机功率
			if (motor_power > 0)
			{
				if (motor_power > 50)
					motor_power -= 50;
				else
					motor_power = 0;
				TIM_SetCompare2(TIM3, motor_power_max - motor_power);
				printf("Motor power decreased to %d\r\n", motor_power);
			}
		}
		else if (hw_jsm == IR_KEY3)
		{ // 按键3 - 切换光敏自动控制模式
			motor_auto_control = !motor_auto_control;
			if (motor_auto_control)
			{
				printf("Motor auto control enabled\r\n");
				// 切换到自动模式时，停止手动控制
				motor_running = 0;
				TIM_SetCompare2(TIM3, motor_power_max);
			}
			else
			{
				printf("Motor manual control enabled\r\n");
				// 切换到手动模式时，停止自动控制
				motor_running = 0;
				motor_timer = 0;
				TIM_SetCompare2(TIM3, motor_power_max);
			}
		}
		else if (hw_jsm == IR_KEY4)
		{ // 按键4 - 手动控制电机开关
			if (!motor_auto_control)
			{
				motor_running = !motor_running;
				if (motor_running)
				{
					TIM_SetCompare2(TIM3, motor_power_max - motor_power);
					printf("Motor turned ON manually\r\n");
				}
				else
				{
					TIM_SetCompare2(TIM3, motor_power_max);
					printf("Motor turned OFF manually\r\n");
				}
			}
		}

		// 根据红外代码控制LED2
		if (hw_jsm == IR_KEY1)
		{			  // 按键1
			LED2 = 0; // 开LED2
			printf("LED2 ON\r\n");
		}
		else if (hw_jsm == IR_KEY2)
		{			  // 按键2
			LED2 = 1; // 关LED2
			printf("LED2 OFF\r\n");
		}
		else if (hw_jsm == IR_KEY3)
		{ // 按键3 - 进入BEEP设置模式
			beep_setting_mode = 1;
			beep_setting_seconds = 0;
			LCD_ShowString(10, 290, 220, 16, 16, (u8 *)"BEEP Setting Mode");
			printf("Enter BEEP setting mode\r\n");
		}
		else if (hw_jsm == IR_KEY4)
		{ // 按键4 - 增加延时时间
			if (beep_setting_mode)
			{
				beep_setting_seconds += 1;
				if (beep_setting_seconds > 600)
					beep_setting_seconds = 600; // 最大600秒

				sprintf(buf, "Delay: %ds", beep_setting_seconds);
				LCD_ShowString(10, 290, 220, 16, 16, (u8 *)buf);
				printf("BEEP delay set to %d seconds\r\n", beep_setting_seconds);
			}
		}
		else if (hw_jsm == IR_KEY5)
		{ // 按键5 - 减少延时时间
			if (beep_setting_mode && beep_setting_seconds > 0)
			{
				beep_setting_seconds -= 1;
				if (beep_setting_seconds < 0)
					beep_setting_seconds = 0;
				sprintf(buf, "Delay: %ds", beep_setting_seconds);
				LCD_ShowString(10, 290, 220, 16, 16, (u8 *)buf);
				printf("BEEP delay set to %d seconds\r\n", beep_setting_seconds);
			}
		}
		else if (hw_jsm == IR_KEY6)
		{ // 按键6 - 开始倒计时
			if (beep_setting_mode && beep_setting_seconds > 0)
			{
				beep_delay = beep_setting_seconds * 10; // 转换为100ms单位
				beep_timer_active = 1;
				beep_setting_mode = 0;
				LCD_ShowString(10, 290, 220, 16, 16, (u8 *)"BEEP Timer Started!");
				printf("BEEP timer started for %d seconds\r\n", beep_setting_seconds);

				// 初始化RGB LED并显示倒计时起始数字（五彩斑斓效果）
				RGB_LED_Init();
				printf("RGB LED initialized for countdown\r\n");
				if (beep_setting_seconds <= 9)
				{
					RGB_ShowCharNum_Debug(beep_setting_seconds, 0); // 颜色参数已不使用
				}
				else
				{
					// 如果超过9秒，显示个位数
					u8 digit = beep_setting_seconds % 10;
					RGB_ShowCharNum_Debug(digit, 0); // 颜色参数已不使用
				}
			}
		}
		else if (hw_jsm == IR_KEY7)
		{ // 按键7 - 增大电机功率
			if (motor_power < motor_power_max)
			{
				motor_power += 50;
				if (motor_power > motor_power_max)
					motor_power = motor_power_max;
				TIM_SetCompare2(TIM3, motor_power_max - motor_power);
				printf("Motor power increased to %d\r\n", motor_power);
			}
		}
		else if (hw_jsm == IR_KEY8)
		{ // 按键8 - 减小电机功率
			if (motor_power > 0)
			{
				if (motor_power > 50)
					motor_power -= 50;
				else
					motor_power = 0;
				TIM_SetCompare2(TIM3, motor_power_max - motor_power);
				printf("Motor power decreased to %d\r\n", motor_power);
			}
		}
		else if (hw_jsm == IR_KEY9)
		{ // 按键9 - 紧急停止电机
			motor_running = 0;
			motor_auto_control = 0;
			motor_timer = 0;
			TIM_SetCompare2(TIM3, motor_power_max);
			printf("Motor emergency stop\r\n");
		}
		else if (hw_jsm == IR_PREV)
		{ // 上一首按钮 - 切换到上一首歌
			Music_Prev_Song();
			printf("Previous song selected\r\n");
		}
		else if (hw_jsm == IR_NEXT)
		{ // 下一首按钮 - 切换到下一首歌
			Music_Next_Song();
			printf("Next song selected\r\n");
		}

		hw_jsbz = 0; // 清除接收标志
	}
}

// RGB LED Demo Function - Displays the sequence of characters
void RGB_LED_Demo(void)
{
	u8 key_val;
	u8 demo_running = 1;
	u8 current_char = 0;
	const u8 total_chars = 10; // 9 letters + 1 heart
	u16 i;

	printf("Starting RGB LED Demo...\r\n");

	// Initialize RGB LED module
	RGB_LED_Init();

	printf("RGB LED initialized. Displaying character sequence...\r\n");
	printf("Press any key to exit the demo.\r\n");

	// Display each character and heart in sequence, with different colors
	while (demo_running)
	{
		// Display current character with appropriate color
		RGB_ShowCustomChar(custom_char_patterns[current_char], display_colors[current_char]);

		// Move to next character
		current_char = (current_char + 1) % total_chars;

		// Check for key press to exit demo
		for (i = 0; i < 8; i++) // Check for key press for 800ms (8 * 100ms)
		{
			delay_ms(100);
			key_val = KEY_Scan(0);
			if (key_val != 0) // Any key pressed
			{
				demo_running = 0;
				break;
			}
		}
	}

	// Clear RGB LEDs before exiting
	RGB_LED_Clear();
	printf("RGB LED Demo stopped.\r\n");
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
	u16 press_time;
	u8 remaining_seconds; // 倒计时剩余秒数

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

	// 音乐播放模块初始化
	Music_Init();

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

	// 初始化光敏控制变量
	motor_auto_control = 0;							// 默认手动控制
	motor_running = 0;								// 默认电机停止
	motor_timer = 0;								// 计时器清零
	last_light_status = (lsens_value < 20) ? 1 : 0; // 根据初始光照状态设置

	// 显示初始数据 - 第一次显示时，初始化整个布局
	Init_Sensor_Info_Layout();
	Update_Sensor_Values(temp, humi, lsens_value);

	// Display a message that RGB LED demo can be activated with KEY0
	LCD_ShowString(10, 440, 220, 16, 16, (u8 *)"Press KEY0 to start RGB demo");

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

					// 显示五彩斑斓的爱心并启动爱心显示定时器
					RGB_ShowHeart(0);		  // 颜色参数已不使用，自动五彩斑斓
					heart_display_timer = 30; // 显示3秒 (30 * 100ms)
					heart_display_active = 1;

					// 更新显示
					if (current_mode == 0 && display_page == 0)
					{
						// 更新BEEP状态显示
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
					if (current_mode == 0 && display_page == 0)
					{
						char buf[32];
						LCD_Fill(10, 260, 220, 276, WHITE);
						sprintf(buf, "BEEP: COUNTDOWN %d.%ds", beep_delay / 10, beep_delay % 10);
						LCD_ShowString(10, 260, 220, 16, 16, (u8 *)buf);
					}

					// 在RGB模块上显示倒计时数字（五彩斑斓效果）
					remaining_seconds = beep_delay / 10;
					printf("Countdown: %d seconds remaining\r\n", remaining_seconds);
					if (remaining_seconds <= 9)
					{
						RGB_ShowCharNum_Debug(remaining_seconds, 0); // 颜色参数已不使用
					}
					else
					{
						// 如果超过9秒，显示个位数
						u8 digit = remaining_seconds % 10;
						RGB_ShowCharNum_Debug(digit, 0); // 颜色参数已不使用
					}
				}
			}
		}

		// 处理RGB爱心显示定时器
		if (heart_display_active)
		{
			if (heart_display_timer > 0)
			{
				heart_display_timer--;
				if (heart_display_timer == 0)
				{
					// 爱心显示时间结束，清除RGB显示
					RGB_LED_Clear();
					heart_display_active = 0;
					printf("Heart display finished\r\n");
				}
			}
		}

		// 处理红外遥控器输入
		Process_IR_Command();

		// 更新音乐播放
		Music_Update();

		// 光敏控制电机联动逻辑
		if (motor_auto_control)
		{
			// 检查光照状态变化
			if (lsens_value < 20) // 光照低于20，环境较暗
			{
				if (!last_light_status) // 之前是亮的，现在变暗了
				{
					// 启动电机
					motor_running = 1;
					motor_timer = 0;
					TIM_SetCompare2(TIM3, motor_power_max - motor_power);
					printf("Light dim detected, motor started\r\n");
				}
				last_light_status = 1; // 标记当前状态为暗
			}
			else // 光照高于等于20，环境较亮
			{
				if (last_light_status) // 之前是暗的，现在变亮了
				{
					// 停止电机
					motor_running = 0;
					motor_timer = 0;
					TIM_SetCompare2(TIM3, motor_power_max);
					printf("Light bright detected, motor stopped\r\n");
				}
				last_light_status = 0; // 标记当前状态为亮
			}

			// 电机运行计时器
			if (motor_running)
			{
				motor_timer++;
				if (motor_timer >= 50) // 5秒 = 50 * 100ms
				{
					// 5秒后自动关闭电机
					motor_running = 0;
					motor_timer = 0;
					TIM_SetCompare2(TIM3, motor_power_max);
					printf("Motor auto stop after 5 seconds\r\n");
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
					if (display_page == 0)
					{
						Show_Sensor_Info(temp, humi, lsens_value);
					}
					else
					{
						Show_Help_Page();
					}
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
				if (display_page == 0)
				{
					Show_Sensor_Info(temp, humi, lsens_value);
				}
				else
				{
					Show_Help_Page();
				}
			}
		}

		if (current_mode == 0)
		{ // 传感器模式
			if (key_val == KEY_UP_PRESS)
			{
				// KEY_UP: 切换显示页面
				display_page = !display_page;
				if (display_page == 0)
				{
					// 切换到传感器数据页面 - 重新初始化整个布局
					Init_Sensor_Info_Layout();
					Update_Sensor_Values(temp, humi, lsens_value);
				}
				else
				{
					// 切换到操作说明页面
					Show_Help_Page();
				}
			}
			else if (key_val == KEY0_PRESS)
			{
				// Check if KEY0 was held for a long press (>1 second)
				delay_ms(100); // Debounce delay
				if (KEY0 == 0) // Still pressed
				{
					press_time = 0;
					while (KEY0 == 0 && press_time < 10) // Check for up to 1 second
					{
						delay_ms(100);
						press_time++;
					}

					if (press_time >= 10) // Long press - start RGB LED demo
					{
						LCD_Clear(WHITE);
						LCD_ShowString(50, 200, 180, 24, 24, (u8 *)"Starting RGB Demo");
						delay_ms(500);
						RGB_LED_Demo(); // This will enter a loop until a key is pressed
						// After demo, refresh the screen
						if (display_page == 0)
						{
							Show_Sensor_Info(temp, humi, lsens_value);
						}
						else
						{
							Show_Help_Page();
						}
					}
					else // Short press
					{
						// KEY0: 切换光敏自动控制模式
						motor_auto_control = !motor_auto_control;
						if (motor_auto_control)
						{
							printf("Motor auto control enabled\r\n");
							motor_running = 0;
							TIM_SetCompare2(TIM3, motor_power_max);
						}
						else
						{
							printf("Motor manual control enabled\r\n");
							motor_running = 0;
							motor_timer = 0;
							TIM_SetCompare2(TIM3, motor_power_max);
						}
					}
				}
				else // It was a very short press, treat as normal toggle
				{
					motor_auto_control = !motor_auto_control;
					if (motor_auto_control)
					{
						printf("Motor auto control enabled\r\n");
						motor_running = 0;
						TIM_SetCompare2(TIM3, motor_power_max);
					}
					else
					{
						printf("Motor manual control enabled\r\n");
						motor_running = 0;
						motor_timer = 0;
						TIM_SetCompare2(TIM3, motor_power_max);
					}
				}

				// 立即更新显示（只在数据页面时更新）
				if (display_page == 0)
				{
					Update_Sensor_Values(temp, humi, lsens_value);
				}
			}
			else if (key_val == KEY1_PRESS)
			{
				// KEY1: 切换音乐播放/停止
				if (music_status == MUSIC_STOP)
				{
					Music_Play_Song(current_song);
				}
				else if (music_status == MUSIC_PLAY)
				{
					Music_Pause();
				}
				else if (music_status == MUSIC_PAUSE)
				{
					Music_Resume();
				}
				// 立即更新显示（只在数据页面时更新）
				if (display_page == 0)
				{
					Update_Sensor_Values(temp, humi, lsens_value);
				}
			}

			RTC_Get();
			// 读取DHT11温湿度数据
			if (DHT11_Read_Data(&temp, &humi) == 0)
			{
				// DHT11读取成功，只在数据页面时更新显示
				if (display_page == 0)
				{
					Update_Sensor_Values(temp, humi, lsens_value);
				}
			}

			// 读取光敏传感器数据
			lsens_value = Lsens_Get_Val();

			// 更新显示（减少刷新频率，只在数据页面时更新数值，不重新绘制整个屏幕）
			if (t % 20 == 0 && display_page == 0) // 每2秒刷新一次（20 * 100ms = 2000ms）
			{
				Update_Sensor_Values(temp, humi, lsens_value);
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