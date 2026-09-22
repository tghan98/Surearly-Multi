# PTR 보정 알고리즘 설명서

**작성일:** 2026-05-11  
**프로젝트:** Surearly Multi / MFx_New_Ver  

---

## 1. 하드웨어 구성

### PTR / LED 배치

```
[Sample pad] ──── C_Blank ──── C_Band ──── T_Blank ──── T_Band ──── [Absorbent pad]
                     │              │            │             │
                   PTR1           PTR2         PTR2          PTR3
                 (Front)        (Middle)      (Middle)      (Rear)
                     │              │            │             │
                   LED2           LED2          LED1          LED1
                    ↑              ↑             ↑             ↑
                  CH_0           CH_1          CH_2           CH_3
```

> PTR2(Middle)는 LED2와 LED1을 교대로 켜서 C_Band와 T_Blank 2개 위치를 측정  
> → PTR 3개로 4개 위치를 측정하는 핵심 설계

### 채널 매핑

| 채널 | LED | PTR (ADC) | 측정 위치 |
|------|-----|-----------|----------|
| CH_0 | LED2 | PTR_Front (ADC_IDD_FRNT) | C Band Blank |
| CH_1 | LED2 | PTR_Middle (ADC_IDD_MIDL) | C Band |
| CH_2 | LED1 | PTR_Middle (ADC_IDD_MIDL) | T Band Blank |
| CH_3 | LED1 | PTR_Rear (ADC_IDD_REAR) | T Band |

---

## 2. 측정 원리

### 전개 전 / 전개 후 반사광 변화

```
반사광 ADC값
   HIGH ┌─────────────────────────────────────── 전개 전 (dry membrane)
        │
        │  C_Blank, T_Blank ─────────────────── 완만한 감소 (wetting만)
        │
        │  C_Band ───────────────────────────── 중간 감소 (wetting + 발색)
        │
   LOW  └  T_Band (양성) ──────────────────────  큰 감소 (wetting + 발색 + 농도)
        ──────────────────────────────────────▶ 전개 시간
```

### 채널별 반응 특성이 다른 이유

| 요인 | 내용 |
|------|------|
| **광학 메커니즘 차이** | Blank: 멘브레인 wetting 효과만 / Band: wetting + 금·라텍스 발색입자 흡광 |
| **발색 의존성** | C Band: 항상 발색(검사 유효성 지표) / T Band: 분석물 농도에 비례 |
| **전개 타이밍** | 멘브레인이 Sample pad → Absorbent pad 방향으로 순차적으로 젖음 |
| **광학계 구조** | LED-PTR 거리·각도가 채널마다 다름 → 단위 반사율 변화당 ADC 감도 차이 |

→ **채널마다 기울기(반응률)가 다른 것은 정상**이며, 보정 알고리즘이 이를 처리

---

## 3. PTR 보정 알고리즘 전체 흐름

```
스틱 삽입
    │
    ▼
[Phase 1] 초기 PTR 측정 (MeasureInitialStick)
  → 4채널 × 2 PWM 레벨(저/고) 측정
  → 선형 응답 특성 파악
    │
    ▼
[Phase 2] PWM 자동 튜닝 (TuneTargetADC)
  → 각 채널 ADC가 목표값(TARGET_ADC)에 도달하도록 PWM 개별 조정
  → 채널 간 절대값 차이를 소거 → 동일한 0점 기준 확보
    │
    ▼
[Phase 3] 0점 측정 (MeasureEmptyPTR)
  → 튜닝된 PWM으로 전개 전 기준값(Baseline) 저장
    │
    ▼
[전개 진행 - 멘브레인이 젖고 Band 형성]
    │
    ▼
[Phase 4] 전개 후 신호 측정
  → 동일 채널·동일 PWM으로 재측정
    │
    ▼
[Phase 5] 신호 계산
  → Blank 보정 → Band 신호 추출 → 판정
  → [방식 A] Delta(차이값) : ΔADC[Blank] - ΔADC[Band]
  → [방식 B] Ratio(비율)   : (ADC_post/ADC_pre)[Band] ÷ (ADC_post/ADC_pre)[Blank]
```

---

## 4. Phase 1 : 초기 PTR 측정

### 목적
각 채널의 PTR이 광량에 어떻게 반응하는지 선형 특성을 파악

### 방법
2개의 PWM 레벨(저: 199, 고: 399)에서 ADC 측정

```
ADC
 │           × ADC_High (PWM=399)
 │          /
 │         /   ← 선형 응답 구간
 │        /
 │       × ADC_Low (PWM=199)
 │
 └─────────────────── PWM
       199    399
```

