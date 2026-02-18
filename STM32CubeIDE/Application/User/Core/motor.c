/* motor.c */
#include "motor.h"

/*
 * - PWM 출력과 방향핀 제어를 한 파일로 묶어 구동부를 모듈화.
 * - 상위 로직은 speed_step(0..2)만 던지면 된다.
 */

/* TIM 핸들 저장용 */
static TIM_HandleTypeDef *s_htim = 0;

/* speed table (CCR 값) */
static const uint16_t speed_table[3] = {300, 600, 950};

void Motor_Init(TIM_HandleTypeDef *htim)
{
    if (!htim) return;

    s_htim = htim;

    /* PWM Start */
    HAL_TIM_PWM_Start(s_htim, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(s_htim, TIM_CHANNEL_2);
}

void Motor_Run(uint8_t step)
{
    if (!s_htim) return;

    /* 방향핀 (H-bridge 입력) */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8,  GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET);

    uint16_t duty = (step < 3) ? speed_table[step] : 0;

    __HAL_TIM_SET_COMPARE(s_htim, TIM_CHANNEL_1, duty);
    __HAL_TIM_SET_COMPARE(s_htim, TIM_CHANNEL_2, duty);
}

void Motor_Stop(void)
{
    if (!s_htim) return;

    /* PWM 0% */
    __HAL_TIM_SET_COMPARE(s_htim, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(s_htim, TIM_CHANNEL_2, 0);

    /* 방향핀도 LOW로 내려서 완전 정지 */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8 | GPIO_PIN_9, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14 | GPIO_PIN_15, GPIO_PIN_RESET);
}
