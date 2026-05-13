#include  "LhriProtocol.h"

LhriProtocol::LhriProtocol(){
}

void LhriProtocol::Init(){                                              // init fixed field length
  _flds[dir].len = 2;
  _flds[level].len = 5;
  _flds[dlevel].len = 5;
  _flds[path].len = 0;
  _flds[cmd].len = 6;
  _flds[pload].len = 0;
}

lhri_str LhriProtocol::SendMsg (lhri_msg arg_msg){                      // convert tramsmitted message fields into bytes string
  int i;
  _pos = 0;
  flds_2_vals(arg_msg);                                                 // update fields value from message srtucture
  _flds[path].len = _flds[dlevel].val;                                  // update path length from dlevel value
  update_pload_len();                                                   // update length of vairable length fields - path and payload 
  for (i = 0 ; i < 5 ; i++){                                            // revert dir,level,dlevel,path and cmd fields into message bit stream 
    _flds[i].pos = _pos;                                                // update field position in bit stream  
    trs_rev_bits(i);
    _pos += _flds[i].len;    
  }

  int pad_size = 8 - _pos % 8;                                          // add padding zeros to byte end 
  for ( i= _pos; i < _pos + pad_size ; i++){
    _msg_bits[i] = 0;
  }
  _pos += pad_size;
  _flds[pload].pos = _pos;

  if (_flds[pload].len > 0) trs_rev_bits(pload);                        // revert pload 
  _pos += _flds[pload].len;
  Serial.print("_pos="); Serial.print(_pos);
  uint8_t trs_bytes_len = _pos  >> 3 ;                                  // divide bit length by 8 to get bytes count 
  Serial.print(", len="); Serial.println(trs_bytes_len);
  for (i = 0 ; i < trs_bytes_len ; i++){                                // convert message bit stream to byte string 
    _msg.bytes[i] = 0;
    for (int j = 0  ; j <8 ; j++){
      _msg.bytes[i] += _msg_bits[j+8*i] << (7-j);
    }
  }
  
  uint8_t crc = crc8_ccitt(_msg.bytes, trs_bytes_len);                  // calculate bytes string crc
  for (i = trs_bytes_len -1 ; i >=0   ; i--){                           // make room to preamble byte
    _msg.bytes[i+1] = _msg.bytes[i];   
  } 

  _msg.bytes[0] = 0xff;                                                 // add preable byte to byte string 
  _msg.bytes[trs_bytes_len+1] = crc;                                    // terminate string with crc byte
  _msg.len = trs_bytes_len+2;                                           // length now includes preamble and crc
  return _msg;
}

lhri_msg LhriProtocol::RecMsg(lhri_str arg_rec_str){                    // convert bytes stream of received message to message field structre     

  int i,j;
  lhri_msg rec_msg;
  rec_msg.err = 0;
  uint8_t crc = crc8_ccitt(arg_rec_str.bytes, arg_rec_str.len-1);
  if (arg_rec_str.bytes[arg_rec_str.len-1] != crc){
    rec_msg.err = 1;                                                    // error code 1 - crc error
    return rec_msg;
  }

  for (i = 0 ; i < arg_rec_str.len-1 ; i++){                              // convert bytes string to bit stream 
    for(j = 0 ; j < 8 ; j++){
      _msg_bits[i*8+j] = (arg_rec_str.bytes[i] >> (7-j)) & 0x01;
    }
  }

  _pos = 0;
  bits_2_fld(dir);                                                      // revert and convert bit stream to fields
  bits_2_fld(level);
  bits_2_fld(dlevel);
  _flds[path].len = _flds[dlevel].val;
  bits_2_fld(path);
  bits_2_fld(cmd);
  update_pload_len();
  _pos += 8 - _pos % 8;
  bits_2_fld(pload);
  return vals_2_flds();
}


