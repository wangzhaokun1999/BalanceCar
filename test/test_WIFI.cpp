#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

static const char *TAG = "wifi_ap";

// 配置热点参数
wifi_config_t wifi_config = {
    .ap = {
        .ssid = "zwang",                      // 自定义名称
        .ssid_len = 0,                        // 0 表示自动计算长度
        .channel = 1,                         // 尽量避开常用信道
        .password = "12345678",               // 至少8位，否则无法启用加密
        .authmode = WIFI_AUTH_WPA2_PSK,       // 强烈推荐此模式
        .max_connection = 4,                  // 控制并发数量
        .pmf_cfg = {.required = false},       // PMF 可选关闭
    },
};
//mDNS 初始化 
void mdns_setup(void)
{
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set("mydevice"));
    ESP_ERROR_CHECK(mdns_instance_name_set("ESP32 Web Device"));

    ESP_LOGI(TAG, "🌐 mDNS 已启动，可通过 http://mydevice.local 访问");
}

// 事件回调函数声明
void on_wifi_ap_start(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data);
void on_client_connected(void *arg, esp_event_base_t event_base,
                         int32_t event_id, void *event_data);
void on_client_disconnected(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data);

void wifi_init_softap(void)
{
    // 初始化 TCP/IP 网络栈
    ESP_ERROR_CHECK(esp_netif_init());

    // 创建默认事件循环
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 创建 AP 模式的网络接口（关键！）
    esp_netif_create_default_wifi_ap();

    // 初始化 Wi-Fi 子系统
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 注册事件监听
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        WIFI_EVENT_AP_START,
                                                        &on_wifi_ap_start,
                                                        NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        WIFI_EVENT_AP_STACONNECTED,
                                                        &on_client_connected,
                                                        NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        WIFI_EVENT_AP_STADISCONNECTED,
                                                        &on_client_disconnected,
                                                        NULL, NULL));

    // 设置为 AP 模式
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    // 应用配置
    ESP_ERROR_CHECK(esp_wifi_set_config(&wifi_config));

    // 启动 Wi-Fi
    ESP_ERROR_CHECK(esp_wifi_start());
}

// 回调函数实现
void on_wifi_ap_start(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "✅ AP 已启动，等待客户端...");
}

void on_client_connected(void *arg, esp_event_base_t event_base,
                         int32_t event_id, void *event_data)
{
    wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
    ESP_LOGI(TAG, "🟢 新设备连接 - MAC: " MACSTR ", AID: %d",
             MAC2STR(event->mac), event->aid);
}

void on_client_disconnected(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data)
{
    wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
    ESP_LOGI(TAG, "🔴 客户端断开 - MAC: " MACSTR ", AID: %d",
             MAC2STR(event->mac), event->aid);
}

void app_main(void)
{
    // 初始化 NVS（用于保存 Wi-Fi 配置等）
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NEW_VERSION_DETECTED) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "🚀 开始启动 ESP32 Soft-AP...");

    wifi_init_softap();
}
