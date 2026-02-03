/*
* integration of ap_server_4 and gpio_int_test
* client set send register, get acknowledge ant than response  with  interrupt counts 
* created 03/02/2026
*/


#include <Arduino.h>
#include <apServer.h>
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

#define RX_GPIO 4                                                         // mancode receive pin 
#define TX_GPIO 13                                                        // mancode transmit pin 
#define TCP_PORT 8080
#define RX_BUF_SIZE 256


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

const char* AP_SSID = "ESP32_AP";
const char* AP_PASS = "12345678";   // min 8 chars
const uint16_t half_bit = CPU_CLOCK/(DIVIDER)/2/BAUDRATE;              // assuming frequency divider of 40 - clock time is 0.5us
const uint16_t byte_delay = half_bit * 3;
apServer as(TCP_PORT);

// ------------------------------ program variables ----------------------------------------------

uint8_t trs_ptr = 0;
uint8_t state = WAIT_SEND;                                              // client state
uint8_t on_delay = 0;                                                   // on delay flag indicating  edge detected and on delay timer is active
uint8_t send = 0;                                                       // varaible set by serial monitor command to startsending a message
uint16_t nrtoc = 0;                                                     // no response timeout counter
uint16_t half_bit_cnt = 0;
volatile bool rec_flag = false;
uint8_t rec_cnt = 0;
uint8_t do_state = 0;

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
void timer_init();
static inline uint64_t get_timer_cnt();
static inline void reset_timer();
void IRAM_ATTR gpio4_isr();
void enable_gpio4_interrupt();
void disable_gpio4_interrupt();
void show_error_msg(String arg_text);


void setup() {
  Serial.begin(115200);
  Serial.println("ap_server_5");
  as.Init(AP_SSID,AP_PASS);                                           // Initialize serial communication
  Serial.println("TCP server ready");
  pinMode (RX_GPIO,OUTPUT);                                                // comapre version!!!
  digitalWrite(RX_GPIO,LOW);
  timer_init();
  pinMode(RX_GPIO, INPUT_PULLUP);
  enable_gpio4_interrupt();

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
        r_send[0] = 0;
        state = SEND;                                           // send message, to which GPIO output? TBD
        Serial.println("start transmission go to SEND");
        half_bit_cnt = 0;
        reset_timer();
      } 


      // ------------- server responded, check response ------------------------------------------

      if (half_bit_cnt > 32){
        as.send_reg(2,rec_cnt);                                  // send response state host with response value 0
        Serial.println("rtu received respsonse");
        half_bit_cnt = 0 ;
      }
    break;

    case SEND:
      if (get_timer_cnt() > half_bit){                           // wait for timer1 half bit count
        reset_timer();
        if (half_bit_cnt > 32){                            
          state = WAIT_RECEIVE;
        }    
          if (do_state == 0){
          digitalWrite(13,HIGH);
          do_state = 1;
         }
        else{
          digitalWrite(13,LOW);
          do_state = 0;        
        }
        half_bit_cnt += 1;
        }
    break;

    case WAIT_RECEIVE:                                     // wait for server response, stop at timeout
    if (rec_flag == true){
      rec_flag = 0;
      rec_cnt += 1; 
      Serial.println(rec_cnt);
    }
      state = RECEIVE;                                            // set receiver to receive mode
    break;

    case RECEIVE: // get here  by receiver byte start from WAIT_RECEIVE, untill message timeout (5 bits)
      state = WAIT_SEND;
    break;
  }
}

void timer_init(){
    // configure timer with 0.5us clock, up counting, freee running
    TIMERG0.hw_timer[0].config.enable = 0;                              // Stop timer before setting
    TIMERG0.hw_timer[0].config.divider = DIVIDER+1;                      // 80 MHz / 40 = 2 MHz (0.5 µs)
    TIMERG0.hw_timer[0].config.increase = 1;                            // Count up
    TIMERG0.hw_timer[0].config.autoreload = 0;                          // free running mode
    TIMERG0.hw_timer[0].config.alarm_en = 0;                            // no alarm value
    TIMERG0.int_ena.val = 0;                                            // Disable interrupts explicitly
    TIMERG0.hw_timer[0].load_low  = 0;                                  // Load counter = 0
    TIMERG0.hw_timer[0].load_high = 0;
    TIMERG0.hw_timer[0].reload    = 1;
    TIMERG0.hw_timer[0].config.enable = 1;                              // Start timer
}

static inline uint64_t get_timer_cnt(){
    // update counter value an read
    TIMERG0.hw_timer[0].update = 1;
    uint32_t lo = TIMERG0.hw_timer[0].cnt_low;
    uint32_t hi = TIMERG0.hw_timer[0].cnt_high;
    return ((uint64_t)hi << 32) | lo;
}

static inline void reset_timer(){
    // reset timer
    TIMERG0.hw_timer[0].load_low  = 0;
    TIMERG0.hw_timer[0].load_high = 0;
    TIMERG0.hw_timer[0].reload = 1;
}

void IRAM_ATTR gpio4_isr(){
  rec_flag = true;
}

void enable_gpio4_interrupt(){
  // Enable interrupt
  attachInterrupt(digitalPinToInterrupt(RX_GPIO), gpio4_isr,FALLING);
}

void disable_gpio4_interrupt(){
  //Disable interrupt */
  detachInterrupt(digitalPinToInterrupt(RX_GPIO));
}

void show_error_msg(String arg_text){
if (PRINT_ON == 1) Serial.println(arg_text);
}

void IRAM_ATTR gpio4_isr();
void enable_gpio4_interrupt();
void disable_gpio4_interrupt();
void show_error_msg(String arg_text);


