# MFx_New_Ver 펌웨어 계층별(Layered) 분석 리포트

- **분석 대상**: `USER/` (App, Drive, Utill) + `Core/main/` (main.c, stm8l15x_it.c) + `Core/STM8L15x_StdPeriph_Driver` (Vendor HAL)
- **MCU**: STM8L15x (STM8L 시리즈, STM8L15x_StdPeriph_Driver 사용)
- **분석 목적**: 취약점 진단이 아닌 아키텍처 이해 및 계층 구조(HW → HW Driver → Module → Application → System Integration) 검증
- **제품 성격**: 스트립(Stick) 삽입형 광학 측정 진단기(임신테스트기류 정성 판정 장비로 추정 — LOT/T-Line/C-Line 용어 근거)

---

## 1. 하드웨어(HW) 분석

### 1.1 Pin Map

| Pin/Peripheral | 연결 대상 | 트리거 방식 | 외부 연결 여부 | 추정 용도 |
|---|---|---|---|---|
| PB2 (`LED_PWM_GPIO_PIN`) | TIM2_CH2 PWM 출력 | PWM(하드웨어) | 외부 (LED 애노드) | 광학 측정용 LED 밝기(Duty) 제어 |
| PC0 (`LED1_GPIO_PIN`, nLED1) | GPIO Open-Drain 출력 | Polling(GPIO Set/Reset) | 외부 (LED1 캐소드, sink) | LED1 채널 On/Off (LED2와 상호배타) |
| PC1 (`LED2_GPIO_PIN`, nLED2) | GPIO Open-Drain 출력 | Polling | 외부 (LED2 캐소드, sink) | LED2 채널 On/Off |
| PB4 (`LCD_COM0_GPIO_PIN`) | GPIO Push-Pull 출력 | Polling (10ms 주기 위상반전) | 외부 (LCD Common 전극) | LCD 구동 COM 라인 |
| PC5 (`LCD_SEG0`→BOOK) | GPIO Push-Pull 출력 | Polling | 외부 (LCD 세그먼트) | "BOOK" 아이콘 |
| PC6 (`LCD_SEG1`→NO) | GPIO Push-Pull 출력 | Polling | 외부 (LCD 세그먼트) | "NO" 아이콘 |
| PD0 (`LCD_SEG2`→YES) | GPIO Push-Pull 출력 | Polling | 외부 (LCD 세그먼트) | "YES" 아이콘 |
| PB0 (`LCD_SEG3`→DROP) | GPIO Push-Pull 출력 | Polling | 외부 (LCD 세그먼트) | "DROP" (검체 투입) 아이콘 |
| PA2 (`USART_TX_PIN`) | USART1 TX (SYSCFG로 PortA 리맵) | Polling(TC 플래그 대기) | 외부 (서비스/디버그 UART) | 원시 EEPROM 덤프 송신 |
| PA3 (`USART_RX_PIN`) | USART1 RX (SYSCFG로 PortA 리맵) | Interrupt (RXNE) | 외부 (서비스/디버그 UART) | 'M'/'R' 커맨드 수신 |
| PB5 (`ADC_FRNT_PIN`) | ADC1 Channel_13 | Interrupt(EOC) 기반 wfi() 대기 | 외부 (Front 포토센서) | OPTIC_CH_0/1 측정 입력 |
| PB6 (`ADC_MIDL_PIN`) | ADC1 Channel_12 | Interrupt(EOC) | 외부 (Middle 포토센서) | OPTIC_CH_1/2 측정 입력(공유) |
| PB7 (`ADC_REAR_PIN`) | ADC1 Channel_11 | Interrupt(EOC) | 외부 (Rear 포토센서) | OPTIC_CH_3 측정 입력 |
| PB1 (`STP_CK_PIN`) | GPIO 입력 / EXTI_Pin_1 | Interrupt(Falling, Halt Wakeup 전용) / Polling(평상시) | 외부 (스틱 삽입 감지 스위치) | MCU Halt 모드 기상(Wakeup) 트리거 전용 — 지속 상태 판별에는 미사용(아래 3.3 참조) |
| PB3 (`PTR_PW_GPIO`) | GPIO Push-Pull 출력 | Polling | 외부 (PTR 센서 전원 스위치) | 광학 센서 회로 전원 On/Off (측정 시에만 급전) |
| TIM4 | 내부 전용 | Interrupt(Update, 10ms) | 내부 전용 | 시스템 틱(10ms) 및 Halt 기상 타이머 (SysTick 대체) |
| TIM2 | PB2와 연결(위 참조) | - | - | LED PWM 5kHz 생성 |
| Data EEPROM (Flash 내장) | 내부 전용 | Polling(FLASH_WaitForLastOperation) | 내부 전용 | 비휘발성 결과/설정 저장소 (`EEPROM_START_ADDR=0x001000`) |
| DMA1 (Ch0-3) | 미사용 | - | 내부(설정만 포함, 실사용 없음) | `stm8l15x_it.c`에 빈 스텁 핸들러만 존재 — 실제 사용 안 함 |