### 코드 대응
```c
const uint16_t awPWM_Set[2] = {199, 399};
// 4채널 × 2 PWM = 8회 측정 (총 8 × 200ms = 1,600ms)
DM_App_Optic_MeasureInitialStick(gs_awInitialStickPtr);
```

### 출력
`gs_awInitialStickPtr[8]` 배열에 저장
```
Index:  0      1      2      3      4      5      6      7
      CH0_L  CH0_H  CH1_L  CH1_H  CH2_L  CH2_H  CH3_L  CH3_H
      (199)  (399)  (199)  (399)  (199)  (399)  (199)  (399)
```

---

## 5. Phase 2 : PWM 자동 튜닝

### 목적
채널마다 다른 PTR 감도·LED 광량 편차를 PWM으로 보정하여  
**모든 채널의 ADC 출력을 동일한 목표값(TARGET_ADC)으로 정규화**

### 알고리즘

#### Step 1 : 선형 보간으로 초기 PWM 추정
```
                  (TARGET_ADC - ADC_Low)
PWM_est = 199 + ──────────────────────── × (399 - 199)
                  (ADC_High - ADC_Low)
```

#### Step 2 : P-제어 반복 보정 (최대 5회)
```
for iter = 0 to 4:
    ADC_measured = DM_App_Optic_Measure(CH, PWM_current)
    error = TARGET_ADC - ADC_measured

    if |error| ≤ TOLERANCE:
        break  ← 수렴, 완료

    PWM_next = PWM_current + error × (PWM_delta / ADC_delta)
    PWM_next = clamp(PWM_next, 0, 399)
```

#### 수렴 과정 예시
```
Iter 0: PWM=250, ADC=18500, error=+1500 → PWM+=75 → PWM=325
Iter 1: PWM=325, ADC=19800, error=+200  → PWM+=10 → PWM=335
Iter 2: PWM=335, ADC=20050, error=-50   ← TOLERANCE 이내, 완료
```

### 튜닝 결과
채널별 최적 PWM이 `gs_wOptic_PWM[4]`에 저장됨

```
예시:
CH_0: PWM = 312  (PTR1이 감도 낮아 밝게)
CH_1: PWM = 287  
CH_2: PWM = 301  
CH_3: PWM = 334  (PTR3이 감도 높아 어둡게)
```

→ **PWM 튜닝 후 4채널 모두 동일한 ADC값에서 시작** → 공정한 비교 기준 확보

### 소요 시간
- 최선: 4채널 × 1회 × 200ms = **800ms**
- 평균: 4채널 × 2회 × 200ms = **1,600ms**
- 최악: 4채널 × 5회 × 200ms = **4,000ms**

---

## 6. Phase 3 : 0점(Baseline) 측정

### 목적
멘브레인이 젖기 전 상태를 채널별 기준값(0점)으로 저장  
→ 전개 후 변화량 계산의 기준

### 방법
Phase 2에서 확정된 각 채널의 최적 PWM으로 즉시 측정

```c
// 4채널 × 1회 × 200ms = 800ms
DM_App_Optic_MeasureEmptyPTR(gs_awEmptyBand_LowData);
```

### 출력
```
gs_awEmptyBand_LowData[4]:
  [0] = CH_0 (C Blank) baseline
  [1] = CH_1 (C Band)  baseline
  [2] = CH_2 (T Blank) baseline
  [3] = CH_3 (T Band)  baseline
```

---

## 7. Phase 4 : 전개 후 측정

동일 채널·동일 PWM으로 재측정하여 전개 후 ADC값 확보

```c
// 4채널 × 1회 × 200ms = 800ms
// gs_awEmptyBand_LowData[4] = ADC_pre  (Phase 3에서 저장됨)
// awPostDev[4]              = ADC_post (이번 측정)
DM_App_Optic_MeasurePostDev(awPostDev);
```

```
gs_awEmptyBand_LowData[4] : ADC_pre  (전개 전, Phase 3)
awPostDev[4]              : ADC_post (전개 후, Phase 4)

  채널    ADC_pre   ADC_post   변화
  CH_0    20,000    19,600     -400  (C Blank, wetting만)
  CH_1    20,000    16,000   -4,000  (C Band, 발색)
  CH_2    20,000    19,400     -600  (T Blank, wetting만)
  CH_3    20,000    14,000   -6,000  (T Band, 발색 + 농도)
```

---

## 8. Phase 5-A : Delta(차이값) 방식 신호 계산

### 계산식

