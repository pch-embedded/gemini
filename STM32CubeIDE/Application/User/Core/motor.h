/*
 * motor.h
 *
 *  Created on: 2025. 12. 27.
 *      Author: parkc
 */

#ifndef APPLICATION_USER_CORE_MOTOR_H_
#define APPLICATION_USER_CORE_MOTOR_H_
#include <stdint.h>
#include "stm32h7xx_hal.h"

void Motor_Init(TIM_HandleTypeDef *htim);
void Motor_Run(uint8_t speed_step);
void Motor_Stop(void);

#endif /* APPLICATION_USER_CORE_MOTOR_H_ */
