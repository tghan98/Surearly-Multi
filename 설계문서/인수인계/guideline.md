# Project Guideline - MFx New Version (STM8L)

## 1. 개요
본 프로젝트는 STM8L151F3 MCU를 기반으로 하는 펌웨어 개발입니다. "단순성", "중복 방지", "가드레일", "효율성"의 원칙을 따르며, 코드의 가독성과 명확한 명명 규칙을 최우선으로 합니다.

## 2. 개발 원칙
- **가독성 우선:** 코드는 가독성이 좋아야 하며, 복잡한 트릭보다는 명확한 구조를 지향한다.
- **연상 가능 명명법:** 변수 및 함수 이름은 해당 데이터나 동작의 목적을 즉시 연상할 수 있도록 서술적으로 작성한다.
- **언어 규칙:**
    - **Gemini CLI와의 대화(Communication)는 한국어(Korean)를 사용한다.**
    - **코드 주석 및 Doxygen 문서화는 반드시 영어(English)로 작성한다.**
- **펌웨어 가이드라인:**
    - **인터럽트 핸들러 작성 시 `stm8l15x_it.c`를 확인하여 중복되는 핸들러가 있을 경우, 이를 `stm8l15x_it.c`에서 삭제하여 중복 정의를 방지한다.**
    - 변경 코드는 `diff` 형식으로 제공한다.

## 3. 하드웨어 사양 (Project_HW_Define_Describe.txt 기반)
- **MCU:** STM8L151F3 (TSSOP20)
- **System Clock:** 2MHz
- **Timers:**
    - TIM4: 10ms 주기 시스템 타이머 (Sleep/Wakeup 관리)
    - TIM2: LED PWM 제어 (CH2, 5kHz)
- **GPIO:**
    - LED_PWM (PB2): PWM 출력
    - LED1 (PB4), LED2 (PB3): 동작 LED 선택 (Low Active)
    - LCD Pins: COM0 (PC0), SEG (PC5, PC6, PD0, PC0) 등
    - STP_CK (PB1): 스틱 체크 입력
- **ADC:** 3채널 사용 (PB5, PB6, PB7)

## 4. 작업 프로세스
- 모든 수정 사항은 `guideline.md`와 `Project_HW_Define_Describe.txt`를 숙지한 상태에서 진행한다.
- 사용자의 지시 사항에 대해서만 수정을 수행하며, 참고용 파일은 분석에만 활용한다.

## 5. 설계 예외 사항 및 의도 (Architectural Intent)
- **UART RX ISR 내 대량 데이터 전송:** 
    - `USART1_RX_IRQHandler` 내에서 대량의 CSV 데이터를 직접 전송하는 로직은 **의도된 설계**이다.
    - 이는 QA(Quality Assurance) 단계에서 불량으로 입고된 기기의 내부 데이터를 실시간으로 모니터링하기 위한 특수 용도이며, 일반 사용자가 기기를 조작하는 환경에서는 발생하지 않는다.
    - 데이터 전송 중 시스템의 다른 인터럽트(LCD 리프레시 등)가 일시적으로 지연될 수 있으나, 이는 진단 모드에서의 수용 가능한 동작으로 간주한다.
