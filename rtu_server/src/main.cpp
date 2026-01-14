
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiServer.h>

const char* ssid = "Hatzerim";
const char* password = "";
const uint16_t port = 8080;                                             // Port for the TCP server

WiFiServer server(port);
const uint32_t RX_TIMEOUT_MS = 25;                                      // timeout after last byte
const size_t MAX_BUF = 512;                                             // max message size

uint8_t rxBuf[MAX_BUF];
uint8_t txBuf[MAX_BUF];
size_t  rxLen = 0;
size_t  txLen = 0;



void setup() {
  Serial.begin(115200);                                                 // Initialize serial communication
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {                               // wait for saerver to connect to wifi network
      delay(100);
      Serial.print(".");
  }

  Serial.println("\r");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("RRSI: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");

  server.begin();                                                       // Start the server
  Serial.print("TCP server started on port ");
  Serial.println(port);

  Serial.println("\nConnected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();
  server.setNoDelay(true);


}

void loop() {
  WiFiClient client = server.available();                                // Check if a client has connected
  if (client) {
    Serial.println("Client connected.");
    rxLen = 0;
    uint32_t lastRxTime = millis();

    while (client.connected()) {                                         // Loop while the client is connected
      while(client.available()) {                                          // Check for incoming data
        if (rxLen < MAX_BUF) {
          rxBuf[rxLen++] = client.read();
          lastRxTime = millis();
        } 
        else {
          client.read();                                                 // discard overflow
        }
        if (rxLen > 0 && (millis() - lastRxTime) > RX_TIMEOUT_MS) {          // timeout after last received byte
          break;
        }
      }  
      if (rxLen > 0) {
        Serial.print("Received ");
        Serial.print(rxLen);
        Serial.println(" bytes:");

        for (size_t i = 0; i < rxLen; i++) {
          Serial.printf("%02X ", rxBuf[i]);
        }
        Serial.println();

        // ---- echo back ----
        client.write(rxBuf, rxLen);
        Serial.println("Echo sent");
        rxLen = 0;
      }
    }
  }
}


/*
void loop() {
  WiFiClient client = server.available();
  if (!client) return;

  Serial.println("Client connected");

  rxLen = 0;
  uint32_t lastRxTime = millis();

  while (client.connected()) {
    while (client.available()) {
      if (rxLen < MAX_BUF) {
        rxBuf[rxLen++] = client.read();
        lastRxTime = millis();
      } else {
        client.read(); // discard overflow
      }
    }

    // timeout after last received byte
    if (rxLen > 0 && (millis() - lastRxTime) > RX_TIMEOUT_MS) {
      break;
    }
  }

  if (rxLen > 0) {
    Serial.print("Received ");
    Serial.print(rxLen);
    Serial.println(" bytes:");

    for (size_t i = 0; i < rxLen; i++) {
      Serial.printf("%02X ", rxBuf[i]);
    }
    Serial.println();

    // ---- echo back ----
    client.write(rxBuf, rxLen);
    Serial.println("Echo sent");
  }

  client.stop();
  Serial.println("Client disconnected\n");
}
*/