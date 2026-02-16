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
#include "LhriProtocol.h"


#define WAIT_SEND 0                                                     // client states
#define SEND 1
#define WAIT_RECEIVE 2
#define RECEIVE 3


#define CPU_CLOCK 80000000                                              // esp32 has 80Mhz clock
#define BAUDRATE 1200                                                   // half bit time = 417us
#define DIVIDER 40 // divide clock by 40
#define NO_RESP_TIME 12000                                            // no response timeout for defined bauderate is 417us * 12000 = 5s
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

const String  ParameterList = "srldtcp";                                // serial monitor command code string
const uint16_t half_bit = CPU_CLOCK/(DIVIDER)/2/BAUDRATE;              // assuming frequency divider of 40 - clock time is 0.5us
const uint16_t byte_delay = half_bit * 3;
lhri_msg trs_msg;                                                       // transmit message fields structure
lhri_msg rec_msg;                                                       // receive message fields strucutre     
lhri_msg resp_msg;                                                      // server response message fileds structre
lhri_str rec_str;                                                       // receive string structre
lhri_str trs_str;                                                       // transmit string structre
LhriProtocol lp;                                                        // protocol class isntance 
ManCode mc;     

// ------------------------------ program variables ----------------------------------------------

String inString;                                                        // serial monitor input buffer 
volatile bool byte_start = false;
uint8_t trs_ptr = 0;
uint8_t state = WAIT_SEND;                                              // client state
uint8_t on_delay = 0;                                                   // on delay flag indicating  edge detected and on delay timer is active
uint8_t send = 0;                                                       // varaible set by serial monitor command to startsending a message
uint16_t nrtoc = 0;                                                     // no response timeout counter
uint8_t send_port = UP;                                                 // node transmission GPIO for the allocated port
unsigned long start_ms;
unsigned long int_ms;

//----------------------------------- functions declarrations ----------------------------------------

void IRAM_ATTR gpio4_isr();
void parse_parameter();
void show_error_msg(String arg_text);
void show_lhri_str(String title, lhri_str arg_str);
void show_lhri_msg(String arg_title,lhri_msg arg_msg);



void setup() {
  Serial.begin(115200);
  Serial.println("ap_server_7");
  Serial.println("TCP server ready");
  mc.Init(DIVIDER);
  lp.Init();
}

void loop(){
  switch(state){
    case WAIT_SEND:
        while(Serial.available()>0){  
          inString = Serial.readStringUntil('\n');  
          parse_parameter();
        }

      // ------------------------------send command received from host-----------------------------

      if (send == 1){
        trs_str = lp.SendMsg(trs_msg);                                  // convert command to a string 
        show_lhri_str("client trs str",trs_str);
        send = 0;
        state = SEND;                                                   // send message, to which GPIO output? TBD
        Serial.println("start transmission go to SEND");
        mc.TxEnable(UP,HIGH);                                           // enable up port for message transmission        
        trs_ptr = 0;         
        mc.reset_timer();
      } 
      else{        
        // ------------- check if server responded  -----------------------------------------------
        rec_str.len = mc.MsgLen();                                      // check received message length
        if (rec_str.len > 0){
          mc.GetMsg(rec_str.bytes);                                     // get received message copy 
          show_lhri_str("received str", rec_str);
          rec_msg = lp.RecMsg(rec_str);                                 // decode message bytes string to message fields 
          if (rec_msg.err == 0){                                      // no crc error
            show_lhri_msg("received msg",rec_msg);
            Serial.println("rtu received response");
          }  
          else{
            show_error_msg("error while receiving server response");
            Serial.println(rec_msg.err);
          }
          mc.MsgClear();                                                // clear -> set len to zero
        }  
      }
      break;

    case SEND:
      if (mc.get_timer_cnt() > half_bit){                           // wait for timer1 half bit count
        mc.reset_timer();
        if (trs_ptr < trs_str.len){                                     // more bytes to send 
          if (mc.SendByte(trs_str.bytes[trs_ptr],UP)){                  // byte end when true
            trs_ptr += 1;
            while(mc.get_timer_cnt() < byte_delay);                                  // insert a delay between bytes
            mc.reset_timer();
          }    
        }
        else{                                                           // all bytes sent    
          mc.TxEnable(UP,LOW);                                          // disable 485 transmitter                                        // disable 485 transmitter
          state = WAIT_RECEIVE;                                         // enable receiver for server respond 
          Serial.println("transmission ended - go to WAIT_RECEIVE");
          trs_ptr = 0;
          nrtoc = 0;                                                    // reset no respone timeout counter      
          // mc.clear_gpio4_int();                                                                                     
          byte_start = 0;
          // mc.enable_gpio4_int();
        }
      }
    break;

    case WAIT_RECEIVE:                                                  // wait for server response, stop at timeout
      if (mc.get_timer_cnt() > half_bit){                               // count half bits 
        mc.reset_timer();
        nrtoc += 1;
        if (nrtoc > NO_RESP_TIME){                                      // no response received, stop waiting for the message
          state = WAIT_SEND;                                            // return to idle 
          Serial.println("timeout - go to WAIT_SEND");
          send = 0;
          nrtoc = 0;
          // mc.disable_gpio4_int();                                        // disable INT0 when waiting for next client command             
        }
      }
      if (byte_start == 1){                                             // INT0 - response received  
        state= RECEIVE;                                                 // set receiver to receive mode
        mc.StartReceiveMessage();                                       // reset pointers, counters
      }          
    break;

    case RECEIVE: // get here  by receiver byte start from WAIT_RECEIVE, untill message timeout (5 bits)
      if (byte_start == 1){                                             // new byte message arrived INT0
        nrtoc = 0;                                                      // clear no resonse timeout counterl  
        byte_start = 0;
        mc.StartReceiveByte(half_bit);                                  // wait for qtr bit here and sample first half odf start bit, diable INT0 
      } 
      if (mc.get_timer_cnt() > half_bit){                                            // get here, every half bit when receive == 1, timer is reset whenever a byte begins
        mc.reset_timer(); 
        mc.RecMsg();                                                    // enable INT0 at the end of each byte
        if(mc.TimeOut()){
          state = WAIT_SEND;
          Serial.println("msg received- go to WAIT_SEND");
          send = 0;         
        }
      }    
    break;
  }
}

