/*
lhri_msg class handles  message fields conversion into byte string passed between nodes and vica versa
*/

#ifndef LhriProtocol_h
#define LhriProtocol_h

#include "Arduino.h"

enum fieldName {dir,level,dlevel,path,cmd,pload};                       // enumartion of fields
const String flds_lbl[6] ={"dir","level","dlevel","path","cmd","pload"};  // used by prt_flds

struct lhri_msg{                                                        // message fields structure, command & response 
  uint8_t dir;
  uint8_t level;
  uint8_t dlevel;
  uint32_t path;
  uint8_t cmd;
  uint16_t pload;
  uint8_t err;
  uint8_t port; 
};

struct lhri_str{                                                         // transmit and receive message byte string
  uint8_t bytes[11];
  uint8_t len;
};

struct fld_mdata{                                                       // message fields metadata 
  uint32_t val;                                                         // values size is determined by longest path - 32 bits 
  uint8_t pos;                                                          // position in bit representation of byte string  
  uint8_t len;                                                          // length (bits) of the field 
};

class LhriProtocol{
  public:
    LhriProtocol();                                                     // create an instance of the class                     
    void Init();
    lhri_str SendMsg(lhri_msg arg_msg);                                 // convert meesage fields structure to bytes string 
    lhri_msg RecMsg(lhri_str arg_rec_msg);                              // convert bytes string to message fields     
  private:
    uint8_t crc8_ccitt(const uint8_t *data, uint16_t length);           // delete "const" TBD
    void flds_2_vals(lhri_msg arg_msg);                                 // update metadata sturcutres values from fields 
    lhri_msg vals_2_flds();                                             // update fields from metadata sturcutres values 
    void update_pload_len();                                            // update payload length based on command type 
    void trs_rev_bits(uint8_t arg_field);                               // reveret endian of fields values into a bit stream 
    void bits_2_fld (uint8_t arg_fld);                                  // revert field bits into field value 
    void prt_flds(String title);                                        // display metadata structres content - for debugging 
    void prt_bits(String title);                                        // display message bit stream  - for debigging
    fld_mdata _flds[6];                                                 // message fields metadata strucutre
    uint8_t _msg_bits[72];                                              // message bit stream 
    lhri_str _msg;                                                      // message bytes string structre
    uint8_t _pos;                                                       // field start position in message bit stream      
};

#endif
