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
- 데이터 필드는 오프셋 **0 ~ 251**에 배치된다. 220~243, 245~247, 252~255는 현재 일반 기록에 쓰지 않는 자리다.

저수준 Read/Write 함수 (`USER/Drive/DM_HW_Drv.c`):

```c
void DM_HW_Drv_EEPROM_Write(uint16_t wAddrOffset, uint8_t* pBuffer, uint16_t wLength);
void DM_HW_Drv_EEPROM_Read(uint16_t wAddrOffset, uint8_t* pBuffer, uint16_t wLength);
```

쓰기 시 `FLASH_Unlock(FLASH_MemType_Data)` → `FLASH_ProgramByte()` 반복 → `FLASH_Lock(FLASH_MemType_Data)` 순서로 동작. 바이트 단위 접근.

**미기록 상태 처리**: 공장 출하 상태 EEPROM 바이트는 `0xFF`(word는 `0xFFFF`)다. 일반 getter 중에는 이를 0으로 기록하는 함수가 있지만, `M` 덤프 전용 읽기 경로는 전송값만 정규화하고 EEPROM에 쓰지 않도록 구현했다. `FF` 상태에서 실제로 쓰기가 없는지는 아직 실기로 확인하지 않았다(진행사항 문서의 4-1 미검증 항목).

### 1-2. EEPROM 오프셋 맵 (`USER/App/DM_Rom_Handl_App.c` 상단 매크로)

```c
#define ROM_OFFS_INSERT_COUNT           0   /* Offset for Stick Insert Count (1 Byte) */
#define ROM_OFFS_TEST_COMPLETE_COUNT    1   /* Offset for Test Complete Count (1 Byte) */
#define ROM_OFFS_ERROR_STEP             2   /* Offset for Error Step (1 Byte) */
#define ROM_OFFS_ERROR_CODE             3   /* Offset for Error Code (1 Byte) */
#define ROM_OFFS_STICK_PTR_DATA         4   /* Offset for Stick PTR Data Block (16 Bytes / 8 Shorts) */
#define ROM_OFFS_RESULT_DATA            20  /* Six result slots, 32 bytes each */
#define ROM_OFFS_QC_REFERENCE           212 /* Four QC reference words */
#define ROM_OFFS_DUMP_POS               244 /* Latest result slot */
#define ROM_OFFS_WAKE_FAIL_MIN          248 /* Wake-gate failure minimum */
#define ROM_OFFS_WAKE_FAIL_MAX          250 /* Wake-gate failure maximum */
#define ROM_OFFS_RESERVED_TAIL          252 /* Reserved final block */

#define ROM_RESULT_SLOT_COUNT           6   /* DM_Rom_Handl_App.h */
#define ROM_QC_REFERENCE_WORDS          4   /* DM_Rom_Handl_App.h */
#define RESULT_DATA_SIZE                32  /* size of 16 shorts in bytes */
```

| Offset (dec) | 크기 | 필드명 | 타입 | 의미 |
|---|---|---|---|---|
| 0 | 1 byte | `ROM_OFFS_INSERT_COUNT` | `uint8_t` | 스틱 삽입 누적 횟수 |
| 1 | 1 byte | `ROM_OFFS_TEST_COMPLETE_COUNT` | `uint8_t` | 테스트 완료(사용) 누적 횟수. `_nSCAN_MAX_CNT`(30회) 도달 시 `ERROR_OVER_USED` 발생 |
| 2 | 1 byte | `ROM_OFFS_ERROR_STEP` | `uint8_t` | 최근 에러 발생 시퀀스 스텝 |
| 3 | 1 byte | `ROM_OFFS_ERROR_CODE` | `uint8_t` | 최근 에러 코드 (§1-4 표 참고) |
| 4 ~ 19 | 16 bytes (`uint16_t[8]`) | `ROM_OFFS_STICK_PTR_DATA` | `uint16_t[8]` | 스틱 삽입 직후 초기 광학 원시값 4워드(CH0~CH3)와 미사용 0 4워드(`DM_App_Optic_MeasureInitialStick()` 결과) |
| 20 ~ 211 | 192 bytes (6슬롯 × 32 bytes) | `ROM_OFFS_RESULT_DATA` | `uint16_t[6][16]` | 최근 결과 6칸 순환 버퍼 |
| 212 ~ 219 | 8 bytes | `ROM_OFFS_QC_REFERENCE` | `uint16_t[4]` | QC 초기 광학 기준값 CH0~CH3 |
| 220 ~ 243 | 24 bytes | 미사용 | - | 옛 7번째 결과 슬롯의 남은 자리. `Q` 실행 시 `FF`로 지움 |
| 244 | 1 byte | `ROM_OFFS_DUMP_POS` | `uint8_t` | 가장 최근에 저장한 결과 슬롯 번호(0~5) |
| 245 ~ 247 | 3 bytes | 미사용 | - | `Q` 실행 시 `FF`로 지움 |
| 248 ~ 249 | 2 bytes | `ROM_OFFS_WAKE_FAIL_MIN` | `uint16_t` | 초기 CH3 관문 실패값의 최솟값 |
| 250 ~ 251 | 2 bytes | `ROM_OFFS_WAKE_FAIL_MAX` | `uint16_t` | 초기 CH3 관문 실패값의 최댓값 |
| 252 ~ 255 | 4 bytes | `ROM_OFFS_RESERVED_TAIL` | - | 예약 영역. `Q` 실행 시 `FF`로 지움 |

