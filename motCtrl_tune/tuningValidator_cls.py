# author : sungsoo
# date : 26.02.28
# script name : tuningValidator_cls.py

import serial 
import time
import csv
from enum import Enum

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
        self.msg_period = self.cfg_init['MSG_PERIOD']
        self.tran_maxTick = int(self.cfg_judge['TIMEOUT_SEC'] / self.msg_period)
        self.idle_maxTick = int(self.cfg_judge['DETECT_REF_SEC'] / self.msg_period)
        self.stdy_maxTick = int(self.cfg_judge['MAINTAIN_SEC'] / self.msg_period)

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

        self.tran_shoot_minAllowed = 0
        self.tran_shoot_maxAllowed = 0
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
        return self.faultCode.name
    
    # 다음상태를 결정하여 반환
    def stMachine_swtSt(self):
        next_state = self.prev_state

        match(self.prev_state):
            case State.INIT:
                if (self.openSerial == True) :
                    next_state = State.IDLE
                else:
                    self.faultCode = FaultCode.PORT_OPEN_FAIL
                    next_state = State.DONE
                pass
            
            case State.IDLE:
                if (len(self.buff_records) > 0) and (self.idle_tgtRPM_Same == False):
                    self.faultCode = FaultCode.NOT_CURR_RPM
                    next_state = State.DONE
                elif self.idle_waitCnt >= self.idle_maxTick:
                    self.faultCode = FaultCode.DET_TIMEOUT
                    next_state = State.DONE
                elif (self.idle_tgtRPM_Same == True) and (self.idle_prev_tgtRPM != self.idle_curr_tgtRPM):
                    next_state = State.TRAN
                pass
            
            case State.TRAN:
                if self.tran_shootFlg == True:
                    self.faultCode = FaultCode.TRAN_SHOOT
                    next_state = State.DONE
                elif (self.tran_shootFlg == False) and (self.stdy_minAllowed <= self.tran_currRPM <= self.stdy_maxAllowed):
                    next_state = State.STDY
                elif self.tran_waitCnt >= self.tran_maxTick:
                    self.faultCode = FaultCode.TRAN_TIMEOUT
                    next_state = State.DONE
                pass

            case State.STDY:
                if self.stdy_dropFlg == True:
                    self.faultCode = FaultCode.STDY_DROPOUT
                    next_state = State.DONE
                elif self.stdy_maintainCnt >= self.stdy_maxTick:
                    self.faultCode = FaultCode.NONE
                    self.result = True  # 성공
                    next_state = State.DONE
                pass

            case State.DONE:
                # do nothing
                pass

        return next_state

    # 상태를 빠져나갈때 1회 실행
    def stMachine_exitAction(self):
        match(self.prev_state):
            case State.INIT:
                # do nothing
                pass
            case State.IDLE:
                self.Target_RPM = self.idle_curr_tgtRPM
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
        match(self.curr_state):
            case State.INIT:
                # do nothing
                pass
            case State.IDLE:
                self.idle_waitCnt = 0
                self.idle_curr_tgtRPM = 0
                self.idle_prev_tgtRPM = 0
                self.idle_tgtRPM_Same = False
                pass
            case State.TRAN:
                # 10% 슈트 마진 계산
                shoot_margin = self.cfg_judge['SHOOT_MARGIN']
                self.tran_shoot_minAllowed = self.Target_RPM * (1.0 - shoot_margin)
                self.tran_shoot_maxAllowed = self.Target_RPM * (1.0 + shoot_margin)
                
                # 5% 밴드 마진 계산
                band_margin = self.cfg_judge['BAND_MARGIN']
                self.tran_minAllowed = self.Target_RPM * (1.0 - band_margin)
                self.tran_maxAllowed = self.Target_RPM * (1.0 + band_margin)
                
                self.tran_currRPM = 0
                self.tran_shootFlg = False
                self.tran_waitCnt = 0
                pass
            case State.STDY:
                self.stdy_minAllowed = self.tran_minAllowed
                self.stdy_maxAllowed = self.tran_maxAllowed
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
        match(self.curr_state):
            case State.INIT:
                # 시리얼포트 열기 (함수는 나중에 만들예정)
                pass
            case State.IDLE:
                self.idle_curr_tgtRPM = self.mcu_buffer.get('refRPM', 0.0)
                init_rpm = self.cfg_judge['INIT_REF_RPM']
                
                if len(self.buff_records) == 0:
                    if self.idle_curr_tgtRPM == init_rpm:
                        self.idle_tgtRPM_Same = True
                    else : 
                        self.idle_tgtRPM_Same = False
                        
                if self.idle_prev_tgtRPM == self.idle_curr_tgtRPM:
                    self.idle_waitCnt += 1
                
                self.idle_prev_tgtRPM = self.idle_curr_tgtRPM
                pass
            case State.TRAN:
                self.tran_currRPM = self.mcu_buffer.get('RPM', 0.0)
                
                if not (self.tran_shoot_minAllowed <= self.tran_currRPM <= self.tran_shoot_maxAllowed):
                    self.tran_shootFlg = True
                else :
                    self.tran_shootFlg = False

                self.tran_waitCnt += 1
                pass
            case State.STDY:
                self.stdy_currRPM = self.mcu_buffer.get('RPM', 0.0)
                
                if not (self.stdy_minAllowed <= self.stdy_currRPM <= self.stdy_maxAllowed):
                    self.stdy_dropFlg = True
                else :
                    self.stdy_dropFlg = False
                
                self.stdy_maintainCnt += 1
                pass
            case State.DONE:
                
                # csv 저장 함수 만들기
                
                # 시리얼 닫기 함수 만들기
                
                self.exitFlg = True
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
        self.buff_records.append(self.mcu_buffer.copy())
        self.prev_state = self.curr_state