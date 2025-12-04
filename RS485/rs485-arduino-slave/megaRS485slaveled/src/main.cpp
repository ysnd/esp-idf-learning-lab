#include <Arduino.h>

#define SLAVE_ADDR 0x01
#define START_BYTE 0xAA

#define CMD_PING 0x10
#define CMD_LED_ON 0x30
#define CMD_LED_OFF 0x31

const int LED_PIN = 13;
const int DERE_PIN = 2;

HardwareSerial &rs485 = Serial1;

void rs485Write(byte *buffer, uint8_t len) {
  digitalWrite(DERE_PIN, HIGH);
  delayMicroseconds(2000);
  rs485.write(buffer, len);
  rs485.flush();
  delayMicroseconds(2000);
  digitalWrite(DERE_PIN, LOW);
}

void sendACK(byte cmd, byte value) {
    byte tx[5] = {
        START_BYTE,
        SLAVE_ADDR,
        (byte)(cmd | 0x80), //respon flag
        0x01,   //payload length
        value
    };
    rs485Write(tx, 5);
}

void handleFrame(byte *f) {
    byte addr = f[1];
    byte cmd = f[2];

    if (addr != SLAVE_ADDR) return;

    switch (cmd) {
        case CMD_PING:
            sendACK(cmd, 0x55); // arbitary ok
            break;
        
        case CMD_LED_ON:
            digitalWrite(LED_PIN, HIGH);
            sendACK(cmd, 1);
            break;

        case CMD_LED_OFF:
            digitalWrite(LED_PIN, LOW);
            sendACK(cmd, 0);
            break;
    }
}

void setup() {
    pinMode(DERE_PIN, OUTPUT);
    digitalWrite(DERE_PIN, LOW);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    rs485.begin(9600);
}

byte frame[32];
uint8_t idx = 0;
bool collecting = false;

void loop() {
    while (rs485.available()) {
        byte b = rs485.read();

        if (!collecting && b == START_BYTE) {
            idx = 0;
            frame[idx++] = b;
            collecting = true;
        } else if (collecting) {
            frame[idx++] = b;

            if (idx >=4) {
                uint8_t expected = frame[3] + 4;
                if (idx == expected) {
                    handleFrame(frame);
                    collecting = false;
                }
            }
        }
    }
}

