#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "mqtt_client.h"
#include "esp_https_ota.h"
#include "esp_netif.h"

#include "cJSON.h"

#define WIFI_SSID "yourssid"
#define WIFI_PASS "yourpassword"
#define MQTT_BROKER_URI "mqtt://192.168.1.1:1883"
#define OTA_TOPIC "ota/update"

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

static void perform_ota(const char *url)
{
    printf("Starting OTA from URL: %s", url);

    esp_http_client_config_t http_cfg = {
        .url = url,
        .skip_cert_common_name_check = true,
    };

    esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
    };

    esp_err_t ret = esp_https_ota(&ota_cfg);
    if (ret == ESP_OK)
    {
        printf("OTA Update successful, rebooting...");
        esp_restart();
    }
    else
    {
        printf("OTA Update failed! Error: %s", esp_err_to_name(ret));
    }
}


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        printf("Connected to MQTT broker!");
        esp_mqtt_client_subscribe(client, OTA_TOPIC, 1);
        break;

    case MQTT_EVENT_DATA:
        printf("Message received on topic: %.*s", event->topic_len, event->topic);
        char *payload = strndup(event->data, event->data_len);
        printf("Payload: %s", payload);

        cJSON *json = cJSON_Parse(payload);
        if (json)
        {
            const cJSON *url = cJSON_GetObjectItemCaseSensitive(json, "url");
            if (cJSON_IsString(url) && (url->valuestring != NULL))
            {
                perform_ota(url->valuestring);
            }
            else
            {
                printf("Invalid JSON format!");
            }
            cJSON_Delete(json);
        }
        free(payload);
        break;

    default:
        break;
    }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        nvs_flash_init();
    }

    wifi_init_sta();
    vTaskDelay(pdMS_TO_TICKS(5000));

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    printf("MQTT OTA system running...");
}

