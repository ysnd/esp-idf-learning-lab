#include <stdbool.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_https_ota.h"
#include "esp_http_client.h"

#define SSID "yourssid"
#define PASS "yourpassword"

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
}

static void ota_task(void *pvParameter) {
    printf("Starting OTA update...\n");
     const char *dummy_cert_pem = "";

    esp_http_client_config_t http_conf = {
        .url = "http://0.0.0.0:8070/ota_http.bin",
        .transport_type = HTTP_TRANSPORT_OVER_TCP,
        .timeout_ms = 5000,
        .cert_pem = dummy_cert_pem,
        .skip_cert_common_name_check = true,
        .disable_auto_redirect = false,
        .crt_bundle_attach = NULL,
        .use_global_ca_store = false,

    };

    esp_https_ota_config_t ota_conf = {
        .http_config = &http_conf,
        .partial_http_download = false,
    };

    esp_err_t ret = esp_https_ota(&ota_conf);
    if (ret == ESP_OK) {
        printf("OTA successfull to v2 upgrade, restarting...\n");
        esp_restart();
    } else {
        printf("OTA failed: %s\n", esp_err_to_name(ret));
    }
    vTaskDelete(NULL);
}

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init_sta();
    vTaskDelay(pdMS_TO_TICKS(10000));
    xTaskCreate(&ota_task, "ota_task", 8192, NULL, 5, NULL);
}

