# EEPROM 저장 구조 및 USART 전송 프로토콜 명세

> PC용 GUI(수신/파싱 프로그램) 개발을 위한 참고 문서.
> 대상 펌웨어: STM8L15x 계열, 배란/임신 진단기(Surearly Multi) MFx 신버전.
> 관련 소스: `USER/App/DM_Rom_Handl_App.c/.h`, `USER/Drive/DM_HW_Drv.c`, `USER/User_Main.c`, `USER/App/DM_Main_Sq_App.c`, `USER/Parameter_define.h`, `Core/main/main.h`

---

## 1. EEPROM 저장 구조

### 1-1. 하드웨어 개요

- 외부 I2C EEPROM이 아니라 **STM8L15x 내장 Data EEPROM**(FLASH 메모리 영역)을 사용.
- 베이스 주소: `EEPROM_START_ADDR = 0x001000` (`Core/main/main.h`)
- 오프셋 접근 범위는 **0 ~ 255 (256바이트)**로 하드코딩 제한됨 (`DM_HW_Drv.c`의 `if ((wAddrOffset + i) < 256)`).
- 실제 사용 중인 범위: **오프셋 0 ~ 243 (총 244바이트)**.

저수준 Read/Write 함수 (`USER/Drive/DM_HW_Drv.c`):

```c
void DM_HW_Drv_EEPROM_Write(uint16_t wAddrOffset, uint8_t* pBuffer, uint16_t wLength);
void DM_HW_Drv_EEPROM_Read(uint16_t wAddrOffset, uint8_t* pBuffer, uint16_t wLength);
```

쓰기 시 `FLASH_Unlock(FLASH_MemType_Data)` → `FLASH_ProgramByte()` 반복 → `FLASH_Lock(FLASH_MemType_Data)` 순서로 동작. 바이트 단위 접근.

**미기록 상태 처리**: 공장 출하 상태(미기록) EEPROM 바이트는 `0xFF`(또는 word 단위는 `0xFFFF`)로 읽힘. 상위 앱 함수들은 이 값을 감지하면 0으로 초기화 후 재기록하는 guard 로직을 포함한다.

### 1-2. EEPROM 오프셋 맵 (`USER/App/DM_Rom_Handl_App.c` 상단 매크로)

```c
#define ROM_OFFS_INSERT_COUNT           0   /* Offset for Stick Insert Count (1 Byte) */
#define ROM_OFFS_TEST_COMPLETE_COUNT    1   /* Offset for Test Complete Count (1 Byte) */
#define ROM_OFFS_ERROR_STEP             2   /* Offset for Error Step (1 Byte) */
#define ROM_OFFS_ERROR_CODE             3   /* Offset for Error Code (1 Byte) */
#define ROM_OFFS_STICK_PTR_DATA         4   /* Offset for Stick PTR Data Block (16 Bytes / 8 Shorts) */
#define ROM_OFFS_RESULT_DATA            20  /* Offset for Results (7 Channels * 32 Bytes = 224 Bytes) */

#define RESULT_CHANNEL_MAX              7   /* Maximum number of stored results */
#define RESULT_DATA_SIZE                32  /* size of 16 shorts in bytes */
```

| Offset (dec) | 크기 | 필드명 | 타입 | 의미 |
|---|---|---|---|---|
| 0 | 1 byte | `ROM_OFFS_INSERT_COUNT` | `uint8_t` | 스틱 삽입 누적 횟수 |
| 1 | 1 byte | `ROM_OFFS_TEST_COMPLETE_COUNT` | `uint8_t` | 테스트 완료(사용) 누적 횟수. `_nSCAN_MAX_CNT`(30회) 도달 시 `ERROR_OVER_USED` 발생 |
| 2 | 1 byte | `ROM_OFFS_ERROR_STEP` | `uint8_t` | 최근 에러 발생 시퀀스 스텝 |
| 3 | 1 byte | `ROM_OFFS_ERROR_CODE` | `uint8_t` | 최근 에러 코드 (§1-4 표 참고) |
| 4 ~ 19 | 16 bytes (`uint16_t[8]`) | `ROM_OFFS_STICK_PTR_DATA` | `uint16_t[8]` | 스틱 삽입 직후 4채널 × 2 Duty(50%/100%) 초기 광학 캘리브레이션 원시값(`DM_App_Optic_MeasureInitialStick()` 결과) |
| 20 ~ 243 | 224 bytes (7채널 × 32 bytes) | `ROM_OFFS_RESULT_DATA` | `uint16_t[7][16]` | 최근 7회 테스트 결과 레코드 (순환 버퍼) |

