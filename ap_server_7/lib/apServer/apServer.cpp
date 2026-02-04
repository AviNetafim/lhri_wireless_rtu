#include <Arduino.h>
#include "apServer.h"
#include <FastCRC.h>
#include <FastCRC_tables.h>

FastCRC16 CRC16;


apServer::apServer(uint16_t port) : _server(port){
}

void apServer::Init(const char* ssid,const char* password){
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  _server.begin();                                                       // Start the server
  _server.setNoDelay(true);
}


void apServer::client_connect(){
    if (_connected){
        if (!_client.connected() || !_client){
            _client.stop();                                             // stop client
            Serial.print("Client disconnected after ");
            Serial.print(millis()-_start_ms);
             Serial.println(" ms");
            _connected = false;
        }
    }
    else{
        _client = _server.available();                                  // check if new client is trying to connect
        if (_client) {
            Serial.println("Client connected");
            _start_ms = millis();
            _connected = true;
        }
    }
}
bool apServer::cmd_available(registers (&aregs)[REG_MAP_SIZE]){
  _rxLen = 0;
  if (_client.available()) {                                // Check for incoming data
    while(_client.available() > 0) {
      if (_rxLen < MAX_BUFFS_SIZE) {
        _rxBuf[_rxLen++] = _client.read();
      } 
      else {
        _client.read();                                             // discard overflow
      }
    }  
      if (_rxLen > 0) {                                                        
        Serial.print("Received ");
        Serial.print(_rxLen);
        Serial.println(" bytes:");
        for (size_t i = 0; i < _rxLen; i++) {
            Serial.printf("%02X ", _rxBuf[i]);
        }
        Serial.println();
        parse_cmd(aregs);
        return true;
      }
    }
    else{
        return false;
  }
  return false;
}    

void apServer::send_reg(uint8_t reg_no,uint8_t reg_val){
    _txBuf[RTU] = 1;
    _txBuf[FUN] = 4;
    _txBuf[ADD] = reg_no;
    _txBuf[IXL] = 0;
    _txBuf[IXH] = 0;
    _txBuf[SZL] = 1;
    _txBuf[SZH] = 0;
    _txBuf[CODE] = 0;
    _txBuf[8] = reg_val & 0X00FF;
    _txBuf[9] = (reg_val & 0xFF00) >> 8;
    _txLen = resp_crc("respond update: ",10);
  reply();
}

void apServer::reply(){
    _client.write(_txBuf, _txLen);
}


void apServer::parse_cmd(registers (&aregs)[REG_MAP_SIZE]){
  int payload_size;                                                     // command register payload size
  int index;                                                            // command register index        
  uint16_t CRC1 = 0;                                                    // for receive message crc calcualtion
  int i;
  _txLen=0;
  prt_msg("cmd: ",_rxBuf,_rxLen);            
  if (_rxBuf[RTU] != myRTU){                                              // igonre command for other RTUs, no response
    Serial.println("cmd is not for me");
    return;
  }
  
  if (_rxLen < 9){                                                       // ignore too short commands - no resposne
    prt_msg("incomplete command",_rxBuf,_rxLen);
    return;
  }
  CRC1 = CRC16.ccitt(_rxBuf,_rxLen-2);
  if ( lowByte(CRC1) != _rxBuf[_rxLen-2] || highByte(CRC1) != _rxBuf[_rxLen-1]){
    prt_msg("CRC error",_rxBuf,_rxLen);                                    // CRC of received message is correct - no repsonse
    return;
  }
  for (i=0; i < 7 ; i++) _txBuf[i] = _rxBuf[i];                             // copy command header to resp header
  if (_rxBuf[ADD] >= REG_MAP_SIZE){                                       // register address is not in RTU space send response with error cpde =1
    _txBuf[CODE] = 0x01;
    _txLen  = resp_crc("wrong register address: ",8);
    return;      
  }
  payload_size = _rxBuf[SZL] + _rxBuf[SZH] * 256;
  index =  _rxBuf[IXL] + _rxBuf[IXH] * 256;
  if (index + payload_size > aregs[_rxBuf[ADD]].dim){                     // command range exceeds register array size
    _txBuf[CODE] = 0x02;
    _txLen  = resp_crc("range too high: ",8);
    return;
  }

  if (payload_size > (MAX_BUFFS_SIZE - 10)/2){                          // command payload is too big for buffers size
    _txBuf[CODE] = 0x03;
    _txLen  = resp_crc("payload to big: ",8);
    return;
  }

  switch(_rxBuf[FUN]){                                                    // process  commands
    case 0x06:                                                          // write command, copy data command from payload to reg specified range
      if (_rxLen < 9 + payload_size*2){                                  // command is too short for palyload size  
        _txBuf[CODE] = 0x04;
        _txLen  = resp_crc("short payload: ",8);
        return;
      }  
      for ( i=0; i<payload_size; i++){                                  // copy payload to register range    
        *(aregs[_rxBuf[ADD]].point+index+i) = _rxBuf[7+2*i] + _rxBuf[8+2*i]*256;  // Little-endian    
      }
      _txBuf[CODE]=0;
      _txLen = resp_crc("write response: ",8);
      return;
      break;

    case 0x04:                                                           // read command, copy data reg to resp payload
      for (i=0; i < payload_size ; i++){                                 // read register range to repsonse message
        _txBuf[8+i*2]=lowByte(*(aregs[_rxBuf[ADD]].point+index+i));
        _txBuf[9+i*2]=highByte(*(aregs[_rxBuf[ADD]].point+index+i));
      }
      _txBuf[CODE] = 0;
      _txLen  = resp_crc("read response: ",8 + payload_size*2);
      break;
     
    default:
      _txBuf[CODE] = 0x05;
      _txLen = resp_crc("wrong command: ",8);
      return;
      break;
  }                                                      
}

int apServer::resp_crc(String when,int arg_resp_size){
  uint16_t CRC2;
  CRC2 = CRC16.ccitt(_txBuf,arg_resp_size);                            
  _txBuf[arg_resp_size]  = lowByte(CRC2);
  _txBuf[arg_resp_size+1] = highByte(CRC2);
  prt_msg(when,_txBuf,arg_resp_size+2);
  return (arg_resp_size+2);
}

void apServer::prt_msg(String when, uint8_t arg_msg[], int arg_msg_size){
  int i;
  Serial.print(when);
  for ( i = 0 ; i < arg_msg_size-1 ; i++){
    Serial.print(arg_msg[i],HEX);
    Serial.print(",");
  }
  Serial.println(arg_msg[i],HEX);    
}