### 1.2 MCU Peripheral 요약

| Peripheral | 외부 연결 여부 | 비고 |
|---|---|---|
| GPIO (PortA/B/C/D) | 외부 | LED, LCD, ADC 입력, USART, 스틱 감지, PTR 전원 스위치 |
| ADC1 | 외부(3채널 아날로그 입력) | 12-bit, 384 cycle 샘플링(고임피던스 RC 회로 대응), EOC 인터럽트로 Wait 모드 기상 |
| TIM2 | 외부(PWM 출력) | LED 밝기 제어 (Ch2, 5kHz, ARR 399) |
| TIM4 | 내부 전용 | 시스템 틱/저전력 Wakeup 타이머로만 사용, 표준 SysTick 미사용 |
| USART1 | 외부 | 서비스 포트(9600bps), RX는 인터럽트, TX는 폴링 |
| EXTI (Pin1) | 외부 트리거 | Halt 모드에서 스틱 삽입 감지 전용 Wakeup 소스 |
| FLASH(Data EEPROM) | 내부 전용 | 결과/카운트/에러코드 영속 저장 |
| PWR | 내부 전용 | Ultra-Low-Power 레귤레이터, Halt 진입/복귀 제어 |
| SYSCFG | 내부 전용 | USART1 Tx/Rx를 PortC 기본 핀에서 PortA로 리맵 |
| CLK | 내부 전용 | HSI 16MHz ÷ 8 = 2MHz 시스템 클럭 |
| DMA1 | 미사용 | 헤더만 포함, 실제 채널 사용/초기화 코드 없음 |
| IWDG/WWDG | 미사용 | 워치독 관련 호출 전무 (5장 Fail-safe 참조) |

---

## 2. HW 종속 코드 / 드라이버 분석

- 1차 캡슐화 계층은 **`USER/Drive/DM_HW_Drv.c` / `.h` 단일 파일**로 구성되어 있음. 프롬프트가 가정한 `HW_XXX_Drive.c` 형태의 인터페이스별 파일 분할은 되어 있지 않고, GPIO/Clock/TIM2/TIM4/ADC/USART/EEPROM/전원관리를 **하나의 드라이버 파일이 전부 캡슐화**하는 구조.
- 모든 함수가 STM8L15x StdPeriph 라이브러리(`GPIO_Init`, `ADC_Init`, `TIM2_...`, `USART_...`, `FLASH_...`, `PWR_...`)를 직접 호출하는 얇은 래퍼(thin wrapper) 형태.

| 파일/모듈 | 담당 인터페이스 | 대표 함수 예시 | 상태 관리 | MCU 종속도 |
|---|---|---|---|---|
| DM_HW_Drv.c | GPIO 초기화 전반 | `DM_HW_Drv_GPIO_Init()` | 없음 | 매우 높음 |
| DM_HW_Drv.c | TIM2 (LED PWM) | `DM_HW_Drv_LED_Duty_Set()`, `DM_HW_Drv_LED_Control()` | 없음(즉시 레지스터 반영) | 매우 높음 |
| DM_HW_Drv.c | TIM4 (시스템 틱/Sleep) | `DM_HW_Drv_SystemSleep_10ms()` | 없음(정적 지역변수 `dwTickCheck`로 1회성 비교만) | 매우 높음 |
| DM_HW_Drv.c | ADC1 (측정) | `DM_HW_Drv_ADC_Read()`, `_ChannelSelect/Deselect()` | 있음 (`gs_bADC_Conv_Done` — ISR↔Foreground 플래그) | 매우 높음 |
| DM_HW_Drv.c | USART1 (통신) | `DM_HW_Drv_USART_SendByte/SendWord()` | 없음 | 매우 높음 |
| DM_HW_Drv.c | Data EEPROM (Flash) | `DM_HW_Drv_EEPROM_Write/Read()` | 없음(매 호출 Unlock/Lock) | 매우 높음 |
| DM_HW_Drv.c | 전원관리(Halt) | `DM_HW_Drv_Power_PrepareSleep/Resume()`, `SystemHalt_WakeupOnStick()` | 없음 | 매우 높음 |
| DM_HW_Drv.c | 스틱 체크 핀(GPIO 원시입력) | `DM_HW_Drv_STP_CK_Get_Status()` | 없음 | 매우 높음 |