### 1-3. 결과 레코드 내부 구성 (32 bytes = `uint16_t[16]`, 채널 1개당)

출처: `DM_App_Main_Sq_Save_Result_To_Rom()` (`USER/App/DM_Main_Sq_App.c`)

| word idx | 항목 | word 수 | 상세 |
|---|---|---|---|
| 0~3 | PWM 값 | 4 | `OPTIC_CH_0`~`OPTIC_CH_3` 각 채널의 Optic PWM 듀티 값 |
| 4~7 | Empty Band 데이터 | 4 | 빈 스틱(Blank) 상태 T/BT/BC/C 밴드 ADC 값 (`gs_awEmptyBand_LowData[BAND_MAX]`) |
| 8~11 | Result Band 데이터 | 4 | 실측 T/BT/BC/C 밴드 ADC 값 (`gs_awResultBand_LowData[BAND_MAX]`) |
| 12~14 | 계산 결과 | 3 | `RES_T_INTENSITY`(T라인 강도), `RES_C_INTENSITY`(C라인 강도), `RES_YES_NO`(판정: 1=양성/0=음성) |
| 15 | Dummy | 1 | 패딩(0x0000) |

밴드/결과 열거형 (`USER/App/DM_Main_Sq_App.h`):

```c
typedef enum { BAND_T = 0, BAND_BT, BAND_BC, BAND_C, BAND_MAX } BAND_TYPE_t;
typedef enum { RES_T_INTENSITY = 0, RES_C_INTENSITY, RES_YES_NO, RES_MAX } RES_TYPE_t;
```

**주의**: 레코드는 `삽입횟수 % 7` (`bIndex % RESULT_CHANNEL_MAX`)로 순환 저장되는 7슬롯 링버퍼이며, 슬롯 인덱스 자체는 시간순이 아니다. 최신순을 파악하려면 `InsertCount`와 대조해야 한다.

### 1-4. 에러 코드 (`ROM_OFFS_ERROR_CODE`에 저장, `DM_Main_Sq_App.h`)

| 코드 | 값 | 의미 |
|---|---|---|
| `ERROR_NONE` | 0x00 | 에러 없음 |
| `ERROR_STICK_REMOVE` | 0x01 | C-Band 변화 이전에 스틱 제거 |
| `ERROR_STICK_FAIL` | 0x02 | C-Band 변화 이후 스틱 제거 또는 기타 실패 |
| `ERROR_OVER_FLOW` | 0x03 | 샘플 과유입 감지 |
| `ERROR_LOW_SAMPLE` | 0x04 | 샘플량 부족 |
| `ERROR_OVER_USED` | 0x05 | 사용 횟수(30회) 초과 |
| `ERROR_C_LINE_FAIL` | 0x06 | 대조선(C-line) 강도 기준 미달 |
| `ERROR_SAMPLE_LOAD_FAIL` | 0x07 | 5분 내 샘플 유입 미감지(타임아웃) |
| `ERROR_OPTIC_FAIL_CH0`~`CH3` | 0x10~0x13 | 채널별 초기 유효성 검증/밝기 튜닝 실패 |

### 1-5. 앱 레벨 Read/Write API (`USER/App/DM_Rom_Handl_App.h/.c`)

| 함수 | 대상 |
|---|---|
| `uint8_t DM_App_Rom_Get_InsertCount(void)` | offset 0 |
| `void DM_App_Rom_Set_InsertCount(uint8_t bCount)` | offset 0 |
| `uint8_t DM_App_Rom_Get_TestCompleteCount(void)` | offset 1 |
| `void DM_App_Rom_Set_TestCompleteCount(uint8_t bCount)` | offset 1 |
| `uint8_t DM_App_Rom_Get_ErrorStep(void)` | offset 2 |
| `void DM_App_Rom_Set_ErrorStep(uint8_t bStep)` | offset 2 |
| `uint8_t DM_App_Rom_Get_ErrorCode(void)` | offset 3 |
| `void DM_App_Rom_Set_ErrorCode(uint8_t bCode)` | offset 3 |
| `void DM_App_Rom_Get_StickPtrData(uint16_t* pBuffer)` | offset 4, 16바이트(8 word) |
| `void DM_App_Rom_Set_StickPtrData(const uint16_t* pBuffer)` | offset 4, 16바이트(8 word) |
| `void DM_App_Rom_Get_ResultData(uint8_t bChannel, uint16_t* pBuffer)` | offset 20 + `bChannel*32`, 채널 0~6 |
| `void DM_App_Rom_Save_CurrentResult(uint8_t bIndex, const uint16_t* pBuffer)` | offset 20 + `(bIndex%7)*32` |

