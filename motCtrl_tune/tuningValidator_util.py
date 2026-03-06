# author : sungsoo
# date : 26.03.04
# script name : tuningValidator_util.py

import serial 
import time
import csv

class serialHandler:
    # UART 통신 전담
    def __init__(self, port, baudrate, prefix, suffix, timeout=0.01):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.prefix = prefix
        self.suffix = suffix
        self.ser = None
    
    def openPort(self):
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=self.timeout)
            return True
        except Exception as e:
            return False
    
    def closePort(self):
        if self.ser and self.ser.is_open:
            self.ser.close()
        return
    
    def readLine(self):
        # 한 줄을 읽어서 Prefix/Suffix를 검증하고 Key, Value를 튜플로 리턴
        # 성공 시: ('refRPM', 4000.0), 실패 시: (None, None)
        if not self.ser or self.ser.in_waiting == 0:
            return None, None
        try: 
            raw_line = self.ser.readline().decode('utf-8', errors='ignore').strip()
            
            if not raw_line.startswith(self.prefix):
                return None, None
            else : 
                content = raw_line[len(self.prefix):] # ">" 제거
                if ":" not in content:
                    return None, None
                
                key, value_str = content.split(":", 1) # 첫 번째 ":" 기준으로 분리
                key = key.strip()
                value = float(value_str.strip())
                
                return key,value # Key, Value를 튜플로 리턴
            
        except (ValueError, IndexError, Exception):
            # 파싱 실패나 float 변환 실패 시 무시
            return None, None
        
class csvLogger:
    # 데이터 저장 전담
    def __init__(self, filename):
        self.filename = filename
    
    def saveData(self, dataLog, startTime, endTime):
        if not dataLog:
            return False
        
        try:
            # 1. 헤더로 사용할 Key 추출
            header = dataLog[0].keys() # 첫 번째 딕셔너리의 'Key'들만 뽑아서 표의 제목으로 사용
            
            # 2. 기록할 시간 정보 문자열 생성
            s_time = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(startTime))
            e_time = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(endTime))
            duration = round(endTime - startTime, 2)
            
            time_info = f"Test Time: {s_time} ~ {e_time} (Duration: {duration}s)\n"
            
            with open(self.filename, 'w', newline='', encoding='utf-8') as fp:
                fp.write(time_info)
                writer = csv.DictWriter(fp, fieldnames=header)
                writer.writeheader()
                writer.writerows(dataLog)
            return True
        except Exception as e:
            return False