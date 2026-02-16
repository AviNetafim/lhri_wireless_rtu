
#include  "ManCode.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
// #include "soc/gpio_struct.h"

ManCode::ManCode(){
}

void ManCode::Init(uint16_t arg_divider){
    pinMode(UP_TRSIO,OUTPUT);
    digitalWrite(UP_TRSIO,HIGH);                                          // intial serial output state is low 
    pinMode(LEFT_TRSIO,OUTPUT);
    digitalWrite(LEFT_TRSIO,HIGH);
    pinMode(RIGHT_TRSIO,OUTPUT);
    digitalWrite(RIGHT_TRSIO,HIGH);                                     
    pinMode(UP_ENABLE,OUTPUT);
    digitalWrite(UP_ENABLE,LOW);                                          // intial serial output state is low 
    pinMode(LEFT_ENABLE,OUTPUT);
    digitalWrite(LEFT_ENABLE,LOW);  
    pinMode(RIGHT_ENABLE,OUTPUT);
    digitalWrite(RIGHT_ENABLE,LOW);
    pinMode(RECIO,INPUT);
    _half_bit_cnt = 0;   
    // configure timer with 0.5us clock, up counting, freee running
    _timer0 = timerBegin(0,40,true);
    timerWrite(_timer0,0);
    enable_gpio4_int();

}  

void ManCode::StartReceiveMessage(){
    _idle_cnt = 0;
    _rec_ptr = 0;                                                                                       
 }

void ManCode::StartReceiveByte(uint16_t arg_half_bit){
    uint16_t qbit = 0;
    clear_timer();
    qbit = arg_half_bit >> 1;
    while (read_timer() < qbit);                                      // wait here for a quarter bit to elapse  
    clear_timer() ;                                                      // reset timer counter  for next sampling
    _start_bit_val = digitalRead(RECIO);                                 // read first half of start bit value 
    _half_bit_cnt = 1;                                                   // set half bit counter ready for next half  
    _byte_err = 0;
    disable_gpio4_int();                                                 // disable further INT0  until byte end
}

void ManCode::RecMsg(){
    uint8_t bit_val=0;
    uint8_t bit_cnt;
    uint8_t switch_var = 0;
    uint8_t read_code;
    _data_bit = digitalRead(RECIO);                                       // read input state
    if (_half_bit_cnt < BYTE_LENGTH){                                     // 
        switch_var = rec_switch_array[_half_bit_cnt];                       // get switch var value from switch array
        switch (switch_var){
            case 1:                                                           // second sample of start bit
                if(_start_bit_val + _data_bit > 0){                             // both samples of start bit must be low
                    _byte_err = 1;
                }
            break;
            case 2:                                                           // first sample of data bits 
                _data_bit_val = _data_bit;
            break;
            case 3:                                                           // second sample of data bits 
                read_code = (_data_bit << 1) + _data_bit_val;
                switch (read_code){
                    case 0:                                                       // both half bits are 0 -> error
                        _byte_err |= 2;
                    break;
                    case 1:                                                       // 2nd half = 0, 1st half = 1 high to low  transition = 0
                        bit_val = 0;                                   
                    break;
                    case 2:                                                       // 2nd half = 1, 1st half = 0 low to high transition = 1
                        bit_val = 1; 
                    break;
                    case 3:
                        _byte_err |= 4;                                        // both half bits are 1 -> error
                    break;
                }
                bit_cnt = (_half_bit_cnt-2) >> 1;                               // get bit location in byte from from half bit counter
                if(bit_val == 0 ) _byte_val &= ~(1<< bit_cnt);                  // write bit value in the byte
                else _byte_val |= (1<< bit_cnt);
            break;

            case 4:                                                           // first half of stop bit
                _stop_bit_val = _data_bit;

            break;
        
            case 5:                                                           // second half of stop bit (_half_bit_cnt =19)
                if(_stop_bit_val + _data_bit < 2){                              // both samples of stop bit must be high
                    _byte_err |= 8;
                }
                if (_byte_err == 0){                                            // byte received without errors
                    if (_rec_ptr > 0 && _rec_ptr < MAX_MSG_LENGTH){               // ignore preamble
                        _rec_msg[_rec_ptr-1] = _byte_val;                           // add byte to received message up to maximum length 
                    }
                    _byte_val = 0;
                    _rec_ptr += 1;        
                }
                // clear_gpio4_int();                                           // clear INT0 pending interrupts   
                enable_gpio4_int();                                           // enable INT0 to receive next byte
            break;
        }
        _half_bit_cnt += 1;
    }  
}

