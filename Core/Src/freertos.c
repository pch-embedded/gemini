/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "gpio.h"
#include "app_state.h"
#include "motor.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/*
 * [PORTFOLIO NOTE - 버튼 처리 설계]
 * 1) ISR(EXTI)은 "최소 작업"만 수행:
 *    - tick + level 을 큐에 넣고 바로 리턴 
 *
 * 2) ButtonTask는:
 *    - 큐에서 이벤트를 받고 디바운스 후 안정 상태 변화로 인정
 *    - press 때는 기록만 하고, release 때만 duration 계산해서 short/long 판정
 *      => "누르자마자 반응" 문제 해결(떼는 순간에 판정)
 *
 * 3) MotorTask는:
 *    - 모터 제어의 단일 소유자
 *    - ButtonTask가 만든 MotorCmd를 받아서 Motor_Run/Stop 수행
 *
 * [풀다운 기준]
 * - pressed=1(SET), released=0(RESET)
 */
#define BTN_PORT          GPIOC
#define BTN_PIN           GPIO_PIN_13

#define BTN_DEBOUNCE_MS   (30u)
#define BTN_LONG_MS       (800u)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
volatile uint32_t dbg_btn_msg    = 0;
volatile uint32_t dbg_edge_tick  = 0;
volatile uint32_t dbg_edge_level = 0;
volatile uint32_t dbg_press_tick = 0;
volatile uint32_t dbg_dur_ms     = 0;
/* USER CODE END Variables */

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* USER CODE BEGIN RTOS_THREADS */
/* Definitions for ButtonTask */
osThreadId_t ButtonTaskHandle;
const osThreadAttr_t ButtonTask_attributes = {
  .name = "ButtonTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal1,
};

/* Definitions for MotorTask */
osThreadId_t MotorTaskHandle;
const osThreadAttr_t MotorTask_attributes = {
  .name = "MotorTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* USER CODE END RTOS_THREADS */

/* USER CODE BEGIN RTOS_QUEUES */
/* btnEdgeQ: ISR이 (tick,level)을 보내는 큐 */
osMessageQueueId_t btnEdgeQHandle;
const osMessageQueueAttr_t btnEdgeQ_attributes = {
  .name = "btnEdgeQ"
};

/* motorCmdQ: ButtonTask -> MotorTask 명령 큐 */
osMessageQueueId_t motorCmdQHandle;
const osMessageQueueAttr_t motorCmdQ_attributes = {
  .name = "motorCmdQ"
};
/* USER CODE END RTOS_QUEUES */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
/* USER CODE BEGIN FunctionPrototypes2 */
void StartButtonTask(void *argument);
void StartMotorTask(void *argument);
/* USER CODE END FunctionPrototypes2 */

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  btnEdgeQHandle  = osMessageQueueNew(10, sizeof(uint32_t), &btnEdgeQ_attributes);
  motorCmdQHandle = osMessageQueueNew(10, sizeof(MotorCmd_t), &motorCmdQ_attributes);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  ButtonTaskHandle = osThreadNew(StartButtonTask, NULL, &ButtonTask_attributes);
  MotorTaskHandle  = osThreadNew(StartMotorTask,  NULL, &MotorTask_attributes);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  (void)argument;

  /*
   * [PORTFOLIO NOTE]
   * - defaultTask에서 모터를 직접 제어하던 구조를 뺌
   * - MotorTask만 모터를 제어하도록 분리
   */
  for(;;)
  {
    osDelay(1000);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartButtonTask */
/**
  * @brief  Function implementing the ButtonTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartButtonTask */
void StartButtonTask(void *argument)
{
  /* USER CODE BEGIN StartButtonTask */
  (void)argument;

  /* stable_level: 0=released, 1=pressed (풀다운) */
  uint8_t  stable_level = 0u;
  uint32_t press_tick   = 0u;

  for(;;)
  {
    uint32_t msg;
    osMessageQueueGet(btnEdgeQHandle, &msg, NULL, osWaitForever);

    dbg_btn_msg = msg;

    /* ISR에서 캡처한 tick/level을 사용: Task 지연이 있어도 이벤트 시각은 ISR 기준으로 유지 */
    uint32_t edge_tick = (msg >> 1);
    uint8_t  edge_lvl  = (uint8_t)(msg & 1u); /* 1=pressed, 0=released */

    dbg_edge_tick  = edge_tick;
    dbg_edge_level = edge_lvl;

    /* 디바운스: 잠깐 기다렸다가 실제 핀 상태로 확정 */
    osDelay(BTN_DEBOUNCE_MS);

    uint8_t now_lvl = (HAL_GPIO_ReadPin(BTN_PORT, BTN_PIN) == GPIO_PIN_SET) ? 1u : 0u;

    /* 바운스면 버림 */
    if (now_lvl != edge_lvl) continue;

    /* 이미 안정상태와 같으면 버림 */
    if (now_lvl == stable_level) continue;

    /* 안정상태 갱신 */
    stable_level = now_lvl;

    if (stable_level == 1u)
    {
      /*
       * pressed:
       * - "누르자마자 동작"을 막기 위해 여기서 아무 것도 하지 않고
       *   시간 기준점만 기록한다.
       */
      press_tick     = edge_tick;
      dbg_press_tick = press_tick;
    }
    else
    {
      /*
       * released:
       * - 여기서만 short/long 판정 후 상태 갱신
       * - 사용자가 '떼는 순간'에만 동작이 결정되므로 요구사항 충족
       */
      uint32_t dur = edge_tick - press_tick;
      dbg_dur_ms = dur;

      if (dur >= BTN_LONG_MS)
      {
        /* long: speed만 변경 */
        speed_step = (uint8_t)((speed_step + 1u) % 3u);
      }
      else
      {
        /* short: stop/run 토글 */
        stop_flag ^= 1u;
      }

      /* MotorTask로 명령 전달 (모터 제어는 MotorTask만 수행) */
      MotorCmd_t cmd;
      cmd.speed_step = speed_step;
      cmd.type = (stop_flag != 0u) ? MOTOR_CMD_STOP : MOTOR_CMD_RUN;

      osMessageQueuePut(motorCmdQHandle, &cmd, 0U, 0U);
    }
  }
  /* USER CODE END StartButtonTask */
}

/* USER CODE BEGIN Header_StartMotorTask */
/**
  * @brief  Function implementing the MotorTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartMotorTask */
void StartMotorTask(void *argument)
{
  /* USER CODE BEGIN StartMotorTask */
  (void)argument;

  /*
   * [PORTFOLIO NOTE]
   * - Safe boot: 스케줄러 시작 후에도 모터가 자동으로 돌지 않게 Motor_Stop()부터 수행.
   * - 이후 모든 제어는 motorCmdQ를 통해서만 들어온다.
   */
  Motor_Stop();

  for(;;)
  {
    MotorCmd_t cmd;
    osMessageQueueGet(motorCmdQHandle, &cmd, NULL, osWaitForever);

    if (cmd.type == MOTOR_CMD_STOP)
      Motor_Stop();
    else
      Motor_Run(cmd.speed_step);
  }
  /* USER CODE END StartMotorTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/* USER CODE END Application */