### 1-3. 결과 레코드 내부 구성 (슬롯 하나당 32 bytes = `uint16_t[16]`)

출처: `DM_App_Main_Sq_Save_Result_To_Rom()` (`USER/App/DM_Main_Sq_App.c`)

| word idx | 항목 | word 수 | 상세 |
|---|---|---|---|
| 0~3 | 배율 값 | 4 | `OPTIC_CH_0`~`OPTIC_CH_3` 각 채널의 정수 퍼센트 배율(0~100) |
| 4~7 | Empty Band 데이터 | 4 | 빈 스틱(Blank) 상태 T/BT/BC/C 밴드 ADC 값 (`gs_awEmptyBand_LowData[BAND_MAX]`) |
| 8~11 | Result Band 데이터 | 4 | 실측 T/BT/BC/C 밴드 ADC 값 (`gs_awResultBand_LowData[BAND_MAX]`) |
| 12~14 | 계산 결과 | 3 | `RES_T_INTENSITY`(T라인 강도), `RES_C_INTENSITY`(C라인 강도), `RES_YES_NO`(판정: 1=양성/0=음성) |
| 15 | Error Step / Error Code | 1 | 상위 바이트=에러 발생 스텝, 하위 바이트=에러 코드. 정상 회차는 `0x0000` |

밴드/결과 열거형 (`USER/App/DM_Main_Sq_App.h`):

```c
typedef enum { BAND_T = 0, BAND_BT, BAND_BC, BAND_C, BAND_MAX } BAND_TYPE_t;
typedef enum { RES_T_INTENSITY = 0, RES_C_INTENSITY, RES_YES_NO, RES_MAX } RES_TYPE_t;
```

**주의**: 새 레코드는 `(DumpPos + 1) % 6` 슬롯에 저장하고 그 슬롯 번호를 `DumpPos`에 기록한다. `M` 덤프는 `DumpPos`부터 역순으로 **최신순 6칸**을 보낸다. Insert Count로 슬롯을 역산하지 않는다. `DumpPos`가 미기록(`FF`)이거나 범위(0~5) 밖이면 읽을 때 0으로 보정한다.

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
| `void DM_App_Rom_Get_DumpStatus(uint16_t* pBuffer)` | offset 0~3을 읽어 `FF`만 전송값 0으로 정규화, EEPROM 쓰기 없음 |
| `void DM_App_Rom_Get_StickPtrData(uint16_t* pBuffer)` | offset 4, 16바이트(8 word) |
| `void DM_App_Rom_Set_StickPtrData(const uint16_t* pBuffer)` | offset 4, 16바이트(8 word) |
| `void DM_App_Rom_Get_ResultData(uint8_t bChannel, uint16_t* pBuffer)` | offset 20 + `bChannel*32`, 슬롯 0~5 |
| `void DM_App_Rom_Save_NextResult(const uint16_t* pBuffer)` | `(DumpPos+1)%6` 슬롯에 저장하고 DumpPos 갱신 |
| `void DM_App_Rom_Get_QcReference(uint16_t* pBuffer)` / `Set_QcReference(...)` | offset 212, 8바이트(4 word) |
| `uint8_t DM_App_Rom_Get_DumpPos(void)` | offset 244, 미기록·범위 밖이면 전송용 값 0 |
| `void DM_App_Rom_Get_DumpWakeFail(uint16_t* pMin, uint16_t* pMax)` | offset 248~251, 미기록이면 `65535/0`으로 반환. EEPROM 쓰기 없음 |
| `void DM_App_Rom_Update_WakeFail(uint16_t wADC)` | offset 248~251, 범위가 넓어질 때만 기록 |
| `void DM_App_Rom_Reset_AllData(void)` | `Q` 명령의 256바이트 초기화 |

