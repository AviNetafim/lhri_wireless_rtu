/*
* temp version of lhri bridge - derived in parts from ap_server_2 and lhri_bridge  to idetifiy the  program crash cause 
* created 21/1/2026
*/ 

#include <Arduino.h>
#include <apServer.h>

#define TCP_PORT 8080
#define RX_BUF_SIZE 256

const char* AP_SSID = "ESP32_AP";
const char* AP_PASS = "12345678";   // min 8 chars

apServer as(TCP_PORT);

//---------------------------7-------  RTU registers ---------------------------------------------------

uint16_t r_send[1] = {0x0};                                   // send command to lhri tree
uint16_t r_trs[7] = {0x0,0x05,0x05,0x1,0x0c,0x05,0x0};                  // transmit msg fields
uint16_t r_rec[9] = {0x0,0x05,0x05,0x1,0x0c,0x05,0x0,0x0,0x0};          // receive msg fields

registers regs[] = {                                                    // register list metadata
{1,0,0,&r_send[0]},
{7,0,0,&r_trs[0]},
{9,0,0,&r_rec[0]}
};

void setup() {
  Serial.begin(115200);
  Serial.println("ap_server_3:");
  as.Init(AP_SSID,AP_PASS);                                               // Initialize serial communication
  Serial.println("TCP server ready");  
}

void loop(){
  as.client_connect();
  if(as.cmd_available(regs)){ 
    as.reply();
  }
}