**관찰**:
- 드라이버 계층은 프롬프트 가이드의 전형적 특징(상태 없음, MCU 종속도 매우 높음)을 잘 따르고 있음.
- 유일하게 상태를 갖는 `gs_bADC_Conv_Done`은 `ADC1_COMP_IRQHandler`(User_Main.c, 시스템 계층)가 setter를 호출하고, 드라이버가 getter/clear를 제공하는 형태로 ISR-Foreground 간 동기화 지점 역할을 함 — 계층 경계상 적절한 설계.
- `DM_HW_Drv_SystemSleep_10ms()`, `DM_HW_Drv_ADC_Read()` 내부의 시간 보정 로직(`SystemTick_AddUntracked_us()` 호출)은 드라이버가 System 계층(User_Main.h) 함수를 역참조하는 구조 — 아래 3장/4장에 걸쳐 있는 타이밍 정확도 이슈의 근원이며 계층이 다소 뒤섞여 있음(드라이버가 상위 계층 헤더를 include).

---

## 3. 드라이버 조합 모듈 분석

이 프로젝트에서 "여러 드라이버를 조합해 기능 단위를 구성"하는 모듈 계층에 해당하는 파일은 `DM_Optic_Handle_App.c`, `DM_Rom_Handl_App.c`, `DM_LCD_Stick_Check_App.c` 세 개다(파일명은 `_App`이지만 역할상 Module 계층).

### 3.1 광학 측정 모듈 (`DM_Optic_Handle_App.c`)

- 조합: `DM_HW_Drv_PTR_Power_On/Off()` → `DM_HW_Drv_LED_Duty_Set()` → `DM_HW_Drv_LED_Control()` → `DM_HW_Drv_LED_PWM_Start()` → `DM_HW_Drv_ADC_ChannelSelect()` → `DM_HW_Drv_SystemSleep_10ms()`(안정화 대기) → `DM_HW_Drv_ADC_Read()` ×20(버스트) → `DM_HW_Drv_ADC_ChannelDeselect()` → `DM_HW_Drv_LED_Stop()`/`LED_Control(DISABLE)` → `DM_HW_Drv_PTR_Power_Off()` → `DM_HW_Drv_SystemSleep_10ms()`(회복 대기).
- 20개 샘플을 버블 정렬 후 중간 14개(인덱스 3~16)를 합산해 반환(`DM_App_Optic_Measure`) — 이상치 제거 필터.
- 상태: 채널별 튜닝된 PWM 값 배열(`gs_wOptic_PWM[OPTIC_CH_MAX]`)을 보유. `DM_App_Optic_TuneTargetADC()`는 초기 2점(PWM199/399) 측정값으로 기울기를 추정한 뒤 최대 5회 비례제어(P-control)로 목표 ADC값에 수렴시키는 튜닝 상태 로직을 가짐.

### 3.2 ROM 데이터 모듈 (`DM_Rom_Handl_App.c`)

- 조합: `DM_HW_Drv_EEPROM_Read/Write()`만 사용, 오프셋 상수(`ROM_OFFS_*`)로 InsertCount/TestCompleteCount/ErrorStep/ErrorCode/StickPtrData/ResultData(7채널 순환 저장)를 매핑.
- 상태 관리 없음(단순 주소 매핑 계층). 0xFF/0xFFFF 초기값 감지 시 0으로 자동 초기화하는 로직만 존재.

### 3.3 LCD/Stick 판정 모듈 (`DM_LCD_Stick_Check_App.c`)

- 조합: LCD 세그먼트 제어(테이블 기반 애니메이션 상태머신, `LCD_SQ_STEP_t`)와 스틱 존재 판정(`DM_App_Stick_Get_Status()`, `DM_App_Optic_Check_StickPresent()` 경유)을 함께 담당.
- 상태: `LCD_SQ_STEP_t`(IDLE/STICK_INSERT/…/ERR_*) + 서브스텝 타이머 기반 테이블 구동 애니메이션(`gs_atStickInsertAnim[]` 등). TIM4 ISR에서 10ms마다 `DM_App_LCD_Sq_Process()` 및 `DM_App_LCD_Refresh()`(COM 위상반전 + Blink) 호출.
- **설계 특이사항**: `STP_CK`(PB1) 스위치는 Halt 기상 트리거 전용이며, 실제 "스틱 삽입 여부" 판정은 기계식 스위치의 접촉 불량(mechanical play) 문제로 **OPTIC_CH_3 광학 센서 측정값**으로 대체(`DM_App_Stick_Get_Status()`, 250ms 스로틀 캐시).

