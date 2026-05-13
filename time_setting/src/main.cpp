
/*
Setup Instructions:
1. Install Required Library: In the Arduino IDE, go to Sketch > Include Library > Manage Libraries, and install "NTPClient" by Fabrice Weinberg.
2. Update WiFi Credentials: Replace `"your_wifi_ssid"` and `"your_wifi_password"` with your actual WiFi network details.
3. Upload to ESP32: Open the sketch in Arduino IDE, select your ESP32 board, and upload.

Code Explanation:
- In `setup()`, the ESP32 starts in AP+STA mode, begins the AP, connects to WiFi, initializes NTP, and prints the current time.
- In `loop()`, it continuously updates and prints the time every second (you can adjust this as needed).

The `NTPClient` library in your ESP32 Arduino sketch provides several methods to retrieve the current time in different formats. Here's a summary of the key ones:

### Available Time Formats and Methods:
- **`getFormattedTime()`**: Returns a string in "HH:MM:SS" format (e.g., "14:30:45"). This is what's currently being printed in your `setup()` and `loop()` functions.
- **`getEpochTime()`**: Returns the Unix timestamp (seconds since January 1, 1970, UTC) as an `unsigned long` (e.g., 1715520000).
- **`getHours()`**: Returns the current hour (0-23) as an `int`.
- **`getMinutes()`**: Returns the current minute (0-59) as an `int`.
- **`getSeconds()`**: Returns the current second (0-59) as an `int`.
- **`getDay()`**: Returns the day of the week (0 = Sunday, 1 = Monday, ..., 6 = Saturday) as an `int`.

### Calculating Seconds of the Day:
If you want the total seconds elapsed since midnight (00:00:00), you can compute it using the individual components:
```cpp
unsigned long secondsOfDay = timeClient.getHours() * 3600UL + timeClient.getMinutes() * 60UL + timeClient.getSeconds();
```
This gives a value from 0 to 86399 (since there are 86400 seconds in a day).
*/



#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Arduino.h>

// WiFi credentials for STA mode (to connect to internet for NTP)
const char* ssid = "Hatzerim-Free";
const char* password = "";

// AP mode settings
const char* ap_ssid = "ESP32_AP";
const char* ap_password = "12345678"; // Minimum 8 characters

// NTP Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 10800, 864000000); // UTC+3 offset (Israel DST), update every 60 seconds

void setup() {
  Serial.begin(115200);

  // Set WiFi to AP + STA mode
  WiFi.mode(WIFI_AP_STA);

  // Start AP
  WiFi.softAP(ap_ssid, ap_password);
  Serial.println("AP started");
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // Connect to WiFi for internet access
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize NTP
  timeClient.begin();
  timeClient.update();

  Serial.print("Current time: ");
  Serial.println(timeClient.getFormattedTime());
}

void loop() {
  // Update time every loop or as needed
  timeClient.update();
  Serial.println(timeClient.getHours() * 60 + timeClient.getMinutes());
  delay(5000);
}
