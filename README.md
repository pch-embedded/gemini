# STM32H753ZI FreeRTOS 모터 PWM + 버튼 EXTI (Queue 기반)

STM32H753ZI + FreeRTOS(CMSIS-RTOS2)에서 버튼 EXTI 입력을 큐로 처리하고, 모터 PWM 제어는 MotorTask에서만 수행하도록 분리한 예제입니다.

개요
- 보드/MCU: NUCLEO-H753ZI (STM32H753ZI)
- 목표(현재 단계): 모터 PWM 제어 + 버튼 EXTI 입력을 FreeRTOS 큐로 처리
- OS: FreeRTOS (CMSIS-RTOS2)
- 설계 포인트: ISR에서는 이벤트만 큐로 전달하고, 판정/제어는 Task 레벨에서 처리. 모터 제어는 MotorTask 단독 소유.

동작
- 버튼 짧게: STOP/RUN 토글
- 버튼 길게(800ms 이상): speed step 증가 (0->1->2->0)

설계/구조
- ISR에서 디바운스/판정/모터제어를 수행하면 지연/우선순위/재진입 이슈로 불안정해질 수 있어, 인터럽트는 엣지 이벤트 캡처와 큐 전송만 수행하도록 최소화
- ButtonTask에서 디바운스 후 누름/뗌을 확정하고, 뗌 시점에만 눌린 시간으로 short/long을 판정합니다.
- ButtonTask는 MotorCmd를 motorCmdQ로 전달하고, MotorTask가 해당 명령을 받아 Motor_Run/Stop을 수행합니다.
- Safe boot 정책으로 시작 시 Motor_Stop()을 수행해 예기치 않은 기동을 방지했습니다.

데이터 흐름
1) EXTI ISR (HAL_GPIO_EXTI_Callback)
- msg = (tick << 1) | level 형태로 패킹 후 btnEdgeQ로 전송
- level: 외부 풀다운 기준 누름=1, 뗌=0

2) ButtonTask
- 큐에서 이벤트 수신
- 30ms 디바운스 후 실제 핀 상태로 확정
- 뗌 시점에만 duration 계산 -> short/long 판정
- motorCmdQ로 MotorCmd_t{STOP/RUN, speed_step} 전달

3) MotorTask
- 시작 시 Motor_Stop()
- motorCmdQ 수신 시에만 Motor_Stop() 또는 Motor_Run(step) 실행

하드웨어 설정(현재 코드 기준)
- 버튼: PC13 / EXTI15_10 (외부 풀다운, 누름=HIGH)
- PWM: TIM3 CH1, CH2
- 속도 단계: speed_table = {300, 600, 950} (CCR 값)
- 방향 핀(H-bridge): PB8, PB9 / PD14, PD15

빌드/실행 (STM32CubeIDE)
1. STM32CubeIDE 실행
2. File > Import > Existing Projects into Workspace
3. Build
4. Debug/Run 으로 보드에 Flash

코드 파일
- motor.c/.h: PWM 출력 + 방향핀 제어 캡슐화
- app_state.c/.h: stop_flag/speed_step 및 MotorCmd 타입 정의
- freertos.c: ButtonTask/MotorTask/Queue 생성 및 동작 로직
- main.c: 초기화, TIM3 init 후 Motor_Init(&htim3), EXTI callback에서 큐 전송

- 향후 계획
- 초음파 센서/서보 모터를 추가해 자율 주행 로직으로 확장 예정