### 1-6. EEPROM에 저장되지 않는 값 (참고)

펌웨어 버전/LOT 번호는 EEPROM이 아니라 컴파일타임 상수(`USER/Parameter_define.h`)이며, USART 덤프 시 헤더로만 첨부된다.

```c
#define _strFIRMWARE_VER        "v2.1"       // 4 byte, 표시용 버전
#define _REAL_FIRMWARE_VER      "v2.1.1.2"   // 8 byte, 실제 빌드 버전
#define _strLOT_UPPER_4CHAR     "HCGM"       // LOT 앞 4자리
#define _strLOT_LOWER_5NUM      "19001"      // LOT 뒤 5자리
```

---

## 2. USART 전송 구조

### 2-1. 물리 계층 스펙

`USER/Drive/DM_HW_Drv.c` `DM_HW_Drv_USART_Init()`:

- 사용 주변장치: **USART1**
- 핀: `SYSCFG_REMAPPinConfig(REMAP_Pin_USART1TxRxPortA, ENABLE)`로 **PA2(TX) / PA3(RX)**에 remap
- Baud rate: **9600**
- 프레임: **8 data bit, stop bit 1, parity 없음 (8N1)**
- 방향: TX/RX 모두 사용. **TX는 폴링 방식**(TC 플래그 대기), **RX는 인터럽트 방식**(`USART_IT_RXNE`)
- 워드(16bit) 전송은 **Big-endian(상위 바이트 먼저)** 순수 바이너리 — ASCII 변환 없음

```c
void DM_HW_Drv_USART_SendByte(uint8_t bData);   // TC 플래그 폴링 후 송신
void DM_HW_Drv_USART_SendWord(uint16_t wVal);   // High byte 먼저, Low byte 나중
```

### 2-2. 수신(커맨드) 프로토콜

