#include <Arduino.h>
#define DE_RE 2

void setup() {
    pinMode(DE_RE, OUTPUT);
    digitalWrite(DE_RE, LOW);
    Serial.begin(115200);
    Serial1.begin(9600);
}

void loop() {
    if (Serial1.available()) {
        String data = Serial1.readString();
        Serial.print("SLAVE Mega recevied: ");
        Serial.println(data);

        //reply
        digitalWrite(DE_RE, HIGH);
        delay(2);
        Serial1.print("OK From MEGAWATI");
        Serial1.flush();    //wait complete
        delay(2);
        digitalWrite(DE_RE, LOW);
        Serial.println("Replied");
    }
}


