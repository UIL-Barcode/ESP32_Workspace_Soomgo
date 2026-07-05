import os
import requests
import json
import sys
from dotenv import load_dotenv

# Windows 터미널에서 Unicode 출력이 깨지는 현상 방지
if sys.stdout.encoding.lower() != 'utf-8':
    sys.stdout.reconfigure(encoding='utf-8')

# 1. .env 파일에서 환경변수 로드
load_dotenv()

API_KEY = os.getenv("API_KEY")
API_SECRET = os.getenv("API_SECRET")

# 키움증권 REST API 호스트 (실전투자: https://api.kiwoom.com, 모의투자: https://mockapi.kiwoom.com)
# 발급받으신 App Key가 실전/모의 중 어느 용도인지에 따라 변경해야 할 수 있습니다.
KIWOOM_HOST = "https://api.kiwoom.com"
SAMSUNG_ELEC_CODE = "005930" # 삼성전자 종목코드

def get_access_token():
    """App Key와 Secret Key를 사용하여 접근 토큰을 발급받습니다."""
    url = f"{KIWOOM_HOST}/oauth2/token"
    
    headers = {"Content-Type": "application/json"}
    # 키움 OpenAPI는 appkey와 secretkey 파라미터를 사용합니다.
    data = {
        "grant_type": "client_credentials",
        "appkey": API_KEY,
        "secretkey": API_SECRET
    }
    
    try:
        response = requests.post(url, headers=headers, json=data, timeout=10)
        
        # 응답이 성공이 아닌 경우 예외를 발생시키기 전에 내용 확인
        if response.status_code != 200:
            print(f"[오류] 토큰 발급 API 오류 (상태 코드: {response.status_code})")
            print("응답 원본:", response.text)
            return None
            
        res_data = response.json()
        
        # 응답 구조에서 토큰 추출 (토큰 필드명이 token 또는 access_token일 수 있음)
        token = res_data.get("token") or res_data.get("access_token")
        
        if token:
            print("[성공] 접근 토큰 발급 완료!")
            return token
        else:
            print("[오류] 응답에 토큰 정보가 없습니다. 응답 원본:", res_data)
            return None
            
    except requests.exceptions.RequestException as e:
        print(f"[네트워크 오류] 요청 실패: {e}")
        return None
    except Exception as e:
        print(f"[오류] 알 수 없는 오류 발생: {e}")
        return None


def get_current_price(token, stock_code):
    """발급받은 토큰을 이용해 특정 종목의 현재가를 조회합니다."""
    # 주식기본정보요청(ka10001) 엔드포인트
    url = f"{KIWOOM_HOST}/api/dostk/stkinfo"
    
    headers = {
        "Content-Type": "application/json;charset=UTF-8",
        "authorization": f"Bearer {token}",
        "api-id": "ka10001"  # TR ID
    }
    
    data = {
        "stk_cd": stock_code  # 종목코드
    }
    
    try:
        response = requests.post(url, headers=headers, json=data, timeout=10)
        
        if response.status_code != 200:
            print(f"[오류] 현재가 조회 API 오류 (상태 코드: {response.status_code})")
            print("응답 원본:", response.text)
            return None
            
        res_data = response.json()
        print("\n[API 응답 원본 JSON]")
        print(json.dumps(res_data, indent=4, ensure_ascii=False))
        
        # 결과에서 현재가 추출
        # 키움증권 응답 구조는 최상위에 데이터가 있습니다.
        current_price_raw = res_data.get("cur_prc")
        
        if current_price_raw:
            # 가격 앞의 부호(+, -)는 전일대비 상승/하락을 의미하므로 제거하여 절대값으로 표시
            current_price = current_price_raw.replace("-", "").replace("+", "")
            print("\n" + "=" * 50)
            print(f"[종목코드: {stock_code}] 현재가 정보")
            print("=" * 50)
            print(f"현재가: {current_price}원")
            print("=" * 50)
            return current_price
        else:
            print("[오류] 응답에서 현재가 정보를 찾을 수 없습니다. 위 응답 원본 JSON 구조를 확인해주세요.")
            return None

    except requests.exceptions.RequestException as e:
        print(f"[네트워크 오류] 요청 실패: {e}")
        return None
    except Exception as e:
        print(f"[오류] 알 수 없는 오류 발생: {e}")
        return None


if __name__ == "__main__":
    if not API_KEY or not API_SECRET:
        print("[오류] 환경변수에서 API_KEY 또는 API_SECRET을 찾을 수 없습니다.")
        print(".env 파일이 존재하는지, 키가 올바르게 설정되었는지 확인해주세요.")
    else:
        print("[정보] .env 환경변수 로드 완료. API 키를 확인했습니다.")
        print("토큰 발급을 시도합니다...")
        
        access_token = get_access_token()
        
        if access_token:
            print(f"삼성전자({SAMSUNG_ELEC_CODE}) 현재가 조회를 시도합니다...")
            get_current_price(access_token, SAMSUNG_ELEC_CODE)
