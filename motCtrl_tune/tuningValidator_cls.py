# author : sungsoo
# date : 26.02.28
# script name : tuningValidator_cls.py

import serial 
import time
import csv
from enum import Enum

# 요구사항
# 1. 정상상태 : 지령 인가 후, 목표 RPM의 +-5% 대역에 진입 (100ms 이내)
# 2. 오버슛 : 전체 구동 시간 동안 목표 RPM의 ±10%를 초과하는 오버슛이 발생 X 일 것. (30sec)
# 3. 유지 : 정상상태 진입후, +-5% 대역에 머무를 것
# 4. 외란 : 고려 X

# 특이사항
# ** 위상분산 데이터 수신 **
# MCU가 한 번에 데이터를 보내지 않고 10ms 주기 내, 2ms(지령), 4ms(RPM), 6ms(Imax), 8ms(CCR) 순서로 쪼개서 송신
# 지령(2ms) 데이터를 시작으로 한 주기 버퍼에 모아두었다가 모든 데이터를 수신하면 (8ms CCR) 평가
#
# ** MCU 데이터 포멧 **
# 모든 메시지는 ">데이터종류: 값\n" 형태로 날아온다
#
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

# 상태정의
class State(Enum):
    INIT = 0
    IDLE = 1
    TRAN = 2
    STDY = 3
    DONE = 4

class FaultCode(Enum):
    NONE = 0                # 어떠한 에러 발생되지 않음
    PORT_OPEN_FAIL = 1      # 포트 오픈에 실패
    NOT_CURR_RPM = 2        # 초기 지령값이 하드코딩된 RPM이 아님
    DET_TIMEOUT = 3         # 목표 지령값 수신까지의 대기시간 초과
    TRAN_SHOOT = 4          # 과도응답 도중 오버/언더슛 발생
    TRAN_TIMEOUT = 5        # 과도응답 ~ 정상상태 시간초과
    STDY_DROPOUT = 6        # 정상상태에서 대역유지 실패
    USER_ABORT = 7          # 사용자 종료

