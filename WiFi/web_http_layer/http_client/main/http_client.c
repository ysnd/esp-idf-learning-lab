#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_client.h"

#define SSID "yourssid"
#define PASS "yourpassword"

#define WIFI_CONNECTED_BIT BIT0
static EventGroupHandle_t s_wifi_event_group;

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect(); 
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        printf("Disconected, retrying...\n");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        printf("Got IP: " IPSTR "\n", IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init_sta(void) {
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = SSID,
            .password = PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    printf("Connecting to WiFi...\n");
}

static void http_get_example(void) {
    esp_http_client_config_t config = {
        .url = "https://httpbin.org/get",
        .cert_pem = NULL,
    };

    printf("=== Performing HTTP GET ===\n");
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err  = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        int content_length = esp_http_client_get_content_length(client);
        printf("GET Status = %d, content_length = %d", status, content_length);
    } else {
        printf("HTTP GET reqwuest failed: %s\n", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}

static void http_post_example(void) {
    printf("=== Performing HTTP POST ===\n");
    esp_http_client_config_t config = {
        .url = "https://httpbin.org/post",
        .cert_pem = NULL,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    const char *post_data = "{\"message\": \"Hello from ESP32!\"}";
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        printf("POST Status = %d, content_length = %lld\n",
                esp_http_client_get_status_code(client), 
                esp_http_client_get_content_length(client));
    } else {
        printf("HTTP POST request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init_sta();
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        printf("Wifi Connected, proceeding to HTTP requests...\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
        http_get_example();
        vTaskDelay(pdMS_TO_TICKS(2000));
        http_post_example();
    } else {
        printf("Failed to connect to wifi.\n");
    }
}
