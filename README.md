# 🚀 소개 (Introduce)
이 프로젝트는 STM32F767 MCU를 기반으로 작성된 **BLDC 모터 인버터 제어 커스텀 펌웨어** 입니다. 

단순히 HAL 라이브러리에 의존하는 것을 넘어, 하드웨어 특성을 고려한 **비선점형 커스텀 스케줄러**와 **시스템 아키텍처**를 직접 설계하여 적용했습니다.

구현 과정과 SW에 대한 보다 자세한 사항은 [링크](https://velog.io/@smersh/%EB%AA%A8%ED%84%B0-%EC%A0%9C%EC%96%B4-%ED%94%84%EB%A1%9C%EC%A0%9D%ED%8A%B8)를 참고해주세요. 

## 목차

* 주요특징
* 개발환경
* 사용방법
* 시스템 아키텍처
* 향후 개선사항
* 이슈트래킹

# 💡 주요 특징 (Key Features)

### **홀센서 기반 상태 추정 및 폐루프(Closed-loop) PI 제어**

- 20kHz 제어 루프 내에서 홀센서상태를 실시간으로 읽어 정확한 로터 위치 기반의 6-Step Commutation(상전환)을 수행합니다.
- 단순 개루프(Open-loop) 형태의 스로틀 제어뿐만 아니라, 목표 RPM을 정밀하게 추종하는 **PI 제어 알고리즘**을 구현하여 모터의 동적 특성을 제어합니다.

### 하드웨어 보호를 위한 20kHz ISR Fail-Safe 제어

- 정지 명령이나 에러 발생 시, 단순히 PWM Duty를 조절하는 대신 타이머의 **MOE(Main Output Enable) 비트를 즉각 차단**합니다.
- 6개의 MOSFET을 즉시 High-Z(플로팅) 상태로 전환하여, 역기전력(Back-EMF) 단락을 방어하는 Coasting Stop 로직을 적용했습니다.

### 객체 지향 설계 및 계층적 설계

- 하드웨어를 직접적으로 제어하는 계층과 그 모듈을 이용해 요구조건을 충족시키는 계층을 나누고 나서 구현함으로써, 유지보수성을 높였습니다.
- 각 모듈을 객체화 시켜서, 응집성과 디버깅 효율을 높였습니다.

### 실무형 파이썬 튜닝 툴 및 다중 빌드 지원

- 매크로 한 줄로 App 모드와 Debug 모드를 컴파일 타임에 스위칭합니다.
- Teleplot 및 자체 제작 파이썬 스크립트(`tuningValidator.py`)를 연동하여 실시간 모터 응답성 모니터링 및 PI 게인 튜닝 환경을 구축했습니다.

# 👾 개발 환경 (Dev Env)

| **항목** | **설명** | **비고** |
| :--- | :--- | :--- |
| MCU | STM32F767VIT6 | [메뉴얼](https://www.st.com/resource/en/datasheet/stm32f765zi.pdf) |
| Motor | BL42S-24026N | [구매링크](https://www.devicemart.co.kr/goods/view?no=1326302) |
| UART | FTDI 쪽보드를 사용 | [구매링크](https://www.devicemart.co.kr/goods/view?no=1290042) |
| IDE | STM32CubeIDE | ver 1.19.0 |
| debug | ST-Link V2 | |
| 외부 프로그램 | teleplot | VScode 확장프로그램 |

# 💻 사용 방법 (How to Use)

## 제어방법

### Throttle mode

#### 전제조건
- mtrCtrl 모듈에서 `vMotorCtrl_manager` 인스턴스의 `selCtrl_mode` 멤버변수가 `MTRCTRL_CTRL_THROTTLE` 여야 합니다.
- mtrCtrl 모듈에서 `vMotorCtrl_manager` 인스턴스의 `errCode` 멤버변수가 `MTRCTRL_ERR_NONE (5)` 여야 합니다.
- sysInput 모듈에서 `vSysInput` 인스턴스의 `userPause` 멤버변수가 false 여야 합니다.
      
#### 제어방법
- 스로틀을 손으로 당기십시오
    

### PI control mode

#### 전제조건
- mtrCtrl 모듈에서 `vMotorCtrl_manager` 인스턴스의 `selCtrl_mode` 멤버변수가 `MTRCTRL_CTRL_PI` 여야 합니다.
- mtrCtrl 모듈에서 `vMotorCtrl_manager` 인스턴스의 `errCode` 멤버변수가 `MTRCTRL_ERR_NONE (5)` 여야 합니다.
- sysInput 모듈에서 `vSysInput` 인스턴스의 `userPause` 멤버변수가 false 여야 합니다.
- mtrCtrl 모듈에서 `vPiCtrl_handler` 인스턴스의 `Kp,Ki` 멤버변수가 0이 아니어야 합니다.

#### 제어방법
- sysInput 모듈에서 `vSysInput` 인스턴스의 `userRefRPM` 멤버변수를 조작하십시오.
- 스로틀을 손으로 당기십시오

## 모드 선택
utils.h 파일에서 `MTR_INVTR_CTRL_MODE` 를 사용해서 펌웨어를 2종류로 빌드할 수 있습니다.

```cpp
#define MTR_INVTR_CTRL_DEBUG    (0UL)
#define MTR_INVTR_CTRL_APP      (1UL)
#define MTR_INVTR_CTRL_MODE     (MTR_INVTR_CTRL_APP)
```

### App 모드

Host가 sysInput 계층에서 정의된 명령어와 파라미터를 통해 모터를 제어하기 위한 용도입니다.

통신을 위해 `teraterm` 혹은 터미널로써 펌웨어와 소통할 수 있는 프로그램이 반드시 필요로 합니다.

#### UART 통신 설정
하단의 스펙대로 설정해야 정상적으로 호스트와 타겟이 통신할 수 있습니다.
| **설정** | **값** |
| :--- | :--- |
| speed | 115200 bps |
| data | 8 bit |
| parity | none |
| stopbit | 1 bit |
| flowcontrol | none |

#### CMD 종류

하단의 표에 나타난 명령어 + 파라미터 조합을 통해서만 타겟이 호스트의 의도대로 동작합니다.
<img width="645" height="314" alt="uart_input2" src="https://github.com/user-attachments/assets/db4d5af3-1788-4a27-ae5e-80cd3bdab794" />

| **명령어** | **파라미터** | **설명** | **비고** |
| :--- | :--- | :--- | :--- |
| `m>` | 문자열 | 사용자는 이 명령을 통해 제어 모드를 선택할 수 있습니다. | "th" 는 스로틀 제어모드<br>"pi" 는 PI 제어모드 |
| `p>` | 실수 | Kp를 변경합니다 | 0~5.0 으로 클리핑됨 |
| `i>` | 실수 | Ki를 변경합니다 | 0~10.0 으로 클리핑됨 |
| `r>` | 정수 | PI 제어모드일 때의 지령값을 설정합니다 | 0~4000으로 클리핑 됨<br>(모터의 정격출력 = 4000RPM) |
| `t>` | - | 일시적으로 모터제어를 중단합니다 | toggle 방식으로 동작 |
| `q>` | - | 현재 동작하는 모터 인버터 시스템을 종료시킵니다 | - |

### Debug 모드

전체적인 시스템을 검증하고 개선하기 위한 용도입니다.

`teraterm` 으로 상태를 주기적으로 받아보거나, 튜닝을 위해 VScode의 확장프로그램인 `teleplot` 에서 그래프를 실시간으로 확인하며 제어 방법을 실시간으로 수정해가며 개선할 수 있습니다.

이 상태에서는 입력을 ST-Link 디버거만을 통해서 줄 수 있습니다.

또한, 구성파일의 `Motor Tuner`를 사용하여 목표로 하는 PI제어기 요구사항을 맞출 수 있습니다.

#### Teleplot 사용법

VScode의 익스텐션인 teleplot을 이용하여 제어와 관련된 요소들을 그래프로써 아래의 이미지처럼 추적할 수 있습니다

<img width="1333" height="603" alt="image527" src="https://github.com/user-attachments/assets/d7d270db-f332-4e30-b67c-f8d8de374ed0" />

- VScode Extension 창에 `teleplot` 입력 후 설치
- 창의 좌측하단의 `teleplot` 버튼 클릭
- TeraTerm 통신 설정(115200bps 등)과 동일하게 세팅

#### MotorTuner 사용법

motCtrl_tune 폴더내의 파이썬 스크립트로 튜닝목표를 정확하게 달성할 수 있습니다.

<img width="215" height="191" alt="image521" src="https://github.com/user-attachments/assets/22c4f8d8-64ed-4014-8fec-fe83d06c5e9b" />

- Target 인버터를 동작시킨 상태 유지
- `motCtrl_tune` /`tuningValidator.py` 파일을 수정하기
- `tuningValidator.py` 파일 수정 (`CONFIG_INIT`,`CONFIG_JUDGE` 를 커스터마이징)
- `tuningValidator.py` 파일을 실행
- 스로틀을 당겨준 상태에서, 디버거로 지령값을 입력
- 테스트가 끝나면 같은 경로 안의 CLI창과 `tuning_result.csv` 를 분석
  
# ⚙️ 시스템 아키텍처 (System Architecture)

펌웨어는 총 3개의 계층(MCAL단 - 태스크 스케줄러단 - App단)으로 이루어져 있습니다.

이를 통해 디버깅과 SW 유지보수성, 그리고 모듈 이식성을 극대화했습니다.

<img width="259" height="305" alt="image110" src="https://github.com/user-attachments/assets/8847afb3-738a-4d89-8591-b1a4f046c33c" />

또한, 인버터 펌웨어를 하나의 시스템으로 축약한다면 아래의 그림처럼 나타낼 수 있습니다.

시스템 동작에 필수적인 Task와 ISR을 모아 시스템의 3요소인 input, process, output 으로 나눈다면 아래와 같이 표현 가능합니다.
<img width="773" height="437" alt="스크린샷 2026-03-16 235520" src="https://github.com/user-attachments/assets/ef7589fb-5e7a-4b95-a2c2-efba4b50ac96" />

# **🛠️ 향후 개선 사항 (Future Works)**

## **CAN 통신 도입**

자동차 산업에서 ECU 간 통신의 표준인 CAN은 노이즈가 극심한 인버터 환경에서 필수적인 프로토콜입니다.

현재 타겟 보드에 CAN 트랜시버 IC가 실장되어 있지 않음을 확인하여, 추후 트랜시버 확장 보드를 추가해 CAN 프로토콜 기반의 신뢰성 높은 제어 지령 수신 기능을 구현할 예정입니다.

## CallBack, DMA 전환

이번 프로젝트는 순수 CMSIS 레지스터를 직접 제어하며 전체 아키텍처를 세우는 데 집중하다 보니, 구조의 복잡도를 낮추기 위해 UART 송수신이나 일부 센싱 라인을 폴링 방식으로 타협했습니다.

하지만 실무에서 귀중한 CPU 자원의 낭비를 막으려면 논블로킹 방식이 필수적입니다.

향후 DMA와 Tx/Rx 인터럽트 콜백 구조로 전면 리팩토링하여 시스템 퍼포먼스를 극대화해보고 싶습니다.

## **I2C & SPI 통신 지원**

마이크로프로세서 통신장(UART, I2C, SPI) 중 나머지 두 개에 대한 구현입니다.

타겟 보드에 EEPROM이 탑재되어 있으나 이번 페이즈에서는 튜닝 값이 고정되어 있어 굳이 사용하지 않았습니다.

추후 I2C 드라이버를 직접 작성하여, 파이썬 툴로 찾은 최적의 PI 게인(Kp, Ki) 값 등을 EEPROM에 비휘발성 데이터로 저장하고 불러오는 기능을 추가할 계획입니다.

## **블루투스(BLE) 통신 (무선 제어)**

타겟 보드에는 AT09(또는 HC-04) 기반의 블루투스 통신 모듈을 부착할 수 있도록 설계가 되어 있습니다.

아직 하드웨어적인 결선 및 검토 작업이 남아있어 이번 구현에서는 제외했지만, 조만간 스마트폰이나 무선 PC 환경에서 인버터를 원격으로 제어하고 디버깅할 수 있도록 무선 통신 기능을 붙여볼 생각입니다.

# 📝 이슈 트래킹 (Issue Note)

`main.c` 파일의 최하단에 펌웨어 개발 중 발생했던 모든 이상 현상, 원인 분석, 그리고 해결 과정을 기록한 **'이슈 노트(Issue Note)'**와 **'수정 이력(Revision History)'** 를 확인할 수 있습니다.