class tuneJudge:
    def __init__(self, cfg_init, cfg_judge):
        # 1. 설정값 저장
        self.cfg_init = cfg_init
        self.cfg_judge = cfg_judge

        # 2. 시스템 상태 및 통신 객체 초기화
        self.curr_state = State.INIT
        self.prev_state = State.INIT
        self.serial = None

        # 3. 데이터 버퍼
        self.mcu_buffer = {'refRPM' : 0.0, 'RPM': 0.0, 'refCCR': 0}
        self.buff_records = []

        # 4. 시간변수
        self.start_time = 0.0
        self.end_time = 0.0
        self.stdy_start_time = 0.0
        self.trans_maxTick = (int)(self.cfg_init{}) # TIMEOUT_SEC / MSG_PERIOD 어떻게 설정?

        # 4. 결과
        self.faultCode = FaultCode.NONE
        self.exitFlg = False
        self.result = False
        self.openSerial = False

        # 상태 두액션에서 사용하는 변수
        self.idle_waitCnt = 0
        self.Curr_RPM = 0
        self.Target_RPM = 0
        self.idle_curr_tgtRPM = 0
        self.idle_prev_tgtRPM = 0
        self.idle_tgtRPM_Same = False

        self.tran_minAllowed = 0
        self.tran_maxAllowed = 0
        self.tran_currRPM = 0
        self.tran_shootFlg = False
        self.tran_waitCnt = 0

        self.stdy_minAllowed = 0
        self.stdy_maxAllowed = 0
        self.stdy_currRPM = 0
        self.stdy_dropFlg = False
        self.stdy_maintainCnt = 0

        self.done_csvOpened_flg = False
        self.done_csvWrote_flg = False
        self.done_closedSerial = False
    
    def stExit(self):
        return self.exitFlg
    
    def get_result(self):
        return self.result
    
    def get_faultCode(self):
        return self.faultCode
    
    # 다음상태를 결정하여 반환
    def stMachine_swtSt(self):

        next_state = self.prev_state

        match(self.prev_state):
            case State.INIT:
                # 멤버의 시리얼객체가 null이 아님을 확인
                # 혹은 시리얼객체가 반환해주는 함수를 보고 판단
                    # next_state = Idle
                # else 
                    # next_state = Done
                    # faultCode = PORT_OPEN_FAIL
                pass
            case State.IDLE:
                # 레코드 멤버 크기가 0이 아닐때, idle_tgtRPM_Same == False
                    # next_state = Done
                    # faultCode = NOT_CURR_RPM
                # 목표지령값이 사용자가 지정한 지령값(INIT_REF_RPM)에서 타임아웃까지 변하질 않음 (idle_waitCnt 초과)
                    # next_state = Done
                    # faultCode = DET_TIMEOUT
                # curr_tgtRPM가 하드코딩된 지령값(INIT_REF_RPM)과 일치(idle_tgtRPM_Same == true)하고 idle_prev_tgtRPM != idle_curr_tgtRPM 임
                    # next_state = Tran
                pass
            case State.TRAN:
                # 측정 rpm이 목표지령값에서 허용 오버/언더슛 범위를 넘어섬
                    # next-state = Done
                    # faultCode = TRAN_SHOOT
                # 측정 rpm이 목표지령값의 일정시간동안 정상상태 허용범위에 들어오지 못함
                    # next-state = Done
                    # faultCode = TRAN_TIMEOUT
                # 측정 rpm이 허용 오버/언더슛 범위를 넘어서지 않았고 일정시간동안 정상상태 허용범위에 들어옴
                    # next-state = STDY
                pass

            case State.STDY:
                # 측정 rpm이 정상상태 허용범위를 벗어남
                    # next-state = Done
                    # faultCode = STDY_DROPOUT
                # 측정 rpm이 MAINTAIN_SEC 동안 정상상태 허용범위를 지킴
                    # next-state = Done
                pass

            case State.DONE:
                # do nothing
                pass

        return next_state

    # 상태를 빠져나갈때 1회 실행
    def stMachine_exitAction(self):
        # note : 아직 뭐가 와야하는지 감이 안옴
        match(self.prev_state):
            case State.INIT:
                # do nothing
                pass
            case State.IDLE:
                # self.curr_TgtRPM_1stFlg = True
                # self.Target_RPM = curr_tgtRPM
                pass
            case State.TRAN:
                # do nothing
                pass
            case State.STDY:
                # do nothing
                pass
            case State.DONE:
                # do nothing
                pass
        return

    # 상태에 진입할때 1회 실행
    def stMachine_entryAction(self):
        match(self.prev_state):
            case State.INIT:
                # do nothing
                pass
            case State.IDLE:
                self.idle_waitCnt = 0
                # self.Curr_RPM = INIT_REF_RPM
                # self.Target_RPM = INIT_REF_RPM
                self.idle_curr_tgtRPM = 0
                self.idle_prev_tgtRPM = 0
                self.idle_tgtRPM_Same = False
                pass
            case State.TRAN:
                # self.tran_minAllowed = self.Target_RPM - (self.Target_RPM * (1-OVER_MARGIN))
                # self.tran_maxAllowed = self.Target_RPM + (self.Target_RPM * (1-OVER_MARGIN))
                self.tran_currRPM = 0
                self.tran_shootFlg = False
                self.tran_waitCnt = 0
                pass
            case State.STDY:
                # self.stdy_minAllowed = self.Target_RPM - (self.Target_RPM * (1-BAND_MARGIN))
                # self.stdy_maxAllowed = self.Target_RPM + (self.Target_RPM * (1-BAND_MARGIN))
                self.stdy_currRPM = 0
                self.stdy_dropFlg = False
                self.stdy_maintainCnt = 0
                pass
            case State.DONE:
                self.done_csvOpened_flg = False
                self.done_csvWrote_flg = False
                self.done_closedSerial = False
                pass
        return

    # 상태에 머무는 동안 반복실행
    def stMachine_doAction(self):
        match(self.prev_state):
            case State.INIT:
                # 시리얼포트 열기 (함수는 나중에 만들예정)
                pass
            case State.IDLE:
                # 레코드가 0일 때 idle_curr_tgtRPM != INIT_REF_RPM ? idle_tgtRPM_Same = false ? idle_tgtRPM_Same = true
                # idle_prev_tgtRPM != idle_curr_tgtRPM ? return : idle_waitCnt++
                pass
            case State.TRAN:
                # tran_currRPM이 tran_minAllowed, tran_maxAllowed 범위 밖인지 확인 ? tran_shootFlg= t : tran_shootFlg = f
                # tran_waitCnt++
                pass
            case State.STDY:
                # stdy_currRPM이 stdy_minAllowed, stdy_maxAllowed 범위 밖인지 확인 ? stdy_dropFlg = t : stdy_dropFlg = f
                # stdy_maintainCnt++
                pass
            case State.DONE:
                pass
        return
    
    # --------------------------------------------------
    # 메인 상태 머신 엔진 (외부에서 폴링)
    # --------------------------------------------------
    def stMachine(self):
        
        # 1. 상태 전이 판단 (Input)

        # 시리얼 객체의 폴링 리시브 사용 -> init상태가 아닐때만 (pc는 mcu처럼 시간을 잘 지킬 수 없기에 pc에서의 10ms가 실제 10ms가 아닐수도...)
        self.curr_state = self.stMachine_swtSt()

        # 2. 상태 액션 처리 (Process)
        if self.curr_state == self.prev_state:
            self.stMachine_doAction()
        else:
            self.stMachine_exitAction()
            self.stMachine_entryAction()
            self.stMachine_doAction()

        # 3. 현재 상태를 이전 상태로 업데이트 및 기록 업데이트 (Output) 
        self.buff_records.extend(self.mcu_buffer)
        self.prev_state = self.curr_state