void IRAM_ATTR gpio4_isr(){
  byte_start = true;
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
      
      case 1:                                                           // paramater "r", set dir 
        if (ParVal >= 0x0 && ParVal <= 0x02){
          trs_msg.dir = ParVal;
          Serial.print("dir is set to "); Serial.println(trs_msg.dir,HEX);
        }
        else{
          Serial.println("dir value not in range (0-2) "); Serial.println(ParVal);
        }
        break;

      case 2:                                                           // paramater "l", set  level 
        if (ParVal >= 0x0 && ParVal <= 0x20){
          trs_msg.level = ParVal;
          Serial.print("level is set to "); Serial.println(trs_msg.level,HEX);
        }
        else{
          Serial.println("level value not in range (0 - 0x20): "); Serial.println(ParVal);
        }
        break;

      case 3:                                                           // paramater "d", set dest_level
        if (ParVal >= 0x0 && ParVal <= 0x20){
          trs_msg.dlevel = ParVal;
          Serial.print("dest level is set to "); Serial.println(trs_msg.dlevel,HEX);
        }
        else{
          Serial.print("dest level not in range (0 - 0x20): "); Serial.println(ParVal);
        }
        break;

      case 4:                                                           // paramater "t", set path
        if (ParVal >= 0 && ParVal <= 0xffffffff){
          trs_msg.path = ParVal;
          Serial.print("path is set to "); Serial.println(trs_msg.path,HEX);
        }
        else{
          Serial.print("Wrong path value: "); Serial.println(ParVal);
        }
        break;

      case 5:                                                           // paramater "c", set command
        if (ParVal >= 0 && ParVal <= 0x30){
          trs_msg.cmd = ParVal;
          Serial.print(" command is set to "); Serial.println(trs_msg.cmd,HEX);
        }
        else{
          Serial.print("Wrong command value: "); Serial.println(ParVal);
          }
        break;

      case 6:                                                           // paramater "p", set payload
        trs_msg.pload = ParVal;
        Serial.print("pload is set to "); Serial.println(trs_msg.pload,HEX);
        break;

      default:
        Serial.println("Wrong parameter type");
        break;
    }
  }
  else{
    Serial.println("Wrong parameter type");
  }
  show_lhri_msg("client msg",trs_msg);
}


void show_error_msg(String arg_text){
if (PRINT_ON == 1) Serial.println(arg_text);
}


void show_lhri_str(String title, lhri_str arg_str){
  // display transmit message byte stream
    if (PRINT_ON == 1){
    Serial.print(title); Serial.print(": ");
    Serial.print("len="); Serial.print(arg_str.len,HEX);Serial.print(" ");
    for (int i = 0 ; i < arg_str.len ; i++){
      Serial.print(arg_str.bytes[i],HEX); Serial.print(",");
    }
    Serial.println();
  }  
}


void show_lhri_msg(String arg_title,lhri_msg arg_msg){
  // display transmit message fields strucutre
  if (PRINT_ON == 1){  
    Serial.print(arg_title); Serial.print(": ");
    Serial.print("dir="); Serial.print(arg_msg.dir,HEX);
    Serial.print(", level="); Serial.print(arg_msg.level,HEX);
    Serial.print(", dlevel=") ;Serial.print(arg_msg.dlevel,HEX);
    Serial.print(", path="); Serial.print(arg_msg.path,HEX);
    Serial.print(", cmd=");Serial.print(arg_msg.cmd,HEX);
    Serial.print(", pload=");Serial.print(arg_msg.pload,HEX);
    Serial.print(", error=");Serial.println(arg_msg.err,HEX);
  }  
}

  