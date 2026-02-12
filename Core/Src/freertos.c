/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : freertos.c
  * @brief          : FreeRTOS initialization and tasks
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* [중요] 변수와 함수를 쓰기 위해 헤더 포함 */
#include "app_state.h"
#include "motor.h"

/* Private define ------------------------------------------------------------*/
#define LONG_MS 1500     // 장축 기준 시간 (1.5초)

/* USER CODE BEGIN Variables */
/* Global Variables */
osThreadId_t defaultTaskHandle;
osThreadId_t ButtonTaskHandle;
osThreadId_t MotorTaskHandle;
osMessageQueueId_t btnEdgeQHandle;

/* Task Attributes */
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
const osThreadAttr_t ButtonTask_attributes = {
  .name = "ButtonTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal1,
};
const osThreadAttr_t MotorTask_attributes = {
  .name = "MotorTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
const osMessageQueueAttr_t btnEdgeQ_attributes = {
  .name = "btnEdgeQ"
};
/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
void StartDefaultTask(void *argument);
void StartButtonTask(void *argument);
void StartMotorTask(void *argument);

void MX_FREERTOS_Init(void);

/**
  * @brief  FreeRTOS Initialization
  */
void MX_FREERTOS_Init(void) {
  btnEdgeQHandle = osMessageQueueNew(10, sizeof(uint32_t), &btnEdgeQ_attributes);
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);
  ButtonTaskHandle = osThreadNew(StartButtonTask, NULL, &ButtonTask_attributes);
  MotorTaskHandle = osThreadNew(StartMotorTask, NULL, &MotorTask_attributes);
}

/* USER CODE BEGIN Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  for(;;)
  {
    if (stop_flag == 0) Motor_Run(speed_step);
    else Motor_Stop();
    osDelay(10);
  }
}

/* USER CODE BEGIN Header_StartButtonTask */
/**
* @brief Function implementing the ButtonTask thread.
* @note  [Troubleshooting Report - 문제 해결 기록]
* * 1. 초기 문제 :
* - ISR에서 타임스탬프를 큐로 전달하는 방식을 사용했으나,
* - 스위치 채터링과 RTOS 스케줄링 지연으로 인해
* - 단축/장축 판별이 불규칙하게 동작함.
*
* 2. 해결 방안 :
* - ISR은 단순 트리거 역할만 수행하도록 변경.
* - Task 내에서 폴링방식으로 전환.
* - while 루프를 통해 물리적 핀 상태를 직접 추적하여 정확한 Duration 측정.
*/
/* USER CODE END Header_StartButtonTask */
void StartButtonTask(void *argument)
{
  uint32_t msg;

  for(;;)
  {
      // 1. ISR 신호 대기
      if (osMessageQueueGet(btnEdgeQHandle, &msg, NULL, osWaitForever) == osOK)
      {
          // 2. 디바운싱
          osDelay(50);

          // 3. 핀 상태 확인 (PULLDOWN이므로: 누르면 SET(1)이 되어야 함)
          // [수정] RESET -> SET 으로 변경
          if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_SET)
          {
              // --- 눌림 시작 ---
              uint32_t start_tick = osKernelGetTickCount();

              // ★★★ 버튼 감옥 (Blocking Loop) ★★★
              // 버튼이 눌려있는(SET/High) 동안은 여기서 무한 대기
              // [수정] RESET -> SET 으로 변경
              while (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_SET)
              {
                  osDelay(10);
              }

              // --- 손 뗌 (Release) ---

              // 4. 뗌 디바운싱
              osDelay(50);

              // 5. 시간 계산
              uint32_t end_tick = osKernelGetTickCount();
              uint32_t duration = end_tick - start_tick;

              // 6. 동작 수행 (50ms 이상 눌렀을 때만)
              if (duration > 50)
              {
                  if (duration >= LONG_MS)
                  {
                      // 장축 -> 속도 변경
                      speed_step = (speed_step + 1) % 3;
                  }
                  else
                  {
                      // 단축 -> 정지/가동
                      stop_flag ^= 1u;
                  }
              }

              // 7. 큐 비우기
              osMessageQueueReset(btnEdgeQHandle);
          }
      }
  }
}

/* USER CODE BEGIN Header_StartMotorTask */
void StartMotorTask(void *argument)
{
  for(;;)
  {
    osDelay(1000);
  }
}
