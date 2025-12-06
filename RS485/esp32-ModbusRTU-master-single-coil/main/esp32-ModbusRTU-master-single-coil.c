#include <stdio.h>
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#define UART UART_NUM_1
#define TX 17
#define RX 16
#define DERE 4

#define SLAVE_ADDR 0x01

uint16_t modbus_crc(uint8_t *buf, uint8_t len) {
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= buf[i];
        for (uint8_t b = 0; b < 8; b++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xA001;
            else crc >>= 1;
        }
    }
    return crc;
}

void rs485_send(uint8_t *frame, uint8_t len) {
    gpio_set_level(DERE, 1);
    esp_rom_delay_us(200);
    uart_write_bytes(UART, (const char*)frame, len);
    uart_wait_tx_done(UART, 20 / portTICK_PERIOD_MS);
    esp_rom_delay_us(200);
    gpio_set_level(DERE, 0);
}

void modbus_led_on() {
    uint8_t frame[8];
    frame[0] = SLAVE_ADDR;
    frame[1] = 0x05;
    frame[2] = 0x00;
    frame[3] = 0x01; //coil addr 
    frame[4] = 0xFF;
    frame[5] = 0x00;

    uint16_t crc = modbus_crc(frame, 6);
    frame[6] = crc & 0xFF;
    frame[7] = crc >> 8;
    rs485_send(frame, 8);
}

void modbus_led_off() {
    uint8_t frame[8];
    frame[0] = SLAVE_ADDR;
    frame[1] = 0x05;
    frame[2] = 0x00;
    frame[3] = 0x01;
    frame[4] = 0x00;
    frame[5] = 0x00;

    uint16_t crc = modbus_crc(frame, 6);
    frame[6] = crc & 0xFF;
    frame[7] = crc >> 8;
    rs485_send(frame, 8);
}

void app_main(void)
{
    gpio_set_direction(DERE, GPIO_MODE_OUTPUT);
    gpio_set_level(DERE, 0);

    const uart_config_t cfg = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_param_config(UART, &cfg);
    uart_set_pin(UART, TX, RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART, 256, 256, 0, NULL, 0);

    while (1) {
        printf("LED ON!\n");
        modbus_led_on();
        vTaskDelay(pdMS_TO_TICKS(2000));

        printf("LED OFF!\n");
        modbus_led_off();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
