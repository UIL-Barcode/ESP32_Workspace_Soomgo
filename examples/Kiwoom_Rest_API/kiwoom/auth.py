import requests

from config.settings import Config


def fetch_token(session: requests.Session, config: Config) -> str | None:
    if not config.app_key or not config.app_secret:
        return None

    url = f"{config.base_url}/oauth2/token"
    payload = {
        "grant_type": "client_credentials",
        "appkey": config.app_key,
        "secretkey": config.app_secret,
    }
    response = session.post(url, json=payload, headers={"Content-Type": "application/json"}, timeout=10)
    response.raise_for_status()
    data = response.json()
    return data.get("token") or data.get("access_token")