`M` 덤프에서 `Get_StickPtrData()`가 첫 word `FFFF`를 만나면 출력 버퍼만 0으로 채운다. `Get_DumpWakeFail()`도 `FFFF/FFFF`를 출력에서만 `65535/0`으로 해석한다. 반면 기록 경로의 `Get_WakeFail()`은 초기 최댓값을 EEPROM에 0으로 정규화할 수 있다. **덤프 경로는 이 기록용 getter를 부르지 않는다.**

`Q` 초기화 결과: 0~211은 `00`, 212~243은 `FF`, 244는 `00`, 245~255는 `FF`다. 초기화 전의 미사용 영역은 `FF`라고 단정하지 않는다.

### 1-6. EEPROM에 저장되지 않는 값 (참고)

펌웨어 버전/LOT 번호는 EEPROM이 아니라 컴파일타임 상수(`USER/Parameter_define.h`)이며, USART 덤프 시 헤더로만 첨부된다.

```c
#define _strFIRMWARE_VER        "v2.1"       // 4 byte, 표시용 버전
#define _REAL_FIRMWARE_VER      "v2.1.1.2"   // 8 byte, 실제 빌드 버전
#define _strLOT_UPPER_4CHAR     "HCGM"       // LOT 앞 4자리
#define _strLOT_LOWER_5NUM      "19001"      // LOT 뒤 5자리
```

**버전 규칙**: 덤프 형식을 변경할 때는 `_REAL_FIRMWARE_VER`를 **8바이트 이내에서 갱신**한다. 이 규칙은 다음 형식 변경부터 적용한다. 현재 `v2.1.1.2`는 249바이트 이전의 269·273바이트 형식에도 사용됐으므로, 기존 기기의 형식은 버전 문자열만으로 구분할 수 없다. `v2.1.1.10`처럼 8바이트를 넘으면 전송에서 잘리므로 사용하지 않는다. 이번 문서 갱신에서 펌웨어 버전 값은 변경하지 않는다.

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
1. `'M'`/`'m'`, `'R'`/`'r'`, `'Q'`/`'q'` 수신 시 해당 문자를 내부 버퍼(`static uint8_t bCommandChar`)에 저장.
2. 이어서 CR(0x0D) 또는 LF(0x0A) 수신 시, 저장된 커맨드에 따라 동작 실행 후 버퍼 초기화.
3. 그 외 문자가 수신되면 버퍼를 즉시 리셋(커맨드 무효화).

### 2-3. 커맨드 목록 (전체)

| 커맨드 | 트리거 함수 | 동작 |
|---|---|---|
| `M` / `m` + CR 또는 LF | `DM_App_Main_Sq_Send_RawDump()` | 저장된 값을 249바이트 바이너리 스트림으로 송신 (§2-4) |
| `R` / `r` + CR 또는 LF | `DM_App_Main_Sq_Reset_TestCompleteCount()` | 테스트 완료 카운트를 EEPROM+RAM 모두 0으로 리셋 (`ERROR_OVER_USED` 해제용 서비스 명령) |
| `Q` / `q` + CR 또는 LF | `DM_App_Main_Sq_Restart_QcSession()` | 데이터 EEPROM 초기화, `OK\r\n` 송신 완료 대기 후 MCU 재시작. 기존 기록이 지워지는 개발용 명령 |

체크섬/CRC와 길이 필드는 **없다**. `Q`에만 완료 응답 `OK\r\n`이 있다. GUI 쪽에서 덤프 무결성을 확인하려면 수신 길이와 타임아웃/재시도 로직이 필요하다.