파일: `USER/User_Main.c`, 인터럽트 벡터 `USART1_RX_TIM5_CC_IRQHandler` (IRQ #28)

**정식 헤더/길이/CRC 패킷이 아니라 "1개 커맨드 문자 + Enter(CR 0x0D 또는 LF 0x0A)" 형태의 단순 ASCII 라인 커맨드 프로토콜**이다.

동작 방식:
1. `'M'`/`'m'` 또는 `'R'`/`'r'` 수신 시 해당 문자를 내부 버퍼(`static uint8_t bCommandChar`)에 저장.
2. 이어서 CR(0x0D) 또는 LF(0x0A) 수신 시, 저장된 커맨드에 따라 동작 실행 후 버퍼 초기화.
3. 그 외 문자가 수신되면 버퍼를 즉시 리셋(커맨드 무효화).

### 2-3. 커맨드 목록 (전체)

| 커맨드 | 트리거 함수 | 동작 |
|---|---|---|
| `M` / `m` + CR 또는 LF | `DM_App_Main_Sq_Send_RawDump()` | EEPROM 전체 데이터를 269바이트 바이너리 스트림으로 송신 (§2-4) |
| `R` / `r` + CR 또는 LF | `DM_App_Main_Sq_Reset_TestCompleteCount()` | 테스트 완료 카운트를 EEPROM+RAM 모두 0으로 리셋 (`ERROR_OVER_USED` 해제용 서비스 명령) |

체크섬/CRC, 길이 필드, ACK/NACK 응답 등은 **존재하지 않는다**. GUI 쪽에서 무결성 검증이 필요하면 자체적으로 타임아웃/재시도 로직을 둬야 한다.

### 2-4. 송신 패킷(`M`/`m` 응답) 포맷 — 총 269 bytes, 고정 길이

출처: `DM_App_Main_Sq_Send_RawDump()` (`USER/App/DM_Main_Sq_App.c`)

| 순서 | 크기 | 내용 | 원본 |
|---|---|---|---|
| 1 | 4 bytes | 표시용 펌웨어 버전 문자열 (null 패딩/절단, 고정폭) | `_strFIRMWARE_VER` |
| 2 | 8 bytes | 실제 빌드 버전 문자열 | `_REAL_FIRMWARE_VER` |
| 3 | 4 bytes | LOT 앞 4자리 | `_strLOT_UPPER_4CHAR` |
| 4 | 5 bytes | LOT 뒤 5자리 | `_strLOT_LOWER_5NUM` |
| 5 | 2 bytes (1 word) | Insert Count | EEPROM offset 0 |
| 6 | 2 bytes (1 word) | Test Complete Count | EEPROM offset 1 |
| 7 | 2 bytes (1 word) | Error Step | EEPROM offset 2 |
| 8 | 2 bytes (1 word) | Error Code | EEPROM offset 3 |
| 9 | 16 bytes (8 word) | 초기 Stick PTR 캘리브레이션 데이터 | EEPROM offset 4~19 |
| 10 | 224 bytes (7×16 word) | 결과 레코드 7개 (채널 0~6, §1-3 구조 반복) | EEPROM offset 20~243 |
| **합계** | **269 bytes** | | |

고정폭 문자열 전송 규칙 (`DM_App_Main_Sq_Send_FixedStr`):
- 문자열이 지정 길이보다 짧으면 나머지는 `0x00`으로 패딩.
- 문자열이 지정 길이보다 길면 지정 길이에서 절단.

모든 word(16bit) 필드는 **High byte 먼저** 전송된다 (`DM_HW_Drv_USART_SendWord`).

### 2-5. 전체 바이트 오프셋 테이블 (GUI 파서 구현용)

| 바이트 오프셋 | 크기 | 필드 |
|---|---|---|
| 0 | 4 | FIRMWARE_VER (문자열) |
| 4 | 8 | REAL_FIRMWARE_VER (문자열) |
| 12 | 4 | LOT_UPPER (문자열) |
| 16 | 5 | LOT_LOWER (문자열) |
| 21 | 2 | InsertCount (uint16, big-endian) |
| 23 | 2 | TestCompleteCount (uint16, big-endian) |
| 25 | 2 | ErrorStep (uint16, big-endian) |
| 27 | 2 | ErrorCode (uint16, big-endian) |
| 29 | 16 | StickPtrData[8] (uint16 × 8, big-endian) |
| 45 | 32 | ResultRecord[0] (uint16 × 16, big-endian) — 구조는 §1-3 |
| 77 | 32 | ResultRecord[1] |
| 109 | 32 | ResultRecord[2] |
| 141 | 32 | ResultRecord[3] |
| 173 | 32 | ResultRecord[4] |
| 205 | 32 | ResultRecord[5] |
| 237 | 32 | ResultRecord[6] |
| **총계** | **269** | |

각 `ResultRecord[n]` 내부 word 순서 (uint16 × 16, big-endian):

| word idx (레코드 내) | 항목 |
|---|---|
| 0~3 | PWM 값 (CH0~CH3) |
| 4~7 | Empty Band (T, BT, BC, C) |
| 8~11 | Result Band (T, BT, BC, C) |
| 12 | T-Line Intensity |
| 13 | C-Line Intensity |
| 14 | 판정 (1=양성/YES, 0=음성/NO) |
| 15 | Dummy |

---

## 3. GUI 구현 시 참고 사항

- **연결 설정**: 9600-8-N-1, 하드웨어 흐름제어 없음.
- **덤프 요청**: PC → 장치로 ASCII `"M\r\n"` 또는 `"M\n"` 전송(대소문자 무관) 후, 장치가 즉시 269바이트를 순차 송신할 때까지 수신 대기.
- **리셋 요청**: `"R\r\n"` 전송 시 응답 데이터 없음(단순 실행 명령).
- **파싱 시 주의**:
  - 문자열 필드는 중간에 `0x00` 패딩이 포함될 수 있으므로, 널 종료 문자열로 처리.
  - 모든 다중바이트 정수는 **big-endian**(네트워크 바이트 순서와 동일)으로 디코딩.
  - `InsertCount`, `TestCompleteCount`, `StickPtrData` 값이 EEPROM 미기록 상태(0xFF/0xFFFF)로 온다면 이는 실제로는 발생하지 않음 — 펌웨어가 읽기 시점에 0으로 정규화하여 반환하기 때문. 다만 방어적으로 GUI에서도 0xFFFF는 "미측정"으로 표시하는 것을 권장.
  - 결과 레코드 7개는 링버퍼이므로, 최신 레코드를 식별하려면 `InsertCount % 7` (또는 `(InsertCount - 1) % 7`, 마지막으로 쓰여진 인덱스 기준)로 계산 필요 — 정확한 최신 인덱스는 저장 시점 로직(`Save_CurrentResult`가 `bIndex % 7`을 그대로 오프셋에 사용) 기준으로 판단.
  - 체크섬이 없으므로 GUI는 수신 바이트 수가 정확히 269바이트인지로만 최소한의 무결성을 검증할 수 있다(길이 불일치 시 재요청 권장).