**계층 위반 사례**:
- `USER/App/DM_LCD_Stick_Check_App.c:341` — `DM_App_LCD_SEG_Control()`이 `GPIO_WriteBit()`(StdPeriph HAL)를 **직접 호출**, `DM_HW_Drv` 미경유.
- `USER/App/DM_LCD_Stick_Check_App.c:353` — `DM_App_LCD_COM_Control()`도 동일하게 `GPIO_WriteBit()` 직접 호출.
  - → LCD 관련 GPIO 제어만 `DM_HW_Drv_LCD_xxx()` 형태의 드라이버 래퍼가 없고, 이 모듈이 직접 HAL을 호출하는 유일한 예외 지점. 다른 모든 모듈(Optic, Rom)은 `DM_HW_Drv` 경유 원칙을 지키고 있어 이 두 곳만 이례적임.

| 모듈명 | 조합된 드라이버 | 대표 함수 예시 | 상태 관리 | MCU 종속도 |
|---|---|---|---|---|
| 광학 측정 모듈 (DM_Optic_Handle_App) | PTR Power, LED PWM/GPIO, ADC(Select/Read/Deselect), SystemSleep | `DM_App_Optic_Measure(ch, pwm)` | 있음 (채널별 PWM 캐시, 튜닝 반복 카운트) | 낮음 |
| ROM 데이터 모듈 (DM_Rom_Handl_App) | EEPROM Read/Write | `DM_App_Rom_Get_InsertCount()` | 없음 | 낮음 (오프셋 상수만 메모리맵 의존) |
| LCD/Stick 판정 모듈 (DM_LCD_Stick_Check_App) | DM_HW_Drv(간접, Optic 경유) **+ GPIO_WriteBit 직접 호출(위반)** | `DM_App_LCD_Sq_Process()`, `DM_App_Stick_Get_Status()` | 있음 (LCD_SQ_STEP_t 상태머신 + 테이블 애니메이션 + 스틱 캐시) | 중간 (HAL 직접호출로 드라이버 계층 우회) |

---

## 4. 애플리케이션(Application) 분석

제품의 비즈니스 로직(Use Case)을 담당하는 최상위 App 계층은 `DM_Main_Sq_App.c`이며, `MAIN_SQ_STEP_t` 상태머신 하나로 전체 진단 시퀀스를 표현한다.

### 4.1 Use Case 목록

| Use Case | 목적 | 관련 모듈 | 트리거 조건 |
|---|---|---|---|
| 저전력 대기/기상 (IDLE) | Halt 모드로 배터리 절약, 스틱 삽입 시 기상 | DM_HW_Drv(Power/Halt), DM_Optic_Handle_App(기상 후 확인) | STP_CK(PB1) Falling Edge, 또는 위양성 기상 시 재진입 |
| 스틱 삽입/캘리브레이션 (STICK_INSERT) | 사용횟수 검사, 초기 PTR 측정, LED 튜닝, Empty 기준값 확보 | DM_Optic_Handle_App, DM_Rom_Handl_App, DM_LCD_Stick_Check_App | IDLE에서 스틱 삽입 확인 |
| 검체 로딩 대기 (SAMPLE_LOAD_WAIT) | 최대 5분간 T/BT 비율 감시로 검체 투입 감지 | DM_Optic_Handle_App, DM_Utill(CalculateIntensity/Cal_Ratio) | 캘리브레이션 완료 |
| 과량 검체 확인 (OVER_SAMPLE_CHECK) | 5초 관찰 후 C값 급락 여부로 오버플로우 판정 | DM_Optic_Handle_App | 검체 투입 감지 |
| 소량 검체 확인 (LOW_SAMPLE_CHECK) | 20초 대기 후 C값이 충분히 하강했는지 확인 | DM_Optic_Handle_App | 오버플로우 아님 판정 |
| 반응 대기 (REACTION_WAIT) | 총 3분을 채우기 위한 잔여 대기 | - | 소량 검사 통과 |
| 결과 스캔/판정 (RESULT_SCAN) | C/T 밴드 최종 측정·정규화, YES/NO 판정 | DM_Optic_Handle_App, DM_Utill | 3분 반응 완료 |
| 결과 출력/저장 (RESULT_OUT) | 완료카운트 증가, EEPROM 저장, 5분 표시 | DM_Rom_Handl_App | 판정 완료 |
| 에러 처리/복구 (ERROR_WAIT) | 에러코드/스텝/데이터 저장 후 2분 대기, IDLE 복귀 | DM_Rom_Handl_App, DM_LCD_Stick_Check_App | 각 단계의 실패 조건 |
| 서비스 명령 처리 (USART) | 'M'+Enter: EEPROM 원시 덤프 송신 / 'R'+Enter: 사용횟수 리셋 | DM_Rom_Handl_App, DM_HW_Drv(USART) | USART1 RX 인터럽트 |

### 4.2 시퀀스 흐름 예시

