/*
* test eps32-pico-mini code to use gpio interrupt 
* test again  after integration with wireless server functionalities
* created 1/1/2026
*/

#include <Arduino.h>

static const uint8_t GPIO_INT_PIN = 4;
volatile bool rec_flag = false;
uint8_t rec_cnt;

void IRAM_ATTR gpio4_isr(){
  rec_flag = true;
}

void enable_gpio4_interrupt(){
	// Enable interrupt
  attachInterrupt(digitalPinToInterrupt(GPIO_INT_PIN), gpio4_isr,FALLING);
}

void disable_gpio4_interrupt(){
	//Disable interrupt */
	detachInterrupt(digitalPinToInterrupt(GPIO_INT_PIN));
}


void setup() {
  Serial.begin(115200);
  pinMode(GPIO_INT_PIN, INPUT_PULLUP);
  enable_gpio4_interrupt();
  Serial.println("GPIO interrupt test");
}

void loop() {
  if (rec_flag){
	  rec_flag = false;
    rec_flag = 0;
    Serial.println(rec_cnt++);
  }
}

