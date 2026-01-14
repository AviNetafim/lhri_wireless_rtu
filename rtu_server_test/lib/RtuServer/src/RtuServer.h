/* class handling net serial protocol v2 with write to multiple register values in segments 
 *  created 31/12/25
 *  
* command structure
* general    |             header                                                 |         cmd payload         | CRC                | total length
* write:     |RTU | CMD | address | index low | index high | size low | size high | size X (val_low + val_high) | CRC low | CRC high |9 + size X 2 
* read:      |RTU | CMD | address | index low | index high | size low | size high |                             | CRC low | CRC high |9 
*
* response  structure
* general    |                     header                                         |   resp payload                            |  CRC               | total length
* write:     |RTU | CMD | address | index low | index high | size low | size high | compeltion code                           | CRC low | CRC high |10 
* read:      |RTU | CMD | address | index low | index high | size low | size high | completion code | size X low_val+high_val | CRC low | CRC high |10 + size X 2 
*/


#ifndef rtuserver_h
#define rtuserver_h

#include <WiFi.h>
#include <WiFiServer.h>

#define myRTU 0x01                                                      // RTU ID
#define RTU 0                                                           // command index of RTU                                              
#define CMD 1                                                           // command index of command write (6) or read (4) or discover (2)                                             
#define ADD 2                                                           // command index of register number                                              
#define IXL 3                                                           // command index of register start element low byte                                            
#define IXH 4                                                           // command index of register start element high byte                                                                                        
#define SZL 5                                                           // command index of number of register # of elements low byte                                              
#define SZH 6                                                           // command index of number of register # of elements high byte                                                                                            
#define CODE 7                                                          // command index of completion code  

const int REG_MAP_SIZE = 3;                                             // register list size   
const int MAX_BUFFS_SIZE = 128; 
const uint32_t RX_TIMEOUT_MS = 25;                                      // timeout after last byte

typedef struct {                                                        // register list metadata structure
  int dim;                                                              // register array size 
  int in_eeprom_size;                                                   // reister eeprom segment size
  uint16_t eeprom_start_add;                                            // keep register in eeprom switch                
  uint16_t *point;                                                      // pointer to 1st register array element
  }registers;


class RtuServer{
  public:
    RtuServer(uint16_t port);
    void Init(const char* ssid,const char* paswword);
    bool cmd_available(registers (&aregs)[REG_MAP_SIZE]);
    void send_reg(uint8_t reg_no,uint8_t reg_val);
    bool reply();

  private:
    void parse_cmd( registers (&aregs)[REG_MAP_SIZE]);                  // protocol implementation   
    int resp_crc(String when, int arg_resp_size);                       // crc clculation of protocol response 
    void prt_msg(String when, uint8_t arg_msg[], int arg_msg_size);         // print message to  serial monitor
    WiFiServer _server;
    uint8_t _rxBuf[MAX_BUFFS_SIZE];                                         // receive buffer 
    int _rxLen;                                                             // recieve buffer length
    bool _receiving = false;                                                // flag indicitating first byte recvieved 
    uint8_t _txBuf[MAX_BUFFS_SIZE];                                         // transmit buffer    
    int _txLen;                                                             // transmit buffer length

};

#endif 
