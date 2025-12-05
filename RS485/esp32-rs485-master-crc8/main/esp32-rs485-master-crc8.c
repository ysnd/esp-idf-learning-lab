#include <stdio.h>
#include <string.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include <esp_rom_sys.h>

#define UART_NUM UART_NUM_1
#define TX 17
#define RX 16
#define DERE 4

#define START_BYTE 0xAA
#define SLAVE_ADDR 0x01

#define CMD_LED_ON 0x30
#define CMD_LED_OFF 0x31

uint8_t crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0xFF;

    for (uint8_t i = 0 ; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}
       

void rs485Write(uint8_t *buf, uint8_t len) {
    gpio_set_level(DERE, 1);
    esp_rom_delay_us(20);
    uart_write_bytes(UART_NUM, (const char *)buf, len);
    uart_wait_tx_done(UART_NUM, pdMS_TO_TICKS(20));
    gpio_set_level(DERE, 0);
}

void send_cmd(uint8_t cmd) {
    uint8_t frame[8];
    frame[0] = START_BYTE;
    frame[1] = SLAVE_ADDR;
    frame[2] = cmd;
    frame[3] = 0x00;   // no data

    uint8_t crc = crc8(frame, 4);
    frame[4] = crc;

    printf("Send CMD 0x%02X (CRC=0x%02X)\n", cmd, crc);

    rs485Write(frame, 5);
} 

void read_response() {
    uint8_t buf[32];
    int len = uart_read_bytes(UART_NUM, buf, sizeof(buf), pdMS_TO_TICKS(200));
    if (len <= 0) return;

    //minimum frame start addr cnd len crc = 5
    if (len < 5) {
        printf("Frame too short\n");
        return;
    }
    //check start byte 
    if (buf[0] != START_BYTE) {
        printf("Invalid start byte\n");
        return;
    }

    uint8_t addr = buf[1];
    uint8_t cmd = buf[2];
    uint8_t datalen = buf[3];

    uint8_t expected_len = 4 + datalen + 1;
    if (len != expected_len) {
        printf("Wrong length (%d ! = %d)\n", len, expected_len);
        return;
    }
    uint8_t crc_received = buf[len - 1];
    uint8_t crc_calc = crc8(buf, len - 1);

    if (crc_received != crc_calc) {
        printf("CRC Missmatch! recv = 0x%02X calc = 0x%02X\n", crc_received, crc_calc);
        return;
    }
    printf("Valid frtame from slave!\n");
    printf("ADDR: %02X CMD: %02X LEN: %d\n", addr, cmd, datalen);

    if (datalen > 0) {
        printf("Data: %d\n", buf[4]);
    }
}


void app_main(void)
{
    gpio_reset_pin(DERE);
    gpio_set_direction(DERE, GPIO_MODE_OUTPUT);
    gpio_set_level(DERE, 0);

    //UART conf
    uart_config_t cfg = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM, &cfg);
    uart_set_pin(UART_NUM, TX, RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM, 2048, 2048, 0, NULL, 0);

    printf("RS485 Master Ready\n");

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    while (1) {
        printf("Send LED ON\n");
        send_cmd(CMD_LED_ON);
        vTaskDelay(pdMS_TO_TICKS(200));
        read_response();
        vTaskDelay(pdMS_TO_TICKS(1000));
        printf("Send LED OFF\n");
        send_cmd(CMD_LED_OFF);
        vTaskDelay(pdMS_TO_TICKS(200));
        read_response();
        vTaskDelay(pdMS_TO_TICKS(1000));
        
    }
}
