#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_client.h"

extern const uint8_t server_cert_pem_start[] asm("_binary_server_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_server_cert_pem_end");

#define SSID "yourssid"
#define PASS "yourpassword"
#define HTTPS_URL "https://192.168.1.1:8071"

static void wifi_init_sta(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = SSID,
            .password = PASS,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    printf("Connecting to WiFi...\n");
    esp_wifi_connect();
    vTaskDelay(pdMS_TO_TICKS(6000));

    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(netif, &ip_info);
    printf("ESP32 IP Address: " IPSTR "\n", IP2STR(&ip_info.ip));
}

static void https_get_task(void *pvParameters) {
    esp_http_client_config_t config = {
        .url = HTTPS_URL,
        .cert_pem = (const char *)server_cert_pem_start,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err ==ESP_OK) {
        printf("HTTPS Status = %d, content_length = %lld\n", esp_http_client_get_status_code(client), (long long)esp_http_client_get_content_length(client));
    } else {
        printf("HTTPS request failed: %s\n", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    vTaskDelete(NULL);
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    wifi_init_sta();

    xTaskCreate(&https_get_task, "https_get_task", 8192, NULL, 5, NULL);
}