uint8_t LhriProtocol::crc8_ccitt(const uint8_t *data, uint16_t arg_length) {
  uint8_t crc = 0x00;                                                   // Initial value
  uint8_t polynomial = 0x07;                                            // CRC-8-CCITT polynomial
  for (uint16_t i = 0; i < arg_length; i++) {
      crc ^= data[i];
      for (uint8_t j = 0; j < 8; j++) {
          if (crc & 0x80)
              crc = (crc << 1) ^ polynomial;
          else
              crc <<= 1;
      }
  }
  return crc;
}


void LhriProtocol::flds_2_vals(lhri_msg arg_msg){                       // update fields metadata value from message fields atructre
  _flds[dir].val= arg_msg.dir;
  _flds[level].val= arg_msg.level;
  _flds[dlevel].val= arg_msg.dlevel;
  _flds[path].val= arg_msg.path;
  _flds[cmd].val= arg_msg.cmd;
  _flds[pload].val= arg_msg.pload;
}


lhri_msg LhriProtocol::vals_2_flds(){                                   // update message fields atructre from fields metadata values
  lhri_msg tmsg;
  tmsg.err = 0;
  tmsg.dir = _flds[dir].val;
  tmsg.level = _flds[level].val;
  tmsg.dlevel = _flds[dlevel].val;
  tmsg.path =_flds[path].val;
  tmsg.cmd = _flds[cmd].val;
  tmsg.pload= _flds[pload].val;
  return tmsg;
}


void LhriProtocol::update_pload_len(){                                  // update fields metadata length - path and payload 
  uint8_t cmd_type = (_flds[cmd].val & 0x38) >>3;                       // command type  = 3 msb's
  if (_flds[dir].val == 0){                                             // downstream command 
    if (cmd_type == 2 || cmd_type == 4 || cmd_type == 6){               // write sensor data, serial characters or config parameter, payload size is 16 bit   
      _flds[pload].len = 16;
    }
    else{                                                               // no payload to all other downstream command 
      _flds[pload].len = 0;     
    }
  }  
  else{                                                                 // upstream response 
    if (cmd_type == 1 || cmd_type == 3 || cmd_type == 5){               // read sensor data, serial characters or config parameter, payload size is 16 bit  
      _flds[pload].len = 16;
    }
    else{
      _flds[pload].len = 0;        
    } 
  }
}


void LhriProtocol::trs_rev_bits(uint8_t arg_field){                     // revert endian of field value 
  uint32_t std_val = _flds[arg_field].val;
  for ( int i =_flds[arg_field].pos ;  i < _flds[arg_field].pos +_flds[arg_field].len ; i++){
    _msg_bits[i] = std_val & 1;
    std_val >>= 1;
  }
}


void LhriProtocol::bits_2_fld (uint8_t arg_fld){                        // revert field bits into field value 
  if (_flds[arg_fld].len > 0){
    uint32_t std_val = 0;
    for (int i = 0 ; i < _flds[arg_fld].len ; i++){
      std_val += _msg_bits[_pos + i] << i;
    }
    _flds[arg_fld].val = std_val;
    _flds[arg_fld].pos = _pos;
    _pos += _flds[arg_fld].len;
  }
}

// serial printinf function dor debugging

void LhriProtocol::prt_flds(String title){
  //display fields metadata structres
  Serial.println(title);
  for (int i = 0 ; i < 6 ; i++){
    Serial.print(flds_lbl[i]);Serial.print(" : ");
    Serial.print("val=");Serial.print(_flds[i].val,HEX); Serial.print(", ");
    Serial.print("pos=");Serial.print(_flds[i].pos,HEX); Serial.print(", ");
    Serial.print("len=");Serial.print(_flds[i].len,HEX); Serial.println();
  }
  Serial.println();
}

void LhriProtocol::prt_bits(String title){
  //  display message bit stream
  Serial.print(title);Serial.print(": ");
  for (int i = 0 ; i < 72 ; i++){
    Serial.print(_msg_bits[i]); Serial.print(",");
  }
  Serial.println();
}