```
ΔADC[n] = ADC_pre[n] - ADC_post[n]    (반사광 감소량, 양수)

T_signal = ΔADC[T_Blank] - ΔADC[T_Band]
C_signal = ΔADC[C_Blank] - ΔADC[C_Band]
```

### 예시

```
ΔADC[C_Blank] =  400
ΔADC[C_Band]  = 4,000  →  C_signal = 400 - 4,000 = -3,600
ΔADC[T_Blank] =  600
ΔADC[T_Band]  = 6,000  →  T_signal = 600 - 6,000 = -5,400
```

> T_signal 음수의 절댓값이 클수록 T Band 발색 강함 → 판정 임계값 설정

### 판정 로직

```
C_signal 유효 여부 확인
    │
    ├─ |C_signal| < C_THRESHOLD → 검사 실패 (C Band 미발색)
    │
    └─ |C_signal| ≥ C_THRESHOLD
            │
            ├─ |T_signal| ≥ T_THRESHOLD → 양성 (Positive)
            │
            └─ |T_signal| < T_THRESHOLD → 음성 (Negative)
```

### 특성

| 장점 | 단점 |
|------|------|
| 정수 덧셈/뺄셈만 사용 | PWM 튜닝 정밀도에 의존 |
| 계산 빠름 (STM8L 유리) | LED 광량 변화 시 기준값 흔들림 |
| 구현 단순 | 채널 간 ADC 스케일 차이 영향 |

---

## 9. Phase 5-B : Ratio(비율) 방식 신호 계산

### 원리

Delta는 절대 변화량이므로 초기 ADC값 크기에 영향을 받음.  
Ratio는 **각 채널 자신의 기준값 대비 변화율**이므로  
PWM 튜닝 정밀도나 LED 광량 드리프트에 강건함.

```
Ratio[n] = ADC_post[n] / ADC_pre[n]

Ratio = 1.0 (1000‰) → 변화 없음
Ratio < 1.0 (1000‰) → 반사광 감소 (발색 발생)
```

### STM8L 정수 연산 구현 (× 1000 퍼밀 스케일)

#### Step 1 : 채널별 비율 계산

```c
/* ADC 최대: 4095 × 14 = 57,330
   57,330 × 1000 = 57,330,000 → uint32_t 범위 내 안전 */

uint16_t Ratio_Cblank = (uint16_t)((uint32_t)ADC_post[0] * 1000 / ADC_pre[0]);
uint16_t Ratio_Cband  = (uint16_t)((uint32_t)ADC_post[1] * 1000 / ADC_pre[1]);
uint16_t Ratio_Tblank = (uint16_t)((uint32_t)ADC_post[2] * 1000 / ADC_pre[2]);
uint16_t Ratio_Tband  = (uint16_t)((uint32_t)ADC_post[3] * 1000 / ADC_pre[3]);

/* 예시:
   ADC_pre=20000, ADC_post=19600 → 19600×1000/20000 =  980‰ (C Blank)
   ADC_pre=20000, ADC_post=16000 → 16000×1000/20000 =  800‰ (C Band)
   ADC_pre=20000, ADC_post=19400 → 19400×1000/20000 =  970‰ (T Blank)
   ADC_pre=20000, ADC_post=14000 → 14000×1000/20000 =  700‰ (T Band) */
```

#### Step 2 : Blank 보정 비율 계산

Step 1에서 구한 Ratio 값을 직접 사용 (Blank / Band 역수 방향)

```c
/* T_corrected = Ratio_Tblank / Ratio_Tband × 1000
   → Band 발색 없음(음성): Ratio_Tblank ≈ Ratio_Tband → 결과 ≈ 1000‰
   → Band 발색 있음(양성): Ratio_Tband  < Ratio_Tblank → 결과 > 1000‰
   → 값이 높을수록 강한 양성 (직관적)

   중간값 최대: 1000 × 1000 = 1,000,000 → uint32_t 안전 ✅

   [클램프 필요 이유]
   강한 양성 시 Ratio_Tband가 매우 작아지면 결과값이 커짐
   예) Ratio_Tband = 50‰ → 970×1000/50 = 19,400‰ → uint16_t 범위 내이나
   Ratio_Tband → 0 에 가까워지면 결과값 폭발 위험 → 상한 클램프 적용  */

uint32_t dwTemp_T    = (uint32_t)Ratio_Tblank * 1000 / Ratio_Tband;
uint16_t T_corrected = (dwTemp_T > 9999) ? 9999 : (uint16_t)dwTemp_T;

uint32_t dwTemp_C    = (uint32_t)Ratio_Cblank * 1000 / Ratio_Cband;
uint16_t C_corrected = (dwTemp_C > 9999) ? 9999 : (uint16_t)dwTemp_C;

/* 예시:
   Ratio_Tblank = 970‰, Ratio_Tband = 700‰
   T_corrected  = 970 × 1000 / 700 = 1,385‰ → 1200‰ 초과 → 양성

   Ratio_Tblank = 970‰, Ratio_Tband = 950‰  (음성)
   T_corrected  = 970 × 1000 / 950 = 1,021‰ → 1200‰ 미만 → 음성

   Ratio_Cblank = 980‰, Ratio_Cband = 800‰
   C_corrected  = 980 × 1000 / 800 = 1,225‰ → 950‰ 초과 → C Band 발색 정상 */
```

