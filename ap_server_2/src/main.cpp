
/*
* eps32 access point server basic code 
* created 19/1/2026
*/

#include <Arduino.h>
#include <apServer.h>

#define TCP_PORT 8080
#define RX_BUF_SIZE 256

const char* AP_SSID = "ESP32_AP";
const char* AP_PASS = "12345678";   // min 8 chars

apServer as(TCP_PORT);

void setup() {
  Serial.begin(115200);
  Serial.println("sp_server_2");
  as.Init(AP_SSID,AP_PASS);                                               // Initialize serial communication
  Serial.println("TCP server ready");  
}

void loop(){ 
  as.client_connect();
  if(as.cmd_available()){
    as.reply();
  }
}

