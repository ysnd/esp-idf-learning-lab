#include <Arduino.h>

#define SLAVE_ADDR 0x01
#define START_BYTE 0xAA
#define CMD_PING 0x10
#define CMD_LED_ON 0x30
#define CMD_LED_OFF 0x31

const int LED = 13;
const int DERE = 2;

HardwareSerial &rs485 = Serial1;

uint8_t crc8(uint8_t *data, uint8_t len) {
    uint8_t crc = 0xFF;
    for (uint8_t i = 0 ; i < len ; i++) {
        crc ^= data[i];
        for (uint8_t b = 0 ; b < 8 ; b++) {
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
    digitalWrite(DERE, HIGH);
    delayMicroseconds(150);
    rs485.write(buf, len);
    rs485.flush();
    delayMicroseconds(150);
    digitalWrite(DERE, LOW);
}

void sendACK(uint8_t cmd, uint8_t value) {
    uint8_t payload_len = 1;

    uint8_t tx[6];
    tx[0] = START_BYTE;
    tx[1] = SLAVE_ADDR;
    tx[2] = cmd | 0x80;
    tx[3] = payload_len;
    tx[4] = value;
    tx[5] = crc8(tx, 4 + payload_len);

    rs485Write(tx, 6);
}

void handleCMD(uint8_t cmd) {
    switch (cmd) {
        case CMD_PING:
            sendACK(cmd, 0x55);
            break;
        case CMD_LED_ON:
            digitalWrite(LED, HIGH);
            sendACK(cmd, 1);
            break;
        case CMD_LED_OFF:
            digitalWrite(LED, LOW);
            sendACK(cmd, 0);
            break;
    }
}

uint8_t frame[32];
uint8_t idx = 0;
bool collecting = false;

void setup() {
    pinMode(LED, OUTPUT);
    pinMode(DERE, OUTPUT);
    digitalWrite(DERE, LOW);

    rs485.begin(9600);

    Serial.begin(115200);
    Serial.println("SLAVE READY (CRC MODE)");
}

void loop() {
    while (rs485.available()) {
        uint8_t b = rs485.read();
        if (!collecting && b == START_BYTE) {
            idx = 0;
            frame[idx++] = b;
            collecting = true;
            continue;
        }
        if (collecting) {
            frame[idx++] = b;
            if (idx >=4) {
                uint8_t len = frame[3];
                uint8_t needed = 4 + len + 1; //+crc 
                if (idx == needed) {
                    uint8_t crc_calc = crc8(frame, 4 + len);
                    uint8_t crc_recv = frame[needed - 1];

                    if (crc_calc == crc_recv) {
                        uint8_t cmd = frame[2];
                        handleCMD(cmd);
                    }
                    collecting = false;
                }
            }
        }
    }

}

