#include "wifi_ap.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "mdns.h"
#include "esp_err.h"

#include <string.h>

static const char *TAG = "wifi_ap";

wifi_config_t wifi_config;

void mdns_setup()
{
    esp_err_t err;

    err = mdns_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "mdns_init failed: %s", esp_err_to_name(err));
        return;
    }

    mdns_hostname_set("zwang");
    mdns_instance_name_set("ESP32 SoftAP Device");

    ESP_LOGI(TAG, "mDNS started: http://zwang.local");
}

void on_wifi_ap_start(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "AP started event received");
    mdns_setup();
}

void on_client_connected(void *arg, esp_event_base_t event_base,
                         int32_t event_id, void *event_data)
{
    wifi_event_ap_staconnected_t *event =
        (wifi_event_ap_staconnected_t *)event_data;

    ESP_LOGI(TAG,
             "Client connected MAC:" MACSTR " AID:%d",
             MAC2STR(event->mac),
             event->aid);
}

void on_client_disconnected(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data)
{
    wifi_event_ap_stadisconnected_t *event =
        (wifi_event_ap_stadisconnected_t *)event_data;

    ESP_LOGI(TAG,
             "Client disconnected MAC:" MACSTR " AID:%d",
             MAC2STR(event->mac),
             event->aid);
}

void wifi_init_softap()
{
    esp_err_t err;

    ESP_LOGI(TAG, "wifi_init_softap begin");

    err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "esp_netif_init failed: %s", esp_err_to_name(err));
        return;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "esp_event_loop_create_default failed: %s", esp_err_to_name(err));
        return;
    }

    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    if (ap_netif == NULL)
    {
        ESP_LOGE(TAG, "esp_netif_create_default_wifi_ap failed");
        return;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_init failed: %s", esp_err_to_name(err));
        return;
    }

    memset(&wifi_config, 0, sizeof(wifi_config));

    const char *ssid = "zwang";
    const char *password = "12345678";

    strcpy((char *)wifi_config.ap.ssid, ssid);
    strcpy((char *)wifi_config.ap.password, password);

    wifi_config.ap.ssid_len = strlen(ssid);
    wifi_config.ap.channel = 1;
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.ap.beacon_interval = 100;

    err = esp_event_handler_instance_register(
        WIFI_EVENT,
        WIFI_EVENT_AP_START,
        &on_wifi_ap_start,
        NULL,
        NULL);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "register AP_START failed: %s", esp_err_to_name(err));
        return;
    }

    err = esp_event_handler_instance_register(
        WIFI_EVENT,
        WIFI_EVENT_AP_STACONNECTED,
        &on_client_connected,
        NULL,
        NULL);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "register STACONNECTED failed: %s", esp_err_to_name(err));
        return;
    }

    err = esp_event_handler_instance_register(
        WIFI_EVENT,
        WIFI_EVENT_AP_STADISCONNECTED,
        &on_client_disconnected,
        NULL,
        NULL);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "register STADISCONNECTED failed: %s", esp_err_to_name(err));
        return;
    }

    err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_set_mode failed: %s", esp_err_to_name(err));
        return;
    }

    err = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_set_config failed: %s", esp_err_to_name(err));
        return;
    }

    err = esp_wifi_start();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_start failed: %s", esp_err_to_name(err));
        return;
    }

    esp_netif_ip_info_t ip_info;
    err = esp_netif_get_ip_info(ap_netif, &ip_info);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "SoftAP started successfully");
        ESP_LOGI(TAG, "SSID: %s", ssid);
        ESP_LOGI(TAG, "Password: %s", password);
        ESP_LOGI(TAG, "IP: " IPSTR, IP2STR(&ip_info.ip));
    }
    else
    {
        ESP_LOGW(TAG, "SoftAP started but failed to get IP info");
    }
}