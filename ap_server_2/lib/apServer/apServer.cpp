#include <Arduino.h>
#include "apServer.h"

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

// void apServer::client_connect(){
//     if (!_client || !_client.connected()) {
//         if (_client) {
//             _client.stop();
//             Serial.print("Client disconnected after ");
//             Serial.print(millis()-_start);
//             Serial.print(" ms");
//         }
//         _client = _server.available();
//         if (_client) {
//             Serial.println("Client connected");
//             _start = millis();
//         }
//         return;
//     }    
// }

void apServer::client_connect(){
    if (_connected){
        if (!_client.connected() || !_client){
            _client.stop();                                             // stop client
            Serial.println("Client disconnected after");
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
bool apServer::cmd_available(){
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
            // _client.write(_rxBuf, _rxLen);
            // _rxLen = 0; 
        }
        return true;
    }
    else{
        return false;
    }
}    

void apServer::reply(){
    for( int i = 0 ; i < _rxLen ; i++){
        _txBuf[i] = _rxBuf[i];
    }
    _txLen = _rxLen;
    _client.write(_txBuf, _txLen);
}