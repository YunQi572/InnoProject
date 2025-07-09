#include "beep.h"
#include "music.h"
#include "SysTick.h"

// PWM parameters for volume control
u16 beep_period = 100;
u16 beep_duty = 50; // Default volume (0-100)
u8 beep_status = 0; // 0: off, 1: on

/*******************************************************************************
 *            : BEEP_Init
 * 		   : ʼ
 *              :
 *              :
 *******************************************************************************/
void BEEP_Init(void) // ˿ڳʼ
{
	GPIO_InitTypeDef GPIO_InitStructure; // һṹʼGPIO
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;

	RCC_APB2PeriphClockCmd(BEEP_PORT_RCC, ENABLE); /* GPIOʱ */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	/*  GPIOģʽIO */
	GPIO_InitStructure.GPIO_Pin = BEEP_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(BEEP_PORT, &GPIO_InitStructure);

	// Configure Timer for PWM
	TIM_TimeBaseInitStructure.TIM_Period = beep_period - 1;
	TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1; // 72MHz / 72 = 1MHz
	TIM_TimeBaseInitStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseInitStructure);

	// Configure PWM channel - PB8 is TIM4_CH3
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0; // Initially off
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC3Init(TIM4, &TIM_OCInitStructure);
	TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Enable);

	TIM_ARRPreloadConfig(TIM4, ENABLE);
	TIM_Cmd(TIM4, ENABLE);

	beep_status = 0; // Beeper off by default
}

/*******************************************************************************
 *            : BEEP_On
 * 		   :
 *              :
 *              :
 *******************************************************************************/
void BEEP_On(void)
{
	if (beep_status == 0)
	{
		// 检查是否正在播放音乐，如果是则不启动普通蜂鸣器
		extern u8 music_status;
		if (music_status == MUSIC_STOP)
		{
			TIM_SetCompare3(TIM4, beep_duty);
			beep_status = 1;
		}
	}
}

/*******************************************************************************
 *            : BEEP_Off
 * 		   :ر
 *              :
 *              :
 *******************************************************************************/
void BEEP_Off(void)
{
	TIM_SetCompare3(TIM4, 0);
	beep_status = 0;
}

/*******************************************************************************
 *            : BEEP_Volume_Increase
 * 		   :
 *              :
 *              :
 *******************************************************************************/
void BEEP_Volume_Increase(void)
{
	if (beep_duty < beep_period - 10)
	{
		beep_duty += 10;
		if (beep_status == 1)
		{
			TIM_SetCompare3(TIM4, beep_duty);
		}
	}
}

/*******************************************************************************
 *            : BEEP_Volume_Decrease
 * 		   : С
 *              :
 *              :
 *******************************************************************************/
void BEEP_Volume_Decrease(void)
{
	if (beep_duty >= 10)
	{
		beep_duty -= 10;
		if (beep_status == 1)
		{
			TIM_SetCompare3(TIM4, beep_duty);
		}
	}
}

/*******************************************************************************
 *            : BEEP_Toggle
 * 		   :
 *              :
 *              :
 *******************************************************************************/
void BEEP_Toggle(void)
{
	if (beep_status == 0)
	{
		BEEP_On();
	}
	else
	{
		BEEP_Off();
	}
}
