#include <Arduino.h>

#define SLAVE_ADDR 0x01
#define DERE 2
#define LED 13

HardwareSerial &rs485 = Serial1;

uint16_t modbus_crc(uint8_t *buf, uint8_t len) {
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= buf[i];
        for (uint8_t b = 0 ; b<8 ; b++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xA001;
            else crc >>=1;    
        }
    }
    return crc;
}

void rs485Write(uint8_t *buf, uint8_t len) {
    digitalWrite(DERE, HIGH);
    delayMicroseconds(150);

    rs485.write(buf, len);
    rs485.flush();

    delayMicroseconds(150);
    digitalWrite(DERE, LOW);
}

void proccesWriteSingleCoil(uint8_t *frame) {
    uint16_t coilAddr = (frame[2] << 8) | frame[3];
    uint16_t value = (frame[4] << 8) | frame[5];

    if (coilAddr == 0x0001) {
        if (value == 0xFF00) digitalWrite(LED, HIGH);
        else digitalWrite(LED, LOW);
    }

    //echo balik
    uint8_t resp[8];
    memcpy(resp, frame, 6);
    uint16_t crc = modbus_crc(resp, 6);
    resp[6] = crc & 0xFF;
    resp[7] = (crc >> 8) & 0xFF;

    rs485Write(resp, 8);
}

uint8_t buf[32];
uint8_t idx = 0;

void setup() {
    pinMode(DERE, OUTPUT);
    digitalWrite(DERE, LOW);

    pinMode(LED, OUTPUT);
    rs485.begin(9600);
    Serial.begin(115200);
    Serial.println("Modbus Slave Ready");
}

void loop() {
    while (rs485.available()) {
        uint8_t b = rs485.read();
        buf[idx++] = b;

        if (idx >= 8) {
            if (buf[0] == SLAVE_ADDR && buf[1] == 0x05) {
                uint16_t crc_recv = (buf[7] << 8) | buf[6];
                uint16_t crc_calc = modbus_crc(buf, 6);

                if (crc_recv == crc_calc) {
                    proccesWriteSingleCoil(buf);
                }
            }
            idx = 0;
        }
    }
}