### 2-4. 현재 송신 패킷(`M`/`m` 응답) — 총 249 bytes

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
| 10 | 192 bytes (6×16 word) | 결과 레코드 6개, 최신순 (§1-3 구조 반복) | EEPROM offset 20~211의 슬롯을 순환 조회 |
| 11 | 8 bytes (4 word) | QC 기준값 CH0~CH3 | EEPROM offset 212~219 |
| 12 | 4 bytes (2 word) | Wake Fail 최솟값·최댓값 | EEPROM offset 248~251 |
| **합계** | **249 bytes** | | |

고정폭 문자열 전송 규칙 (`DM_App_Main_Sq_Send_FixedStr`):
- 문자열이 지정 길이보다 짧으면 나머지는 `0x00`으로 패딩.
- 문자열이 지정 길이보다 길면 지정 길이에서 절단.

모든 word(16bit) 필드는 **High byte 먼저** 전송된다 (`DM_HW_Drv_USART_SendWord`).

**구형 호환**: 269바이트 형식은 결과 7칸으로 끝나며 Wake Fail과 QC 기준값이 없다. 273바이트 형식은 결과 7칸 뒤에 Wake Fail 4바이트가 있지만 QC 기준값은 없다. 현재 249바이트 형식은 결과 6칸 뒤에 QC 8바이트와 Wake Fail 4바이트가 온다. 세 형식 모두 `v2.1.1.2` 헤더가 있을 수 있으므로 길이로 구분한다. `multi.html`은 249·269바이트에 도달하면 300ms 동안 추가 수신을 기다리고, 273바이트는 즉시 완료한다.

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
| 237 | 8 | QC Reference[4] (uint16 × 4, big-endian) |
| 245 | 2 | Wake Fail Min (uint16, big-endian) |
| 247 | 2 | Wake Fail Max (uint16, big-endian) |
| **총계** | **249** | |

각 `ResultRecord[n]` 내부 word 순서 (uint16 × 16, big-endian):

| word idx (레코드 내) | 항목 |
|---|---|
| 0~3 | 배율 값 (CH0~CH3) |
| 4~7 | Empty Band (T, BT, BC, C) |
| 8~11 | Result Band (T, BT, BC, C) |
| 12 | T-Line Intensity |
| 13 | C-Line Intensity |
| 14 | 판정 (1=양성/YES, 0=음성/NO) |
| 15 | Error Step / Error Code (상위 바이트 / 하위 바이트) |

---

## 3. GUI 구현 시 참고 사항

- **연결 설정**: 9600-8-N-1, 하드웨어 흐름제어 없음.
- **덤프 요청**: PC → 장치로 ASCII `"M\r\n"` 또는 `"M\n"` 전송(대소문자 무관). 현재 형식은 249바이트이며, 구형 269·273바이트와 길이로 구분한다. 249 또는 269바이트에서 300ms 동안 추가 수신이 없으면 해당 길이로 완료한다.
- **사용 횟수 리셋**: `"R\r\n"` 전송 시 Test Complete Count만 0으로 초기화하며 응답은 없다.
- **QC 세션 재시작**: `"Q\r\n"` 전송 시 EEPROM 전체 초기화 후 `"OK\r\n"` 송신 완료를 기다리고 MCU를 재시작한다. 기존 데이터가 지워지므로 덤프를 먼저 보관한다.
- **파싱 시 주의**:
  - 문자열 필드는 고정 길이이며 끝에 `0x00` 패딩이 포함될 수 있다. 각 필드의 정해진 길이만 읽고 패딩을 제거한다.
  - 모든 다중바이트 정수는 **big-endian**(네트워크 바이트 순서와 동일)으로 디코딩.
  - 덤프의 상태 4개는 EEPROM의 `0xFF`를 전송값 `0`으로, 초기 Stick PTR의 첫 워드가 `0xFFFF`이면 출력 8워드를 `0`으로, Wake Fail이 `0xFFFF/0xFFFF`이면 `65535/0`으로 해석한다. 덤프 읽기 경로에서 EEPROM 쓰기를 제거한 것은 코드 기준이며, `FF` 상태 실기 확인은 미실시다(진행사항 문서의 4-1 미검증 항목 참조).
  - 현재 결과 레코드는 6칸이다. 펌웨어가 DumpPos를 기준으로 최신순으로 전송하므로, UI는 수신 순서를 그대로 사용한다. Insert Count로 슬롯을 역산하지 않는다.
  - 체크섬이 없으므로 수신 길이와 형식을 확인한다. 길이가 249·269·273 중 어느 것도 아니면 재요청한다.
