#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  pinMode(13,OUTPUT);

}

void loop() {
  Serial.println("on");
  digitalWrite(13,HIGH);
  delay(2000);
  Serial.println("off");
  digitalWrite(13,LOW);
  delay(2000);
}

