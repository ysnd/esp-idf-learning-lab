#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#define UART_PORT       UART_NUM_1
#define TXD_PIN         17
#define RXD_PIN         16
#define DE_RE_PIN       4

#define BUF_SIZE        1024
#define BAUD_RATE       9600

void app_main(void)
{
    const char *tag = "SLAVE";
    uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_PORT, BUF_SIZE*2, 0, 0, NULL, 0);

    gpio_set_direction(DE_RE_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DE_RE_PIN, 0); // start in receive

    uint8_t data[BUF_SIZE];

    while (1) {
        int len = uart_read_bytes(UART_PORT, data, BUF_SIZE - 1, pdMS_TO_TICKS(1000));
        if (len > 0) {
            data[len] = 0;
            printf("%s: Received %d bytes: %s\n", tag, len, (char*)data);

            // reply "OK" back
            const char *reply = "OK_FROM_SLAVE";
            // enable TX
            gpio_set_level(DE_RE_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(2));
            uart_write_bytes(UART_PORT, reply, strlen(reply));
            uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(100));
            vTaskDelay(pdMS_TO_TICKS(2));
            gpio_set_level(DE_RE_PIN, 0);
            printf("%s: Replied\n", tag);
        } else {
            // nothing received, loop
        }
    }
}

