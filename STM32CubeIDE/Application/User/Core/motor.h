/* motor.h */
#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>
#include "stm32h7xx_hal.h"

/*
 * [PORTFOLIO NOTE]
 * - Motor API는 한 Task만 호출하도록 설계
 * - 하드웨어 레벨은 여기로 캡슐화해서 상위 로직이 하드웨어 세부를 몰라도 되게 한다.
 */

void Motor_Init(TIM_HandleTypeDef* htim);
void Motor_Run(uint8_t speed_step);
void Motor_Stop(void);

#endif /* MOTOR_H */