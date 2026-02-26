
/*
* full lhri brdige  tcp NetRtu implementation
* created 17/02/2026
*/


#include <Arduino.h>
#include <apServer.h>
#include "ManCode.h"
#include "LhriProtocol.h"


#define TCP_PORT 8080

#define WAIT_SEND 0                                                     // client states
#define SEND 1
#define WAIT_RECEIVE 2
#define RECEIVE 3


#define CPU_CLOCK 80000000                                              // esp32 has 80Mhz clock
#define BAUDRATE 1200                                                   // half bit time = 417us
#define DIVIDER 40 // divide clock by 40
#define NO_RESP_TIME 10000                                              // no response timeout for defined bauderate is 417us * 8000 = 3.5s
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

const char* AP_SSID = "ESP32_AP";
const char* AP_PASS = "12345678";   // min 8 chars
const uint16_t half_bit = CPU_CLOCK/(DIVIDER)/2/BAUDRATE;              // assuming frequency divider of 40 - clock time is 0.5us
const uint16_t byte_delay = half_bit * 3;
lhri_msg trs_msg;                                                       // transmit message fields structure
lhri_msg rec_msg;                                                       // receive message fields strucutre    
lhri_msg resp_msg;                                                      // server response message fileds structre
lhri_str rec_str;                                                       // receive string structre
lhri_str trs_str;                                                       // transmit string structre
LhriProtocol lp;                                                        // protocol class isntance
ManCode mc;  
apServer as(TCP_PORT);  

// ------------------------------ program variables ----------------------------------------------

volatile bool byte_start = false;
uint8_t trs_ptr = 0;
uint8_t state = WAIT_SEND;                                              // client state
uint8_t on_delay = 0;                                                   // on delay flag indicating  edge detected and on delay timer is active
uint8_t send = 0;                                                       // varaible set by serial monitor command to startsending a message
uint16_t nrtoc = 0;                                                     // no response timeout counter

//-----------------------------------  RTU registers ---------------------------------------------------

uint16_t r_send[1] = {0x0};                                   // send command to lhri tree
uint16_t r_trs[7] = {0x0,0x05,0x05,0x1,0x0c,0x05,0x0};                  // transmit msg fields
uint16_t r_rec[9] = {0x0,0x05,0x05,0x1,0x0c,0x05,0x0,0x0,0x0};          // receive msg fields

registers regs[] = {                                                    // register list metadata
{1,0,0,&r_send[0]},
{7,0,0,&r_trs[0]},
{9,0,0,&r_rec[0]}
};

//----------------------------------- functions declarrations ----------------------------------------

void IRAM_ATTR gpio4_isr();
void show_error_msg(String arg_text);
void show_lhri_str(String title, lhri_str arg_str);
void show_lhri_msg(String arg_title,lhri_msg arg_msg);
void show_regs(String arg_header);



void setup() {
  Serial.begin(115200);
  Serial.println("ap_server_6");
  as.Init(AP_SSID,AP_PASS);                                           // Initialize serial communication
  Serial.println("TCP server ready");
  mc.Init(DIVIDER);
  lp.Init();
}

void loop(){
  switch(state){
    case WAIT_SEND:
      as.client_connect();
      if(as.cmd_available(regs)){
        as.reply();
      }

      // ------------------------------send command received from host-----------------------------

      if (r_send[0] == 1){
trs_msg.dir = r_trs[DIR]; // copy message parameters to transmit structure
trs_msg.level = r_trs[LEVEL];
trs_msg.dlevel = r_trs[DLEVEL];
trs_msg.path = r_trs[PATH0] + (r_trs[PATH1] >> 16);
trs_msg.cmd = r_trs[CMD];
trs_msg.pload = r_trs[PLOAD];
show_lhri_msg("registers to msg",trs_msg);
        trs_str = lp.SendMsg(trs_msg);                                  // convert command to a string
        show_lhri_str("client trs str",trs_str);
        r_send[0] = 0;
        state = SEND;                                                   // send message, to which GPIO output? TBD
        Serial.println("start transmission go to SEND");
        mc.TxEnable(UP,HIGH);                                           // enable up port for message transmission        
        trs_ptr = 0;        
        mc.clear_timer();
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

// message  will return from root node with rec_msg.level = trs_msg.level + 1
r_rec[DIR]= rec_msg.dir;
r_rec[LEVEL] = rec_msg.level;
r_rec[DLEVEL] = rec_msg.dlevel;
r_rec[PATH0] = rec_msg.path & 0xffff;
r_rec[PATH1] = (rec_msg.path & 0xffff0000) >> 16;
r_rec[CMD] = rec_msg.cmd;
r_rec[PLOAD] = rec_msg.pload;
r_rec[ERR] = rec_msg.err;
r_rec[PORT] = rec_msg.port;
as.send_reg(2,1);                                   // send response state host with response value 0
Serial.println("rtu received respsonse");
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
      if (mc.read_timer() > half_bit){                           // wait for timer1 half bit count
        mc.clear_timer();
        if (trs_ptr < trs_str.len){                                     // more bytes to send
          if (mc.SendByte(trs_str.bytes[trs_ptr],UP)){                  // byte end when true
            trs_ptr += 1;
            while(mc.read_timer() < byte_delay);                                  // insert a delay between bytes
            mc.clear_timer();
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
          mc.enable_gpio4_int();
        }
      }
    break;

    case WAIT_RECEIVE:                                                  // wait for server response, stop at timeout
      if (mc.read_timer() > half_bit){                               // count half bits
        mc.clear_timer();
        nrtoc += 1;
        if (nrtoc > NO_RESP_TIME){                                      // no response received, stop waiting for the message
          state = WAIT_SEND;                                            // return to idle
          Serial.println("timeout - go to WAIT_SEND");
          as.send_reg(2,0);                                   // send response state host with response value 0          
          send = 0;
          nrtoc = 0;
          mc.disable_gpio4_int();                                        // disable INT0 when waiting for next client command            
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
      if (mc.read_timer() > half_bit){                                            // get here, every half bit when receive == 1, timer is reset whenever a byte begins
        mc.clear_timer();
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

void show_regs(String arg_header){
//  show register map at setup
  String prt_str="";
  int r;
  int e;
  Serial.print(arg_header); Serial.print(": ");
  for (r=0 ; r < REG_MAP_SIZE ; r++){
prt_str = "reg[" + String(r)+"]=";
Serial.print(prt_str);
Serial.print(*(regs[r].point),HEX);
Serial.print(",");
  }
  Serial.println();
}

