/*
 * motor.c
 *
 *  Created on: 2025. 12. 27.
 *      Author: parkc
 */
#include "motor.h"

// (1) TIM 핸들 저장용
static TIM_HandleTypeDef *s_htim = 0;


// (2) speed table
static const uint16_t speed_table[3] = {300,600,950};

// (3) Motor_Init: TIM 저장 + PWM Start
void Motor_Init(TIM_HandleTypeDef *htim)
{
	if (htim == 0) return;
	s_htim = htim;
	HAL_TIM_PWM_Start(s_htim, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(s_htim, TIM_CHANNEL_2);
}

void Motor_Run(uint8_t speed_step) {
	if (s_htim == 0) return;
	 HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
	 HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
     HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET);


	uint16_t duty;

	if (speed_step < 3)
		duty = speed_table[speed_step];

	else duty = 0;

	__HAL_TIM_SET_COMPARE(s_htim, TIM_CHANNEL_1, duty);
    __HAL_TIM_SET_COMPARE(s_htim, TIM_CHANNEL_2, duty);
}

void Motor_Stop(void) {
	if(s_htim==0) return;
	 __HAL_TIM_SET_COMPARE(s_htim, TIM_CHANNEL_1, 0);
     __HAL_TIM_SET_COMPARE(s_htim, TIM_CHANNEL_2, 0);
     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8 | GPIO_PIN_9, GPIO_PIN_RESET);
     HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14 | GPIO_PIN_15, GPIO_PIN_RESET);
}
