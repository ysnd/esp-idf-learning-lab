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
    const char *tag = "MASTER";
    uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    // Config UART
    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_PORT, BUF_SIZE*2, 0, 0, NULL, 0);

    // Config DE/RE pin
    gpio_set_direction(DE_RE_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DE_RE_PIN, 0); // start in receive mode

    uint8_t rx_buf[BUF_SIZE];

    while (1) {
        const char *msg = "HELLO_FROM_MASTER";
        printf("%s: Preparing to send: %s\n", tag, msg);

        // Enable driver (TX)
        gpio_set_level(DE_RE_PIN, 1);
        // Wait tiny bit for driver to enable (1-2 ms)
        vTaskDelay(pdMS_TO_TICKS(2));

        // Send data
        int tx_bytes = uart_write_bytes(UART_PORT, (const char*)msg, strlen(msg));
        uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(100)); // wait flush

        // Back to receive
        vTaskDelay(pdMS_TO_TICKS(2));
        gpio_set_level(DE_RE_PIN, 0);

        printf("%s: Sent %d bytes, waiting reply...\n", tag, tx_bytes);

        // Read reply (blocking with timeout)
        int len = uart_read_bytes(UART_PORT, rx_buf, BUF_SIZE - 1, pdMS_TO_TICKS(500));
        if (len > 0) {
            rx_buf[len] = 0;
            printf("%s: Received %d bytes: %s\n", tag, len, (char*)rx_buf);
        } else {
            printf("%s: No reply (timeout)\n", tag);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
 
