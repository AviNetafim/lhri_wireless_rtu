/*
* local serial monitor control over  the bridge - combine lhri_client_11 with esp32 specific code 
* works with lhri_node_10 ??
* serial monitor commands 
* s - send message 
* r - set dir (0,1,2)
* l - level
* d - dest level
* t - path
* c - command
* p - payload
* created 04/02/2026
*/


#include <Arduino.h>
#include "ManCode.h"


#define WAIT_SEND 0                                                     // client states
#define SEND 1
#define WAIT_RECEIVE 2
#define RECEIVE 3


#define CPU_CLOCK 80000000                                              // esp32 has 80Mhz clock
#define BAUDRATE 300                                                    // half bit time = 1667us
#define DIVIDER 40 // divide clock by 40                                // clock is set to 80e6 /40 = 2Mhz -> 0.5us
#define NO_RESP_TIME 1800                                               // no response timeout for defined bauderate is 1.67 * 1800 = 3s
#define PRINT_ON 1                                                      // serial monitor printing swtich

#define DIR 0 // indexes for received message fields
#define LEVEL 1
#define DLEVEL 2
#define PATH0 3
#define PATH1 4
#define CMD 5
#define PLOAD 6
#define ERR 7
#define PORT 8


// ------------------------------ constants and class instances -----------------------------------

const String  ParameterList = "st";                                     // serial monitor command code string
const uint16_t half_bit = CPU_CLOCK/(DIVIDER)/2/BAUDRATE;               // assuming frequency divider of 40 - clock time is 0.5us
const uint16_t byte_delay = half_bit * 3;
ManCode mc;     

// ------------------------------ program variables ----------------------------------------------

String inString;                                                        // serial monitor input buffer 
volatile bool byte_start = false;
uint8_t state = WAIT_SEND;                                              // client state
uint8_t on_delay = 0;                                                   // on delay flag indicating  edge detected and on delay timer is active
uint8_t send = 0;                                                       // varaible set by serial monitor command to startsending a message
uint8_t sent_byte = 0;
uint16_t nrtoc = 0;                                                     // no response timeout counter
uint8_t send_port = UP;                                                 // node transmission GPIO for the allocated port
unsigned long start_ms;
unsigned long byte_end_ms;

//----------------------------------- functions declarrations ----------------------------------------

void parse_parameter();


void setup() {
  Serial.begin(115200);
  Serial.println("ap_server_7");
  mc.Init(DIVIDER);
}

void loop(){
  int i,j;
  switch(state){
    case WAIT_SEND:
      while(Serial.available()>0){  
        inString = Serial.readStringUntil('\n');  
        parse_parameter();
      }

      // ------------------------------send command received from host-----------------------------

      if (send == 1){
        send = 0;
        start_ms = micros();
        state = SEND;                                                   // send message, to which GPIO output? TBD
        mc.reset_timer();
      } 
    break;

    case SEND:
      if (mc.get_timer_cnt() > half_bit){                           // wait for timer1 half bit count
        mc.reset_timer();
        if (mc.SendByte(sent_byte,UP)){                               // byte end when true
          state = WAIT_SEND;                                         // enable receiver for server respond 
            //   byte_end_ms = micros();
            //   Serial.println(byte_end_ms - start_ms);
          Serial.print("start: ");
          Serial.println(start_ms);
          for (i= 0; i < 20 ; i++){
            Serial.print(i);
            Serial.print(": ");
            for (j= 0; j < 8 ; j++){
              Serial.print(mc.log_data[i][j]);
              Serial.print(",");
            } 
            Serial.println();
          } 
          Serial.println();          
          mc.log_cnt = 0;
            //     for (j=0 ; j< 5 ; j++){
            //       Serial.print(mc.log_data[i][j]);
            //       Serial.print(",");
            //     }
            //   Serial.println();
            //   mc.
            //   }
        } 
      }
    break;
  }
}

void parse_parameter(){
  // parse serial monitor input
  char ParameterCode;                                                   // paremeter code - the first character in paramater input string
  int ParNo;                                                            // Paramter location in the code string 
  long ParVal;                                                          // parameter value 
  char char_array[16];
  ParameterCode = inString.charAt(0);   
  ParNo = ParameterList.indexOf(ParameterCode);
  inString.substring(1,8).toCharArray(char_array, sizeof(char_array));  // get serial monitor command data into char array 
  ParVal = strtol(char_array, NULL, 16);                                //  convert char array to hex number
  if (ParNo >= 0)  {
    switch(ParNo){
      case 0:                                                           // paramater "s", send message
        send = 1;
      break;
      
      case 1:                                                           // paramater "t", set path
        if (ParVal >= 0 && ParVal <= 0xffffffff){
          sent_byte = ParVal;
          Serial.print("sent byte  is set to "); Serial.println(sent_byte,HEX);
        }
        else{
          Serial.print("Wrong path value: "); Serial.println(ParVal);
        }
      break;
    }
  }
  else{
    Serial.println("Wrong parameter type");
  }
}