```
[스틱 삽입 캘리브레이션]
DM_App_Optic_ResetPWM()
 → DM_App_Rom_Get_InsertCount()/GetTestCompleteCount()
 → (사용횟수 ≥30 → ERROR_OVER_USED)
 → DM_App_Optic_MeasureInitialStick()      // PWM199/399, 8개 값, 범위검사
 → DM_App_Optic_TuneTargetADC()            // 목표 ADC(_nTAGET_LIGHT=800)로 P제어 튜닝, 최대5회
 → DM_App_Optic_MeasureEmptyPTR()          // Empty 기준값 4채널
 → (LCD_SQ_STICK_INSERT 애니메이션 종료 대기)
 → MAIN_SQ_SAMPLE_LOAD_WAIT 전환

[검체 로딩 감지]
GetSystemTick() 시작점 기록
 → (250ms 스틱체크 sleep ×4 반복, 최대 SAMPLE_LOAD_WAIT_TIME(5분))
 → CalculateIntensity(T,BT) 비율이 기준(wFinal_T0_Ratio) 이하로 하락 시 감지
 → BC/C 밴드 추가 측정 → MAIN_SQ_OVER_SAMPLE_CHECK
 → (5분 타임아웃 시 ERROR_SAMPLE_LOAD_FAIL 저장 후 IDLE 복귀)

[결과 판정]
C→BC→BT→T 순서 재측정
 → CalculateIntensity()로 Empty 대비 정규화(BT/T, BC/C)
 → wResult_C < _nC_LINE_INT_THRESHOLD → ERROR_C_LINE_FAIL
 → wResult_T > _nT_LINE_INT_THRESHOLD → YES, 아니면 NO
 → DM_App_LCD_Sq_Set_Step(YES/NO) → MAIN_SQ_RESULT_OUT
```

### 4.3 예외/에러 처리 로직

- **공통 안전장치**: 모든 대기 단계(`SAMPLE_LOAD_WAIT`, `OVER_SAMPLE_CHECK`, `LOW_SAMPLE_CHECK`, `REACTION_WAIT`)는 `DM_App_Main_Sq_Sleep_And_CheckStick_250ms()`로 250ms마다 스틱 제거 여부를 확인하며, 제거 감지 시 즉시 `ERROR_WAIT`로 전이.
- **재시도/타임아웃**: 광학 튜닝은 채널당 최대 5회 반복 후 실패 처리(`FAIL_OPTIC_CHx`); 검체 로딩은 5분(`SAMPLE_LOAD_WAIT_TIME`) 타임아웃; 에러 대기는 2분(`ERR_RUN_TIME`) 후 자동 IDLE 복귀; 사용횟수 초과 시 별도 120초 대기.
- **진단 데이터 보존**: 에러 발생 시점마다 `DM_App_Main_Sq_Save_Result_To_Rom()`으로 PWM/EmptyBand/ResultBand/최종판정값 32바이트를 EEPROM에 스냅샷 저장 → 사후 USART 덤프(`M`+Enter)로 회수 가능.
- **디버그 경로**: `_DEBUG_LCD_SEQ_ONLY` 플래그로 실측정/EEPROM 접근을 건너뛰고 LCD 애니메이션 타이밍만 검증하는 병렬 코드 경로가 각 스텝 핸들러에 `#if/#else`로 내장.

### 4.4 계층 위반 체크

- `DM_Main_Sq_App.c` 자체는 `DM_HW_Drv_*` 함수만을 통해 하드웨어에 접근하며, HAL/레지스터 직접 호출은 발견되지 않음 (양호).
- 다만 구조적으로 눈여겨볼 점 2가지(엄밀한 "위반"은 아니나 계층 경계가 흐릿해지는 지점):
  1. `DM_App_Main_Sq_Idle_Step_Handler()`가 Halt 진입/복귀라는 **시스템 전원관리 책임**까지 Application 계층 파일 안에서 직접 수행 — 별도의 System Power Manager로 분리되어 있지 않음.
  2. `DM_App_Main_Sq_Send_RawDump()`/`Send_FixedStr()`가 통신 프로토콜(헤더 포맷, 워드 순서 정의)을 Application 파일에 직접 구현 — 별도 Comm 모듈 없음.

---

## 5. 시스템 통합(System Integration) 분석

`Core/main/main.c` + `USER/User_Main.c`가 System Integration 계층 역할을 한다. RTOS나 별도 스케줄러는 없고, **`main()`의 `while(1)` 폴링 루프 + 4개의 ISR**로 전체 시스템이 조율되는 구조다.

### 5.1 시스템 상태 다이어그램 (`MAIN_SQ_STEP_t` 기반)

