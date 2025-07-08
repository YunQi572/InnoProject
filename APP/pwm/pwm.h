#ifndef _pwm_H
#define _pwm_H

#include "system.h"

void TIM3_CH2_PWM_Init(u16 per, u16 psc);
void Motor_PWM_Init(u16 per, u16 psc);
void Motor_On(void);
void Motor_Off(void);
void Motor_SetSpeed(u16 duty);

#endif
