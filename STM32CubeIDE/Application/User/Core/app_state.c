/* app_state.c */
#include "app_state.h"

/*
 * - Safe boot 정책: 전원 인가/리셋 직후는 무조건 STOP으로 시작.
 * - 예상치 못한 모터 기동 방지.
 */
volatile uint8_t stop_flag  = 1;
volatile uint8_t speed_step = 0;