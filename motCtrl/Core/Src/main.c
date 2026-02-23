#include "stm32f767xx.h"
#include "utils.h"
#include "system.h"
#include "gpio.h"
#include "tim.h"
#include "adc.h"
#include "dac.h"
#include "isr.h"
#include "uart.h"
#include "../Drv/tasksch/tasksch.h"
#include "../Drv/mtrCtrl/mtrCtrl.h"

int main(void)
{
    system_clock_init();
    system_sysTick_init();
    gpio_init();
    tim_init();
    exti_init();
    adc_init();
    dac_init();
    uart_AT09_init(9600, 8, 1, UART_PARITY_NONE);
    uart_debug_init(9600, 8, 1, UART_PARITY_NONE);
    tasksch_init();
    mtrCtrl_objInit(0,0);
    mtrCtrl_setPeriphInit();

    tim_pwm1_nvic_counterSet(); // TIM1의 ISR에서 모듈을 사용하기에 제일 마지막에 호출 될 것

    tasksch_execTask();
}

/*---------------------------------------
 * 이슈노트
 *
 * tasksch_config.h 의 오버런 디텍션설정이 enable로 되어있었음 (오버런 감지) -> disable로 수정
 *
 * Vdc를 제대로 읽지 못해서 언더볼테지 에러 발생 -> DCVOLT_VOLT_2_ACTUAL가 전압 분배값으로 나누는게 아닌 곱하는 매크로였음
 *
 * ADC 설정이 틀려서 스로틀이 안먹는거 같음 -> isr과 태스크 adc 측정함수간의 경쟁조건으로 값이 오염됨
 *
 * 전류객체의 전류값이 -단으로 찍힘 (원본은 +로 찍힘) -> 초기화 순서가 잘못되었음 sensing 객체 초기화 전에 adc_offsetCalib 이 끝났어야 함
 * 										-> SENSING_ADC_2_CURR 매크로가 다시 오프셋 측정값을 빼는 문제가 있었음
 *
 * 현재 스로틀로 모터를 동작시키면 우드득 소리가 나면서 덜덜 떨림 -> mcal 단의 cr1 레지스터 설정이 완전하지 않았음
 *									-> REV(-1) 상은 gnd를 출력하기 위해선 CCR = HALLSENS_DUTY_MAX 여야할 것
 *									-> STP(0) 상은 floating 상태를 갖기 위해선 pwm 출력자체를 차단해야함
 *
 * 스로틀을 당기면 손으로 살짝 축을 조작해야 동작을 시작 및 반응성 낮음 -> 반응성 저하 : 스로틀 측정 로직이 10ms에서 동작 => ramp 슬로프 단위가 큼
 *					-> 손으로 살짝 축을 조작 : hallsens_set_PWMduty 에서 tim_Pwm1_Unmute_channel, tim_Pwm1_setCmpVal 함수에 상 번호를 전달하는데,
 *					enum 타입 변환이 안되어서 W상이 아예 동작을 안했었음
 *
 * 스로틀을 동작시키지 않은 정지상태에서 타임아웃 에러가 발생함 -> 스로틀제어 PI 모두 throttle validateFlg 을 확인해야 할 것
 *
 * rpm 계산로직이 이상한 값을 출력함 -> hallsens_cal_motorRPM 에서 deltaTick 구하는 조건이 생뚱맞게 작성됨
 *
 * rpm 계산이 모터가 동작할때만 업데이트 됨 + 스로틀 제어시 CCR 값의 반응성이 너무 느림 -> 타임아웃 로직 삭제=> 타임아웃이 아니라 제로스피드 체크로 변경
 *
 * 모터를 큰 CCR 값으로 동작시키면 진동이 발생 -> isr에서 모터가 켜지면 전부 unmute 했다가 stop 상만 다시 mute 하는데 이 찰나의 순간에 전기적인 쇼트가 발생
 *                                      -> 이 순간적인 쇼트가 진동의 원인으로 모터를 불안정하게 만듬
 *
 * 모터를 최대 CCR 값으로 동작시키면 잔진동이 발생 (max 5300인데, 5280이상으로 주면 잔진동이 발생)
 * 									-> High Duty 상태에서 CCER만 Disable하면 내부 OCxREF는 계속 High로 유지되어 Dead-time 상태가 리셋되지 않고,
 * 									이후 Unmask 순간 내부 상태와 실제 MOSFET 상태가 어긋나면서 Arm-short가 발생해 고속 잔진동이 생긴 현상
 * 									-> 차단되어있다가 상변환에 의해서 ccer이 켜지며 oc와 ocn 신호가 나오게 되고 이 순간, 데드타임이 없이 두 신호 교차하는 시점까지 쇼트 (Arm-short)
 * --------------------------------------*/

// 질문 -> ramp 함수를 품은 모터 제어 로직이 태스크에서 돌아갈 때, 태스크 주기는 무조건 짧은게 좋음
// 근데 만약, 비선점형 스케줄러일 경우 -> 정확한 함수동작 시간 측정 필요
// 이럴때는 모터제어로직을 fsm으로 짜는것보단 input process output이 명확한 함수형태로 짜는게 더 적합?

/*---------------------------------------
 * 수정노트
 *
 * 500ms 태스크에 led 토글 추가
 * 태스크 adc 측정 함수에 critical section 구획
 * DCVOLT_VOLT_2_ACTUAL를 나누는 매크로로 변경
 * adc_offsetCalib을 task_init_hook 함수 상단에서 호출 + SENSING_ADC_2_CURR 매크로 수정
 * tim1(=pwm1)의 cr1 레지스터를 수정 + hallsens_set_PWMduty 함수에서 홀센서 값에 따른 igbt제어(상전류 출력) 로직 수정
 * 반응성이 저하 되기에 스로틀 변환로직을 1ms 태스크에서 호출하게 함
 * hallsens_set_PWMduty 에서 각 상의 상태에 따른 제어시 timer 모듈을 호출시, 전달하는 상번호를 타입변환해서 전달
 * mtrCtrl_check_RPM_timeout 로직의 초반부에서 throttle_get_validateFlg 가 false이면 카운트 초기화 하고 나가게 수정
 * hallsens_cal_motorRPM 에서 deltaTick 구하는 조건문을 올바르게 수정
 * 타임아웃 에러코드 및 체크 로직 삭제 -> 10ms 태스크에서 모터 정지 체크 로직 새로 정의 및 kmh 계산로직과 10ms 태스크에서 호출
 * Unmute_channel 및 Mute_channel 함수의 내용에서 전체선택 케이스를 MOE만 다루게 변경 + 초기화 과정에서 MOE 및 모든채널 뮤트하도록 변경
 * Unmute_channel 에서 stop상에서 채널을 mute하면 그에 따른 CCR값도 초기화 + mtrCtrl_setFinalCCR_refVal을 1ms 태스크에서 호출
 * --------------------------------------*/

// todo : utils의 내용을 변경 -> 인휠모터 삭제 및 디파인문 오토사에 맞게 변경
// todo : PI제어 과정 블로그/포폴로 쓸 수 있게 처리 및 확인
// todo : 오실로스코프로 전해 커패시터의 충방전 시간을 기록하고 postRun 로직에 방전시간 기록 및 다른 로직 추가
// todo : iwdg 달아보기
// todo : 100ms의 uart 전송 로직들을 짧게 위상차를 두어서 쪼개기 + bt 전송 로직들을 짧게
// todo : 태스크 동작 gpio 핀으로 표시 및 오실로스코프로 태스크 실행시간 측정
