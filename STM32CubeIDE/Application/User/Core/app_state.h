/* app_state.h */
#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdint.h>

/*
 * - Task 간 직접 변수 공유를 최소화하기 위해 "명령 기반으로 MotorTask에 전달한다.
 * - ISR/Task에서 모터 함수를 직접 호출하지 않고, MotorTask만이 모터를 제어
 */

 /* MotorTask에 전달할 명령 타입 */
typedef enum {
    MOTOR_CMD_STOP = 0,
    MOTOR_CMD_RUN = 1,
} MotorCmdType_t;

/* MotorTask로 전달되는 명령 패킷 */
typedef struct {
    MotorCmdType_t type;
    uint8_t speed_step;   /* 0..2 */
} MotorCmd_t;

/*
 * stop_flag / speed_step
 * - UI/상태 확인용 전역 상태(단일 바이트라 원자성 문제는 낮지만,
 *   정책적으로는 ButtonTask가 갱신, MotorTask는 큐 기반으로 동작이 핵심)
 */
extern volatile uint8_t stop_flag;   /* 1=STOP, 0=RUN */
extern volatile uint8_t speed_step;  

#endif /* APP_STATE_H */