bool ManCode::TimeOut(){
    if (_data_bit ^ _last_data_bit == 0) _idle_cnt += 1;
    else _idle_cnt = 0; 
    _last_data_bit = _data_bit;     
    if (_idle_cnt > TIMEOUT){
        _half_bit_cnt = 0;
        if (_byte_err > 0){
            _rec_ptr = 0;                                                   // reset message length for error  
            Serial.print("err="); 
            Serial.println(_byte_err);
        } 
        disable_gpio4_int();                                                // disable further INT0 until respond is sent 
        return true;
    }
    return false;
}

void ManCode::MsgClear(){
    _rec_ptr = 0;
}

uint8_t ManCode::MsgLen(){
    if (_rec_ptr > 0) return  _rec_ptr-1;                                 // ptr at messsage end icnludes preamble
    else return 0;
}

void ManCode::GetMsg(uint8_t *arg_msg){
    for(int i = 0 ; i < _rec_ptr ; i++){
        arg_msg[i] =_rec_msg[i];
    }
}

bool ManCode::SendByte(uint8_t sent_byte,uint8_t port){
    uint8_t bit_cnt;
    uint8_t sent_bit;
    uint8_t write_code;
    if (_half_bit_cnt < BYTE_LENGTH) _switch_var = trs_switch_array[_half_bit_cnt];
    else _switch_var = 4;                                                 // error state, do nothing.  
    switch (_switch_var){
        case 0:                                                                                                                 // start bit begins                                                   
            digitalWrite(trsio_map[port],LOW);                                
        break;

        case 2:                                                                                                                     //   data bits
            bit_cnt = (_half_bit_cnt-2) >> 1;                                 // divide half bit count by 2 to get bit number  
            sent_bit = (sent_byte & (1 << bit_cnt)) >> bit_cnt;               // mask the value of the bit number from sent byte and right shift by bit number
            write_code = (sent_bit <<1) + _half_bit_cnt %2;                         
            switch (write_code){                                              // manchester codin per iEEE.802.3    
                case 0:                                                         // sent bit value = 0 , second half  
                    digitalWrite(trsio_map[port],HIGH);
                break;
                case 1:                                                         // sent bit value = 0 , first half  
                    digitalWrite(trsio_map[port],LOW);
                break;
                case 2:                                                         // sent bit value = 1 , second half  
                    digitalWrite(trsio_map[port],LOW);
                break;
                case 3:                                                         // sent bit value = 1 , first half  
                    digitalWrite(trsio_map[port],HIGH);
                break;
            }
        break;
        
        case 3:                                                                                                                     // stop bit
            digitalWrite(trsio_map[port],HIGH);
        break;
    }

    if (++_half_bit_cnt >= BYTE_LENGTH)  {                                // byte transmission ended     
        _half_bit_cnt=0;
        return true;
    }
    return false;
}

void ManCode::TxEnable(uint8_t port,uint8_t state){                     // enable 485 tranciever of seleted port  
    digitalWrite(enable_map[port],state);
}

uint64_t ManCode::read_timer(){
    return timerRead(_timer0);
}

void ManCode::clear_timer(){
    timerWrite(_timer0,0);  
}

void ManCode::enable_gpio4_int(){
  // Enable interrupt
  if (_int_attached == 0){
    attachInterrupt(digitalPinToInterrupt(RECIO), gpio4_isr,FALLING);
    _int_attached  = 1;
  }
}

void ManCode::disable_gpio4_int(){
  //Disable interrupt */
  if (_int_attached == 1){
    detachInterrupt(digitalPinToInterrupt(RECIO));
    _int_attached  = 0;
  }
}

// void ManCode::clear_gpio4_int(){
//     GPIO.status_w1tc = (1 << 4);                                        // Clear pending interrupt for GPIO4
// }
