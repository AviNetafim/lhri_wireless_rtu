
/* T = 1 / baud rate
* Timer  counter period is set to 1/2T 
* timer 1 pre scaler 8 - timer clock = 0.5us 
* half bit cnt    0   1   2   3   4   5   6   7   8   9   10  11  12  12  14  15  16  17  18  19 
                | start |   0   |   1   |   2   |   3   |   4   |   5   |   6   |   7   |  stop |
*/
#ifndef mancode_h
#define mancode_h

#define RECIO 4                                                         // GPIO for serial receiving 
#define UP_TRSIO 13                                                     // GPIO for serial transmission up output
#define LEFT_TRSIO 19                                                   // GPIO for serial transmission left output
#define RIGHT_TRSIO 21                                                  // GPIO for serial transmission right output
#define UP_ENABLE 14                                                    // 485 enable for up output
#define LEFT_ENABLE 25                                                  // 485 enable for left output
#define RIGHT_ENABLE 26                                                 // 485 enable for right output

#define UP 0                                                            // output ports indexes       
#define LEFT 1
#define RIGHT 2 

#define BYTE_LENGTH 20                                                  // total word length, including delay between words (half bit units)
#define MAX_MSG_LENGTH 32
#define TIMEOUT 10                                                      // in half bit units -> 5 bits marks the end of the message  
#define DEBUG False

#include "Arduino.h"

const uint8_t trsio_map[3] = {UP_TRSIO, LEFT_TRSIO, RIGHT_TRSIO};        // for port code (0-2) to transmiit GPIO translation
const uint8_t enable_map[3] = {UP_ENABLE, LEFT_ENABLE, RIGHT_ENABLE};    // for port code (0-2) to transmiit GPIO translation


const uint8_t rec_switch_array[BYTE_LENGTH] = {0,1,2,3,2,3,2,3,2,3,2,3,2,3,2,3,2,3,4,5};    // use in manchester decoding algorithm
const uint8_t trs_switch_array[BYTE_LENGTH] =      {0,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3};    // use in manchester coding 
//                                             0         5         10        15      

void IRAM_ATTR gpio4_isr();

class ManCode{
  public:
    ManCode();
    void Init(uint16_t arg_divider);
    void StartReceiveMessage();                             // respond to INT0 interrupt when meddage begins     
    void StartReceiveByte(uint16_t arg_half_bit);           // respond to INT0 interrupt when new byte begins    
    void RecMsg();                                          // read and decode message 
    bool TimeOut();                                         // check for message end    
    void MsgClear();                                     // reset rec pointer    
    uint8_t MsgLen();                                       // get received message length  
    void GetMsg(uint8_t *arg_msg);                          
    bool SendByte(uint8_t sent_byte,uint8_t port);
    void TxEnable(uint8_t port, uint8_t state);             // enable 485 output port
    uint64_t read_timer();
    void clear_timer();
    void enable_gpio4_int();
    void disable_gpio4_int();
    void clear_gpio4_int();
  private:
    hw_timer_t *_timer0 = nullptr;
    uint16_t _half_bit;                                     // half bit length  
    uint8_t _half_bit_cnt;                                  // half bits in processed byte counter
    uint8_t _start_bit_val;                                 // first half of received start bit value 
    uint8_t _data_bit;                                      // reeceived data bit  - for sumulation puposes only
    uint8_t _data_bit_val;                                  // first half received data bit value  
    uint8_t _stop_bit_val;                                  // first half received stop bit value    
    uint8_t _byte_err;                                      // byte decoding errors counter
    uint8_t _byte_val;                                      // decoded byte value
    uint8_t _rec_msg[MAX_MSG_LENGTH];                       // received message
    uint8_t _rec_ptr = 0;                                   // TBD 
    uint16_t _idle_cnt;                                     // half bit counts when no receiver is idle
    uint8_t _last_data_bit;                                 // for receiver idle sensing
    uint8_t _switch_var;                                    // manchester coding switch-case varibale 
    uint8_t _int_attached;
};

#endif
 