### 결과 해석

```
T_corrected (‰)     의미
────────────────────────────────────────
    1000            T Band 변화 없음 (음성)
    1100            T Band 10% 추가 감소 (약양성)
    1200            T Band 약 17% 추가 감소 (양성 임계)
    1400            T Band 약 29% 추가 감소 (양성)
    2000            T Band 약 50% 추가 감소 (강양성)
    9999            클램프 상한 (극강 양성)

C_corrected (‰)     의미
────────────────────────────────────────
    1000            C Band 미발색 → 검사 무효
    1200 이상       C Band 발색 정상 → 검사 유효
```

### 판정 로직

```c
/* C Band 유효성 확인 */
if (C_corrected < C_VALID_THRESHOLD)        /* 예: 1200‰ */
{
    return RESULT_INVALID;                  /* C Band 미발색 → 검사 무효 */
}

/* T Band 판정 */
if (T_corrected >= T_POSITIVE_THRESHOLD)    /* 예: 1200‰ */
{
    return RESULT_POSITIVE;
}
else
{
    return RESULT_NEGATIVE;
}
```

### 특성

| 장점 | 단점 |
|------|------|
| PWM 튜닝 정밀도 덜 의존 | 나눗셈 연산 필요 (uint32_t) |
| LED 광량 드리프트 자동 상쇄 | Delta보다 연산량 많음 |
| 채널 간 스케일 자동 정규화 | ADC_pre=0 예외 처리 필요 |
| 농도별 선형성 개선 | |

---

## 10. Delta vs Ratio 방식 비교

```
                    Delta 방식          Ratio 방식
                    ──────────          ──────────
PWM 튜닝 의존도     높음                낮음
LED 드리프트        영향 받음           상쇄됨
연산 복잡도         덧셈/뺄셈           나눗셈 (uint32_t)
STM8L 적합성        매우 유리           사용 가능
ADC값 스케일 차이   영향 있음           자동 정규화
임계값 단위         ADC 차이값 (절대)   퍼밀 ‰ (상대율)
```

### 선택 기준

| 조건 | 권장 방식 |
|------|---------|
| PWM 튜닝이 정밀하게 수렴됨 | Delta 또는 Ratio 둘 다 가능 |
| 멘브레인 lot 간 편차가 큼 | **Ratio** |
| 전지 전압 변동이 큼 | **Ratio** |
| 연산 속도가 중요 | **Delta** |
| 농도 정량화가 목표 | **Ratio** |

---

## 11. 채널별 기울기 차이 정리

| 채널 | 위치 | 전개 후 변화 | 상대적 기울기 |
|------|------|------------|------------|
| CH_0 | C Band Blank | wetting만 | 완만 (작음) |
| CH_1 | C Band | wetting + 항상 발색 | 급격 (큼, 일정) |
| CH_2 | T Band Blank | wetting만 | 완만 (작음) |
| CH_3 | T Band | wetting + 농도 의존 발색 | 농도에 비례 |

> Phase 2 PWM 튜닝은 **절대값 기준점**을 맞추는 것이며,  
> 전개 후 **변화 기울기(감도)**는 각 채널 고유의 특성이므로  
> Blank 보정이 반드시 필요

---

## 12. 전체 타이밍 요약

| Phase | 동작 | 소요 시간 |
|-------|------|----------|
| Phase 1 | 초기 PTR 측정 (8회) | 1,600ms |
| Phase 2 | PWM 자동 튜닝 | 800 ~ 4,000ms |
| Phase 3 | 0점 측정 (4회) | 800ms |
| 전개 대기 | 멘브레인 전개 | 제품 스펙에 따름 |
| Phase 4 | 전개 후 측정 (4회) | 800ms |
| **Phase 1~3 합계** | | **≈ 3,200 ~ 6,400ms** |
