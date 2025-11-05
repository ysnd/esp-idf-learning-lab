#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "driver/gpio.h"

#define WIFI_SSID ""
#define WIFI_PASS ""
#define LED_GPIO 2

static void wifi_init_sta(void);
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

static void wifi_init_sta(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    printf("Connecting to WiFi...\n");
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            printf("Connected to HiveMQ Cloud\n");
            esp_mqtt_client_subscribe(client, "esp32/led", 0);
            break;

        case MQTT_EVENT_DATA:
            printf("Topic: %.*s | Data: %.*s\n", event->topic_len, event->topic, event->data_len, event->data);
            if (strncmp(event->data, "ON", event->data_len) == 0) {
                gpio_set_level(LED_GPIO, 1);
                printf("LED ON\n");
            } else if (strncmp(event->data, "OFF", event->data_len) == 0) {
                gpio_set_level(LED_GPIO, 0);
                printf("LED OFF\n");
            }
            break;

        case MQTT_EVENT_DISCONNECTED:
            printf("Disconnected from MQTT broker\n");
            break;

        default:
            break;
    }
}

void app_main(void) {
    nvs_flash_init();
    wifi_init_sta();

    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 0);

   
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = "",
            .verification.skip_cert_common_name_check = true,
        },
        .credentials = {
            .username = "",
            .authentication.password = "s",
        },
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