```
IDLE (Halt/Sleep, PB1 Falling Edge로만 기상)
  └─(기상 후 스틱 감지 성공)→ STICK_INSERT
       ├─(사용횟수≥30)──────────────→ ERROR_WAIT(OVER_USED)
       ├─(스틱 제거/측정 실패)──────→ ERROR_WAIT(STICK_REMOVE/STICK_FAIL)
       └─(캘리브레이션 성공)────────→ SAMPLE_LOAD_WAIT
            ├─(5분 타임아웃)───────→ IDLE  (ERROR_SAMPLE_LOAD_FAIL 저장, 즉시 Halt 재진입)
            ├─(스틱 제거)──────────→ ERROR_WAIT
            └─(검체 투입 감지)─────→ OVER_SAMPLE_CHECK
                 ├─(오버플로우)────→ ERROR_WAIT(OVER_FLOW)
                 └─(정상)──────────→ LOW_SAMPLE_CHECK
                      ├─(소량)─────→ ERROR_WAIT(LOW_SAMPLE)
                      └─(정상)─────→ REACTION_WAIT
                           └──────→ RESULT_SCAN
                                ├─(C라인 실패)→ ERROR_WAIT(C_LINE_FAIL)
                                └─(판정 완료)→ RESULT_OUT ──→ IDLE
ERROR_WAIT (2분 대기, 에러정보/스냅샷 저장) ──(타임아웃)──→ IDLE
```

- 이 상태머신과 **독립적으로 병렬 실행**되는 `LCD_SQ_STEP_t` 상태머신이 TIM4 ISR(10ms)에서 구동되며, Main_Sq 쪽이 `DM_App_LCD_Sq_Set_Step()`을 호출해 단방향으로 명령하는 구조(LCD → Main_Sq로의 역방향 커플링 없음, 단 Main_Sq가 `DM_App_LCD_Sq_Get_Step()==IDLE`을 폴링해 애니메이션 종료를 기다리는 동기화 지점 1곳 존재 — Stick Insert 완료 시).

### 5.2 Application 간 조율 / 리소스 충돌

- **단일 스레드 구조**이므로 Main_Sq(측정 시퀀서)와 LCD_Sq(표시 시퀀서) 외에 경쟁하는 별도 Application이 없음 — 별도의 우선순위 조정 로직은 불필요/부재.
- **공유 자원**: ADC1 + LED + PTR 전원 라인은 `DM_App_Optic_Measure()` 호출로 완전히 직렬화되어 사용되므로 동시접근 충돌 없음.
- **잠재적 리소스 충돌 지점(추정, 확인 필요)**: `USART1_RX_TIM5_CC_IRQHandler`(User_Main.c)가 ISR 컨텍스트에서 `DM_App_Main_Sq_Send_RawDump()`(EEPROM 읽기+UART 폴링 송신, 269바이트)나 `DM_App_Main_Sq_Reset_TestCompleteCount()`(EEPROM 쓰기)를 **직접 호출**함. 이 시점에 메인 루프가 마침 EEPROM 접근/측정 시퀀스 중이라면 인터럽트로 끼어들어 동일 자원(EEPROM Flash, 전역 카운터)에 재진입하는 셈인데, 뮤텍스/크리티컬 섹션 보호가 코드상 보이지 않음. 실사용 시나리오(서비스 포트는 보통 유휴 상태에서만 사용)상 실제 충돌 가능성은 낮아 보이나, 정적 분석만으로는 완전히 배제할 수 없어 "확인 필요" 항목으로 남김.

### 5.3 이벤트/스케줄링 구조

- RTOS 없음. 이벤트 소스는 4개 ISR로 한정:
  - `TIM4_UPD_OVF_TRG_IRQHandler` (10ms): 시스템 틱 증가 + `DM_App_LCD_Sq_Process()` + `DM_App_LCD_Refresh()` 직접 호출 — **ISR 내부에서 Module 계층 함수를 직접 실행**하는 구조(전형적인 임베디드 패턴이나, System 계층이 Module 계층을 강하게 소유하는 형태).
  - `ADC1_COMP_IRQHandler`: EOC 플래그 클리어 후 `DM_HW_Drv_ADC_SetConvDone()` 호출 (드라이버 상태 갱신, 계층 준수).
  - `EXTI1_IRQHandler`: pending bit 클리어만 수행(실질적 Wakeup 처리는 Foreground의 `DM_HW_Drv_SystemHalt_WakeupOnStick()` 이후 `wfi()` 복귀 지점에서 처리) — ISR 자체는 최소 동작.
  - `USART1_RX_TIM5_CC_IRQHandler`: 수신 바이트를 파싱해 'M'/'R' 커맨드 완성 시 App 계층 함수를 직접 호출 — 위 5.2의 잠재 이슈.

### 5.4 전원 관리 / Fail-safe

