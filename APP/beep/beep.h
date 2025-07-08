#ifndef _beep_H
#define _beep_H

#include "system.h"

/*  ������ʱ�Ӷ˿ڡ����Ŷ��� */
#define BEEP_PORT GPIOB
#define BEEP_PIN GPIO_Pin_8
#define BEEP_PORT_RCC RCC_APB2Periph_GPIOB

#define BEEP PBout(8)

void BEEP_Init(void);
void BEEP_On(void);
void BEEP_Off(void);
void BEEP_Volume_Increase(void);
void BEEP_Volume_Decrease(void);
void BEEP_Toggle(void);

#endif
