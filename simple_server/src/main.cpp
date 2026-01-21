/*
* esp32 station mode serever sample program
* not in use
* created 12/1/2026 
*/
#include <Arduino.h>
#include <WiFi.h>
// #include <ESPmDNS.h>
#include <WiFiServer.h>

const char* ssid = "Hatzerim-Free";
const char* password = "";
// const char* ssid = "Schweitzer";
// const char* password = "plmqa212";
const uint16_t port = 8080;                                             // Port for the TCP server
// const char* hostname ="tree-bridge";

WiFiServer server(port);
uint16_t rxLen;
uint8_t rxBuf[256];
uint32_t start;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  // WiFi.setHostname(hostname);
  uint8_t connect_cnt = 0;
  while (WiFi.status() != WL_CONNECTED && connect_cnt < 50) {                                 // wait for saerver to connect to wifi network
    delay(100);
    connect_cnt += 1;
    Serial.print(".");
  }
  if (connect_cnt >= 50){
    Serial.print("\rcould no connect to network");
  }
  else{
    // if(MDNS.begin(hostname)){
    //   MDNS.addService("http","tcp",port);
    // }
    // else{
    //   Serial.print("error starting mDNS");
    // }
    Serial.println("\r");
    Serial.println("WiFi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Host Name: ");
    Serial.println(WiFi.getHostname());
    Serial.print("MAC address: ");
    Serial.println(WiFi.macAddress());
    Serial.print("RRSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    server.begin();                                                       // Start the server
    server.setNoDelay(true);
    Serial.print("TCP server started on port ");
    Serial.println(port);
  }
}

void loop(){ 
  WiFiClient client = server.available();                               // Check if a client has connected
  if (!client) return;
  Serial.println("Client connected.");
  start = millis();
  rxLen = 0;
  while (client.connected()) {                                       // Loop while the client is connected
    while(client.available() > 0) {                                   // Check for incoming data
      if (rxLen < 256) {
        rxBuf[rxLen++] = client.read();
      } 
      else {
        client.read();                                             // discard overflow
      }
      // if (rxLen > 0 && (millis() - lastRxTime) > RX_TIMEOUT_MS) {     // timeout for client received data
      //   break;
      // }
    }  
    if (rxLen > 0) {                                                        
      Serial.print("Received ");
      Serial.print(rxLen);
      Serial.println(" bytes:");
      for (size_t i = 0; i < rxLen; i++) {
        Serial.printf("%02X ", rxBuf[i]);
      }
      Serial.println();
      client.write(rxBuf, rxLen);
      rxLen = 0 ;
    }
  }
  Serial.print("client connection closed after "); 
  Serial.print(millis()-start);
  Serial.println(" ms"); 
  client.stop();
}