- **저전력 전략**: `MAIN_SQ_IDLE`마다 `DM_HW_Drv_Power_PrepareSleep()`(LED/LCD Off, ADC/USART Disable, Ultra-Low-Power 레귤레이터 Enable, 주변장치 클럭 Disable) → `wfi()` 기반 Halt 진입 → PB1 Falling Edge로만 기상 → `DM_HW_Drv_Power_Resume()`(클럭/GPIO/ADC/USART 재초기화)로 복귀하는 명확한 Sleep/Resume 사이클이 구현되어 있음.
- **Watchdog 부재**: `stm8l15x_iwdg.h`가 `stm8l15x_conf.h`에 포함되어 있으나, 프로젝트 전체에서 IWDG/WWDG 관련 함수 호출이 전혀 없음 — 소프트웨어 무한루프/행(Hang) 상황에 대한 하드웨어적 Fail-safe(자동 리셋)가 **구현되어 있지 않음**.
- **에러 시 동작**: 모든 실패 경로가 `ERROR_WAIT` 상태로 수렴해 에러코드/스텝/측정스냅샷을 EEPROM에 남기고 2분 대기 후 IDLE(Halt)로 복귀 — 이는 "정보 보존형" Fail-safe이며, 하드웨어 워치독을 대체하지는 못함(무한루프 발생 시 이 로직조차 도달하지 못함).

---

## 6. 종합 요약 (Synthesis)

### 6.1 전체 계층 구조 다이어그램

```
┌─────────────────────────────────────────────────────────────────┐
│ 5. System Integration                                           │
│   main.c (while(1)) + User_Main.c                                │
│   - Init 순서 지정, ISR 디스패치(TIM4/ADC1/EXTI1/USART1 RX)      │
│   - GetSystemTick()/SystemTick_AddUntracked_us() 전역 틱 관리     │
└───────────────────────────┬───────────────────────────────────────┘
                             │ 호출
┌───────────────────────────▼───────────────────────────────────────┐
│ 4. Application                                                    │
│   DM_Main_Sq_App.c  — MAIN_SQ_STEP_t 상태머신 (진단 시퀀스 전체)   │
└───────────────────────────┬───────────────────────────────────────┘
                             │ 조립
┌───────────────────────────▼───────────────────────────────────────┐
│ 3. Module (드라이버 조합)                                          │
│   DM_Optic_Handle_App.c  (광학 측정/튜닝)                          │
│   DM_LCD_Stick_Check_App.c (LCD 애니메이션 + 스틱 판정)             │
│   DM_Rom_Handl_App.c (EEPROM 데이터 맵)                            │
│   ※ DM_Utill.c (CalculateIntensity/Cal_Ratio) — 계산 유틸, 4번에서 직접 사용 │
└───────────────────────────┬───────────────────────────────────────┘
                             │ 캡슐화
┌───────────────────────────▼───────────────────────────────────────┐
│ 2. HW Driver                                                       │
│   DM_HW_Drv.c/h  (GPIO/Clock/TIM2/TIM4/ADC1/USART1/EEPROM/전원)     │
└───────────────────────────┬───────────────────────────────────────┘
                             │ 호출
┌───────────────────────────▼───────────────────────────────────────┐
│ 1. Hardware                                                         │
│   STM8L15x MCU + STM8L15x_StdPeriph_Driver(Vendor HAL)              │
│   PB2/PC0/PC1(LED), PB4/PC5/PC6/PD0/PB0(LCD), PA2/PA3(USART),        │
│   PB5/PB6/PB7(ADC), PB1(Stick), PB3(PTR Power)                      │
└─────────────────────────────────────────────────────────────────────┘
```

### 6.2 계층별 핵심 요약

1. **HW**: 3광학채널(ADC)+PWM-LED 2채널+4세그먼트 LCD+서비스 UART+스틱 기상스위치로 구성된 저전력 정성 진단 디바이스. DMA/워치독은 헤더만 포함되고 미사용.
2. **HW Driver**: `DM_HW_Drv.c` 단일 파일이 모든 인터페이스를 캡슐화(인터페이스별 파일 분할 없음). ISR-Foreground 동기화용 상태 플래그(`gs_bADC_Conv_Done`) 외 상태 없음, StdPeriph 직접 호출.
3. **Module**: Optic/Rom 모듈은 계층 원칙을 잘 지키나, LCD 모듈만 `GPIO_WriteBit()` 2곳을 직접 호출(유일한 위반 지점).
4. **Application**: `DM_Main_Sq_App.c` 하나가 9단계 상태머신으로 전체 진단 Use Case(대기→삽입→로딩감시→오버플로우/소량검사→반응→판정→출력/에러)를 조립. HAL 직접 호출 없이 드라이버만 경유(양호).
5. **System Integration**: 별도 RTOS 없이 `main()` 루프 + 4개 ISR로 시스템을 조율. Sleep/Resume 저전력 사이클은 명확하나 하드웨어 워치독 Fail-safe가 없음.

