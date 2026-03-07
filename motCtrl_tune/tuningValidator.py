# author : sungsoo
# date : 26.02.28
# script name : tuningValidator.py

from tuningValidator_cls import tuneJudge

# 개요
# 모터 제어기의 PI 튜닝 결과를 PC에서 실시간으로 모니터링하고 정량적으로 자동 검증하는 툴
# UART로 수신된 데이터를 파싱하여, 시스템의 과도응답 및 정상상태 가 목표 스펙을 만족하는지 판별
# 그리고 전체 데이터를 CSV로 기록함

# 요구사항
# 1. 정상상태 : 지령 인가 후, 목표 RPM의 +-5% 대역에 진입 (100ms 이내)
# 2. 오버슛 : 전체 구동 시간 동안 목표 RPM의 ±10%를 초과하는 오버슛이 발생 X 일 것. (30sec)
# 3. 유지 : 정상상태 진입후, +-5% 대역에 머무를 것
# 4. 외란 : 고려 X

# 특이사항
# ** 위상분산 데이터 수신 **
# MCU가 한 번에 데이터를 보내지 않고 10ms 주기 내, 2ms(지령), 4ms(RPM), 6ms(Imax), 8ms(CCR) 순서로 쪼개서 송신
# 지령(2ms) 데이터를 시작으로 한 주기 버퍼에 모아두었다가 모든 데이터를 수신하면 (8ms CCR) 평가

# ** 동작 간단 오버뷰 **
# 이 툴은 FSM 기반으로 동작하는 툴이다.
# 
# 상태 1 : INIT
# 시리얼 포트를 열지 않은 상태
#
# 상태 2 : IDLE
# 입력받은 지령값으로 변하는 순간을 대기하는 상태
#
# 상태 3 : TRAN
# 입력받은 지령값으로 변하는 순간을 감지하여 과도 응답에 들어온 상태
# RPM이 오버슛 제한(±10%)을 넘는지 매 사이클 감시 ? 넘으면 fail 처리 : 상태유지
# 들어왔다면 현재 경과 시간이 제한시간(100ms) 이내인지 확인 ? 이내면 상태전이 : 초과면 fail 처리
#
# 상태 4 : STDY
# RPM이 제한시간내(30초) 지령값 부근에 도달하여 정상상태에 들어온 상태
# ±5% 대역을 계속 유지하는지 감시 ? 넘으면 fail 처리 : 상태유지
# 
# 상태 5 : DONE
# 기존까지 받았던 데이터들을 콘솔창에 출력 및 csv 파일을 열어 작성
# 화면에 결과를 출력 후 csv 및 시리얼 포트 닫기

# 초기설정 및 요구스펙
CONFIG_INIT = {
    'COM_PORT'      : 'COM12',
    'BAUDRATE'      : 115200,
    'MSG_PERIOD'    : 0.01,    # uart 메시지 주기 10ms
    'CSV_NAME'      : 'tuning_result.csv',
    'COM_PREFIX'    : '>',
    'COM_SUFFIX'    : '\n'
}

# 판정설정
CONFIG_JUDGE = {
    'TIMEOUT_SEC'   : 0.8,      # 정상상태 도달 제한 시간 (800ms)
    'MAINTAIN_SEC'  : 30.0,     # 정상상태 유지 시간 (30초)
    'DETECT_REF_SEC': 300.0,    # 목표지령값 대기 시간 (5분)
    'INIT_REF_RPM'  : 0,        # 초기 지령값(시작) (RPM)
    'BAND_MARGIN'   : 0.05,     # ±5%
    'SHOOT_MARGIN'  : 0.1,      # ±10%
}

if __name__ == '__main__':

    print('START motor PI tuner !!\n')

    judge = tuneJudge(CONFIG_INIT, CONFIG_JUDGE)
    
    judge.stMachine_init()
    
    print('START motor PI tuner !!\n')

    while(judge.stExit() == False):
        judge.stMachine()
    
    print('END motor PI tuner !!')
    print(f'RESULT : {judge.get_result()}')
    print(f'FAULT CODE : {judge.get_faultCode()}')
