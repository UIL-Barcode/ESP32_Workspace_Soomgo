#include <stdint.h>
#include <string.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"

#define DEVICE_NAME "ESP32_SPP_ECHO"
#define SPP_SERVER_NAME "SPP_SERVER"
static const char *TAG = "BT_ECHO";

// SPP 이벤트 콜백 핸들러
static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
    switch (event) {
            case ESP_SPP_INIT_EVT:
            // SPP 초기화 완료 이벤트
            // SPP 프로필이 메모리에 정상적으로 로드됨
            ESP_LOGI(TAG, "ESP_SPP_INIT_EVT: Starting Server");
            esp_spp_start_srv(ESP_SPP_SEC_AUTHENTICATE, ESP_SPP_ROLE_SLAVE, 0, SPP_SERVER_NAME);
            break;
            
        case ESP_SPP_START_EVT:
            // SPP 서버 시작 완료 이벤트
            // 서버 소켓이 열리고 통신 대기 상태가 됨
            ESP_LOGI(TAG, "ESP_SPP_START_EVT: Server Started");
            esp_bt_gap_set_device_name(DEVICE_NAME);
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            break;
            
        case ESP_SPP_SRV_OPEN_EVT:
            // 클라이언트 연결됨
            // 상대 장비와 소켓 연결(페어링 후 실제 통신 세션 수립) 완료-통신 준비 완료(끝)
            ESP_LOGI(TAG, "ESP_SPP_SRV_OPEN_EVT: Client Connected");
            break;
            
        case ESP_SPP_DATA_IND_EVT:
            // 데이터 수신 이벤트
            // 상대 장비로부터 데이터가 수신됐음
            ESP_LOGI(TAG, "ESP_SPP_DATA_IND_EVT: Received %d bytes", param->data_ind.len);
            
            // 디버깅을 위한 헥사 덤프 출력 (선택 사항)
            esp_log_buffer_hex("RX_DATA", param->data_ind.data, param->data_ind.len);
            
            // 수신된 데이터를 그대로 클라이언트(앱)로 반환 (Echo)
            esp_spp_write(param->data_ind.handle, param->data_ind.len, param->data_ind.data);
            break;
            
        case ESP_SPP_CLOSE_EVT:
            // 클라이언트 연결 끊김
            // 상대 장비의 종료/블루투스 끊김/통신거리 벗어남 등으로 인해 연결 유실
            ESP_LOGI(TAG, "ESP_SPP_CLOSE_EVT: Client Disconnected");
            break;
            
        default:
            break;
    }
}

void app_main(void) {
    // 1. NVS 플래시 초기화 (블루투스 페어링 정보 저장용)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. 블루투스 컨트롤러 메모리 해제 및 초기화
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
    
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if (esp_bt_controller_init(&bt_cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Bluetooth controller initialize failed");
        return;
    }
    if (esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT) != ESP_OK) {
        ESP_LOGE(TAG, "Bluetooth controller enable failed");
        return;
    }

    // 3. Bluedroid 스택 초기화
    if (esp_bluedroid_init() != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid initialize failed");
        return;
    }
    if (esp_bluedroid_enable() != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid enable failed");
        return;
    }

    // 4. SPP 프로필 콜백 등록 및 초기화
    if (esp_spp_register_callback(esp_spp_cb) != ESP_OK) {
        ESP_LOGE(TAG, "SPP register callback failed");
        return;
    }
    if (esp_spp_init(ESP_SPP_MODE_CB) != ESP_OK) {
        ESP_LOGE(TAG, "SPP initialize failed");
        return;
    }

    ESP_LOGI(TAG, "✅ [System] SPP Echo Server is running. Device Name: %s", DEVICE_NAME);
}