### 6.3 발견된 계층 위반 사례 종합

| # | 위치 | 내용 | 심각도 | 개선 제안 |
|---|---|---|---|---|
| 1 | `USER/App/DM_LCD_Stick_Check_App.c:341` | `DM_App_LCD_SEG_Control()`이 `GPIO_WriteBit()`(HAL) 직접 호출, `DM_HW_Drv` 미경유 | 중 (일관성/이식성 저하, 기능 영향은 낮음) | `DM_HW_Drv_LCD_SEG_Write(port, pin, state)` 래퍼 추가 후 대체 |
| 2 | `USER/App/DM_LCD_Stick_Check_App.c:353` | `DM_App_LCD_COM_Control()`도 동일하게 `GPIO_WriteBit()` 직접 호출 | 중 | 위와 동일하게 `DM_HW_Drv_LCD_COM_Write()` 래퍼로 이전 |
| 3 | `USER/Drive/DM_HW_Drv.c` (전반) | 드라이버가 `User_Main.h`(System 계층)를 include하여 `SystemTick_AddUntracked_us()`를 역호출 | 낮음~중 (구조적 역방향 의존, 타이밍 정확도 이슈와 결합됨) | 시간 보정 로직을 드라이버 밖(Application/System)으로 이동하거나, 콜백/함수포인터로 역방향 의존 제거 |
| 4 | `USER/App/DM_Main_Sq_App.c` (Idle 핸들러) | Application 파일이 전원관리(Halt 진입/복귀)라는 System 책임까지 수행 | 낮음 (기능상 문제 없음, 구조적 관찰) | 별도 System Power Manager로 분리하거나 현재 구조를 의도된 설계로 문서화 |
| 5 | `Core/main/stm8l15x_it.c` / `USER/User_Main.c` (ISR) | `USART1_RX_TIM5_CC_IRQHandler`가 ISR에서 Application 함수(`Send_RawDump`, `Reset_TestCompleteCount`)를 직접 호출, EEPROM 동시접근 보호 없음 | 낮음 (서비스 포트는 통상 유휴 시 사용되어 실사용 리스크는 낮음, 확인 필요) | 커맨드 플래그만 ISR에서 세팅하고 실제 처리는 메인루프에서 수행하도록 변경 검토 |

### 6.4 확실하지 않거나 추가 확인이 필요한 부분

- USART 서비스 커맨드('M'/'R')가 메인 진단 시퀀스 실행 중 수신될 경우의 EEPROM 동시접근 안전성(6.3 #5) — 실기 동작 로그나 사용 시나리오 문서 확인 필요.
- `DM_App_LCD_SEG_Control`/`COM_Control`의 HAL 직접 호출이 의도된 예외(성능/단순성 사유)인지, 단순 누락인지 불명 — 설계 의도 확인 필요.
- `STM8L15x_StdPeriph_Driver` 내 미사용 모듈(AES/BEEP/COMP/DAC/I2C/IRTIM/LCD 컨트롤러/RTC/SPI/WWDG)이 향후 기능 확장을 위해 남겨둔 것인지, 템플릿 잔재인지 불명.
- `DM_Utill.c`는 5계층 어디에도 명시적으로 속하지 않는 순수 계산 유틸리티 — 별도 "Utility/Common" 계층으로 분류할지, Application 계층 하위 요소로 볼지는 팀 컨벤션에 따라 결정 필요.

### 6.5 다음 분석 단계 제안

1. **광학 튜닝 알고리즘 심화 분석**: `DM_App_Optic_TuneTargetADC()`의 P-제어 수렴성(초기 기울기 추정 오차, 5회 반복 한계)을 경계값(스트립 개체차, 온도 등) 관점에서 시뮬레이션 검증.
2. **타이밍 정확도 재검증**: `ARR=149`, `ADC_READ_TIME_US=220` 등 실측 보정값들이 다른 온도/개체 조건에서도 유효한지 후속 실측(메모리에 남아있는 "6초 잔여 오차" 이슈와 연결).
3. **USART ISR-메인루프 동시성 검토**: 6.3 #5 항목에 대한 실기 재현 테스트 또는 코드 리뷰로 실제 위험 여부 확정.
4. **LCD 계층 위반 정리**: `DM_HW_Drv_LCD_*` 래퍼 추가 후 리팩토링, 회귀 테스트로 LCD 애니메이션 타이밍 영향 없음 확인.
5. **워치독 도입 검토**: IWDG 활성화 시 Halt/Sleep 사이클 및 5분/3분 대기 로직과의 상호작용(오탐 리셋 방지) 설계 필요.
