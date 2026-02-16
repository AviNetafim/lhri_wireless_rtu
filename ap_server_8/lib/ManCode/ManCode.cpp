
#include  "ManCode.h"

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
    // enable_gpio4_int();

}  

bool ManCode::SendByte(uint8_t sent_byte,uint8_t port){
    uint8_t bit_cnt;
    uint8_t sent_bit;
    uint8_t write_code;
    if (_half_bit_cnt < BYTE_LENGTH) _switch_var = trs_switch_array[_half_bit_cnt];
    else _switch_var = 4;                                                 // error state, do nothing.  
    switch (_switch_var){
        case 0:                                                                                                                 // start bit begins                                                   
            digitalWrite(13,LOW);                                
        break;

        case 2:                                                                                                                     //   data bits
            bit_cnt = (_half_bit_cnt-2) >> 1;                                 // divide half bit count by 2 to get bit number  
            sent_bit = (sent_byte & (1 << bit_cnt)) >> bit_cnt;               // mask the value of the bit number from sent byte and right shift by bit number
            write_code = (sent_bit <<1) + _half_bit_cnt %2;                         
            switch (write_code){                                              // manchester codin per iEEE.802.3    
                case 0:                                                         // sent bit value = 0 , second half  
                    digitalWrite(13,HIGH);
                break;
                case 1:                                                         // sent bit value = 0 , first half  
                    digitalWrite(13,LOW);
                break;
                case 2:                                                         // sent bit value = 1 , second half  
                    digitalWrite(13,LOW);
                break;
                case 3:                                                         // sent bit value = 1 , first half  
                    digitalWrite(13,HIGH);
                break;
            }
        break;
        
        case 3:                                                                                                                     // stop bit
            digitalWrite(13,HIGH);
        break;
    }
    log_data[log_cnt][0] = sent_byte;    
    log_data[log_cnt][1] = _half_bit_cnt;
    log_data[log_cnt][2] = bit_cnt;
    log_data[log_cnt][3] = sent_bit;
    log_data[log_cnt][4] = write_code;
    log_data[log_cnt][5] = _switch_var;
    _new_us = micros();
    log_data[log_cnt][6] = _new_us - _last_us;
    log_data[log_cnt][7] = timerRead(_timer0);
    _last_us = _new_us;

    log_cnt += 1;

    if (++_half_bit_cnt >= BYTE_LENGTH)  {                                // byte transmission ended     
        _half_bit_cnt=0;
        return true;
    }
    return false;
}

uint64_t ManCode::read_timer(){
    return timerRead(_timer0);
}

void ManCode::clear_timer(){
    timerWrite(_timer0,0);
}

void ManCode::clear_log(){
    int i,j;
    for ( i = 0 ; i < 24 ; i++){
        for (j = 0 ; j < 5; j++){
            log_data[i][j] = 0;
        }    
    }
    log_cnt = 0;
}


