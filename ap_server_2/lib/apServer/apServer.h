#ifndef apserver_h
#define apserver_h

#include <WiFi.h>
#include <WiFiServer.h>

const uint32_t RX_TIMEOUT_MS = 25;                                      // timeout after last byte
const uint16_t MAX_BUFFS_SIZE = 256;

class apServer{
    public:
        apServer(uint16_t port);
        void Init(const char* ssid,const char* paswword);
        void client_connect();
        bool cmd_available();
        void reply();
    private:
        WiFiServer _server;
        WiFiClient _client;
        uint8_t _rxBuf[MAX_BUFFS_SIZE];                                         // receive buffer 
        int _rxLen;                                                             // recieve buffer length
        uint8_t _txBuf[MAX_BUFFS_SIZE];                                         // transmit buffer    
        int _txLen;                                                             // transmit buffer length
        uint32_t _start_ms;
        bool _connected = false;
};

#endif
 