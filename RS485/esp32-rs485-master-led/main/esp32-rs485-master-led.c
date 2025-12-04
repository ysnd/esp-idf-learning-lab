#include <stdio.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "MASTER"

#define UART_PORT UART_NUM_1
#define TXD_PIN   17
#define RXD_PIN   16
#define DERE_PIN  4

#define START_BYTE 0xAA
#define SLAVE_ADDR 0x01

#define CMD_PING     0x10
#define CMD_LED_ON   0x30
#define CMD_LED_OFF  0x31

void rs485_set_tx()
{
    gpio_set_level(DERE_PIN, 1);
    esp_rom_delay_us(2000);      
}

void rs485_set_rx()
{
    esp_rom_delay_us(2000);       
    gpio_set_level(DERE_PIN, 0);
}

void send_frame(uint8_t cmd)
{
    uint8_t tx[4];
    tx[0] = START_BYTE;
    tx[1] = SLAVE_ADDR;
    tx[2] = cmd;
    tx[3] = 0x00;   // no payload

    rs485_set_tx();
    uart_write_bytes(UART_PORT, (char*)tx, 4);
    uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(20));
    rs485_set_rx();
}

int recv_response(uint8_t *buf, int max_len)
{
    int len = uart_read_bytes(UART_PORT, buf, max_len, pdMS_TO_TICKS(200));
    return len;
}

void app_main()
{
    gpio_set_direction(DERE_PIN, GPIO_MODE_OUTPUT);
    rs485_set_rx();

    const uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_PORT, 256, 0, 0, NULL, 0);

    ESP_LOGI(TAG, "MASTER READY");

    uint8_t rxbuf[32];

    while (1) {
        ESP_LOGI(TAG, "Send PING");
        send_frame(CMD_PING);

        int len = recv_response(rxbuf, sizeof(rxbuf));
        if (len > 0) {
            ESP_LOGI(TAG, "PING RESP:");
            for (int i = 0; i < len; i++)
                printf("%02X ", rxbuf[i]);
            printf("\n");
        } else {
            ESP_LOGW(TAG, "NO RESP");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "Send LED ON");
        send_frame(CMD_LED_ON);

        len = recv_response(rxbuf, sizeof(rxbuf));
        if (len > 0) {
            ESP_LOGI(TAG, "LED ON RESP:");
            for (int i = 0; i < len; i++)
                printf("%02X ", rxbuf[i]);
            printf("\n");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "Send LED OFF");
        send_frame(CMD_LED_OFF);

        len = recv_response(rxbuf, sizeof(rxbuf));
        if (len > 0) {
            ESP_LOGI(TAG, "LED OFF RESP:");
            for (int i = 0; i < len; i++)
                printf("%02X ", rxbuf[i]);
            printf("\n");
        }

        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}

