#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_app_desc.h"
#include "cJSON.h"

#define SSID "yourssid"
#define PASS "yourpassword"
#define VERSION_JSON_URL "http://192.168.1.1:8070/version.json"

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

static void perform_ota(const char *url) {
    printf("Starting OTA from URL: %s\n", url);

    esp_http_client_config_t http_cfg = {
        .url = url,
        .skip_cert_common_name_check = true
    };

    esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
    };

    esp_err_t ret = esp_https_ota(&ota_cfg);
    if (ret == ESP_OK) {
        printf("OTA Update successful, rebooting...\n");
        esp_restart();
    } else {
        printf("OTA update failed: %s\n", esp_err_to_name(ret));
    }
}

static void check_and_update_firmware(void) {
    printf("Checking for firmware update...\n");

    esp_http_client_config_t config = {
        .url = VERSION_JSON_URL,
        .method = HTTP_METHOD_GET,
        .skip_cert_common_name_check = true
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int content_length = esp_http_client_get_content_length(client);
        char *buffer = malloc(content_length + 1);
        if (!buffer) {
            printf("Failed to allocate memory for response\n");
            esp_http_client_cleanup(client);
            return;
        }
        int read_len = esp_http_client_read_response(client, buffer, content_length);
        buffer[read_len] = '\0';

        printf("Received JSON: %s\n", buffer);

        cJSON *root = cJSON_Parse(buffer);
        if (root) {
            cJSON *ver = cJSON_GetObjectItemCaseSensitive(root, "version");
            cJSON *url = cJSON_GetObjectItemCaseSensitive(root, "url");

            if (cJSON_IsString(ver) && cJSON_IsString(url)) {
                const char *server_version = ver->valuestring;
                const char *firmware_url = url->valuestring;

                const esp_app_desc_t *app_desc = esp_app_get_description();
                const char *local_version = app_desc->version;

                printf("Current Firmware Version : %s\n", local_version);
                printf("Available Firmware Version: %s\n", server_version);
                if (strcmp(server_version, local_version) != 0) {
                    printf("New firmware available! Starting OTA\n");
                    perform_ota(firmware_url);
                } else {
                    printf("Firmware already up to date.\n");
                }
            }
            cJSON_Delete(root);
        }
        free(buffer);
    } else {
        printf("HTTP GET request failed: %s\n", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    wifi_init_sta();
    check_and_update_firmware();

    printf("Main loop complete. waiting...\n");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

