#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_netif.h"
#include "freertos/projdefs.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_http_client.h"
#include "esp_event.h"

#define SSID "yourssid"
#define PASS "yourpassword"
#define SERVER_URL "http://192.168.1.1:8071/data"

void wifi_init_sta(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_cfg = {
        .sta = {
            .ssid = SSID,
            .password = PASS
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    //activing power save
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MIN_MODEM));
    printf("power save mode: MIN_MODEM\n");

    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
}

static void send_sensor_data_task(void *pvParametes) {
    while (1) {
        //this is simulation reading send data sensor
        float temp = 25.0 + (esp_random() % 1000) / 100.0;

        char post_data[64];
        snprintf(post_data, sizeof(post_data), "{\"temp\": %2.f}", temp);

        //send data to server via http 
        esp_http_client_config_t conf = {
            .url = SERVER_URL,
            .method = HTTP_METHOD_POST
        };
        esp_http_client_handle_t client = esp_http_client_init(&conf);
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_post_field(client, post_data, strlen(post_data));

        esp_err_t err = esp_http_client_perform(client);
            if (err == ESP_OK) {
                printf("Sent data: %s | Status = %d\n", post_data, esp_http_client_get_status_code(client));
            } else {
                printf("HTTP POST failed: %s", esp_err_to_name(err));
            }
            esp_http_client_cleanup(client);
            vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init_sta();
    xTaskCreate(&send_sensor_data_task, "send_sensor_data_task", 8192, NULL, 5, NULL);
}
