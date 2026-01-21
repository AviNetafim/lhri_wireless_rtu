Got it—you want an ESP32 acting as a Wi‑Fi server and a Python client on Windows 11 that controls the connection (i.e., the client decides when to connect, send commands, keep alive, and disconnect). Below is a simple, reliable pattern using plain TCP with a line-based protocol (e.g., PING, STATUS, LED_ON, LED_OFF, BYE). It’s easy to test, firewall-friendly for outgoing client connections, and works with both ESP32 in STA (joining your router) or AP (ESP32 creates its own hotspot).
Architecture in brief

    Transport: TCP (line-delimited text commands)
    ESP32: Listens on port 5050, responds to commands, only accepts one client at a time (can expand later).
    Client (Windows 11 / Python): Connects, optionally reads a greeting, sends commands, handles timeouts and reconnection. Client controls disconnect with BYE.

Option A (recommended): ESP32 Arduino (WiFi TCP server)

    Works with Arduino IDE / PlatformIO using the Arduino core for ESP32.

1) ESP32 server code (Arduino)

    Mode: Station (connects to your router).
    If you prefer AP (hotspot), see the comment in the code to switch modes.
    Protocol: commands end with \n, responses end with \n.

#include <WiFi.h>

/// ---- CONFIG ----
const char ssid     = "YourRouterSSID";   // <-- set this
const char password = "YourRouterPass";   // <-- set this
const uint16t PORT = 5050;

WiFiServer server(PORT);
WiFiClient client;

const int LEDPIN = 2; // On-board LED on many ESP32 dev boards

void waitForWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.mode(WIFISTA);                       // Use WIFIAP for Access Point mode
  WiFi.begin(ssid, password);
  int tries = 0;
  while (WiFi.status() != WLCONNECTED) {
    delay(500);
    Serial.print(".");
    if (++tries % 20 == 0) Serial.println();
  }
  Serial.println();
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());
}

String readLine(WiFiClient &c) {
  String line;
  while (c.connected()) {
    while (c.available()) {
      char ch = c.read();
      if (ch == '\n') return line;
      if (ch != '\r') line += ch;
    }
    delay(1); // yield
  }
  return line; // May be partial if disconnected
}

void sendLine(WiFiClient &c, const String &msg) {
  c.print(msg); c.print("\n");
}

void handleCommand(const String &raw) {
  String cmd = raw;
  cmd.trim();
  String up = cmd; up.toUpperCase();

  if (!client || !client.connected()) return;

  if (up == "PING") {
    sendLine(client, "PONG");
  } else if (up == "LEDON") {
    digitalWrite(LEDPIN, HIGH);
    sendLine(client, "OK LED=ON");
  } else if (up == "LEDOFF") {
    digitalWrite(LEDPIN, LOW);
    sendLine(client, "OK LED=OFF");
  } else if (up == "STATUS") {
    String led = digitalRead(LEDPIN) ? "ON" : "OFF";
    sendLine(client, "OK IP=" + WiFi.localIP().toString() + " RSSI=" + String(WiFi.RSSI()) + " LED=" + led);
  } else if (up == "BYE") {
    sendLine(client, "BYE");
    client.stop(); // Client controls disconnect
  } else {
    sendLine(client, "ERR UNKNOWNCMD");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, LOW);

  // ---- If you want AP mode instead of STA, use:
  // WiFi.mode(WIFIAP);
  // WiFi.softAP("ESP32_AP", "esp32pass123");
  // Serial.print("AP started. IP: ");
  // Serial.println(WiFi.softAPIP());

  waitForWiFi();

  server.begin();
  server.setNoDelay(true);
  Serial.print("Server listening on port "); Serial.println(PORT);
}

void loop() {
  // Accept a new client if none is connected
  if (!client || !client.connected()) {
    WiFiClient newClient = server.available();
    if (newClient) {
      client.stop();            // drop previous if any
      client = newClient;
      client.setTimeout(2000);  // read timeout
      sendLine(client, "HELLO ESP32");
      Serial.println("Client connected.");
    }
  } else {
    // Process incoming data
    if (client.available()) {
      String line = readLine(client);
      if (line.length() > 0) {
        handleCommand(line);
      }
    }

    // If client dropped, clean up
    if (!client.connected()) {
      Serial.println("Client disconnected.");
      client.stop();
    }
  }
}
``

    AP mode note: If you switch to AP mode, the ESP32 server IP is typically 192.168.4.1 by default.

2) Windows 11 Python client (controls connection)

    Uses plain socket, line-based protocol, client drives the session.
    Includes reconnect with backoff and optional heartbeats (PING).
    Outbound connections are typically allowed by Windows Defender Firewall; no extra inbound rule needed for the client.

import socket
import time

ESPIP = "192.168.1.123"   # <-- set this to ESP32 IP; if AP mode, likely 192.168.4.1
ESPPORT = 5050

def connect():
    # createconnection handles DNS and timeout
    s = socket.createconnection((ESPIP, ESPPORT), timeout=5)
    # Read/write timeouts (seconds) so we don't block forever
    s.settimeout(2.0)
    return s

def readline(sock):
    """Read until newline; returns str without trailing newline."""
    buf = b""
    while True:
        chunk = sock.recv(4096)
        if not chunk:
            raise ConnectionError("Socket closed by server")
        buf += chunk
        if b"\n" in buf:
            line, , rest = buf.partition(b"\n")
            # If there's extra bytes after newline, you could keep them for next read
            return line.decode("utf-8", errors="replace").strip()

def sendline(sock, line: str):
    data = (line.strip() + "\n").encode("utf-8")
    sock.sendall(data)
    # Expect one-line response
    return readline(sock)

def main():
    backoff = 1
    while True:
        try:
            print("Connecting to ESP32…")
            sock = connect()

            # Optional: read the server greeting
            try:
                greeting = readline(sock)
                if greeting:
                    print("Server:", greeting)
            except (socket.timeout, ConnectionError):
                pass

            backoff = 1  # reset backoff after success

            # ---- Your control sequence ----
            print("STATUS ->", sendline(sock, "STATUS"))
            print("LEDON ->", sendline(sock, "LEDON"))

            # Heartbeats
            for i in range(3):
                time.sleep(3)
                print("PING ->", sendline(sock, "PING"))

            # Graceful client-controlled disconnect
            print("BYE ->", send_line(sock, "BYE"))
            sock.close()

            print("Disconnected by client. Sleep 10s before next cycle…")
            time.sleep(10)

        except (OSError, ConnectionError, socket.timeout) as e:
            print("Connection problem:", e)
            time.sleep(backoff)
            backoff = min(backoff * 2, 60)  # exponential backoff up to 60s

if name == "main":
    main()

How the client controls the connection

    The client decides when to connect (socket.create_connection).
    The client dictates the session by sending commands.
    The client closes gracefully by sending BYE (server confirms, then closes) or just closing the socket on its side.
    Heartbeats (PING/PONG) let the client detect dead connections and reconnect.

Finding the ESP32 IP (STA mode)

    Open the Arduino Serial Monitor: the sketch prints Connected. IP: ....
    Or check your router’s DHCP leases.
    In AP mode, default is usually 192.168.4.1.

Windows 11 tips

    Python: Use Python 3.10+; run the client as a normal user (python client.py).
    Firewall: Outbound client connections are typically permitted. If you later host a server on Windows, create an inbound rule or allow on first prompt.

Troubleshooting

    Nothing happens / no response: Ensure commands end with \n. The server only responds after newline.
    Connection refused: Verify the ESP32 is actually connected and server.begin() executed (watch serial logs).
    Timeouts: Increase sock.settimeout(…) or ensure the ESP32 loop reads promptly (avoid long blocking calls).
    Random disconnects: Use heartbeats. If you need more resilience, reconnect with backoff (already in the Python sample).

Variations & enhancements

    AP mode: Replace STA join with WiFi.softAP("ESP32_AP", "esp32pass123"); and use WiFi.softAPIP().
    WebSocket or HTTP: If you want browser control or structured messaging, use WebSockets (ESPAsyncWebServer + Python websockets) or simple HTTP endpoints. This adds overhead but can be convenient.
    Security: This example is unencrypted. Keep your Wi‑Fi WPA2 password secure. For encryption, use WiFiClientSecure on ESP32 and Python ssl to wrap the socket (requires certs and more setup).
    Multiple clients: Maintain a list of WiFiClient objects and iterate over them; add a simple state machine and queues if needed.

Want me to tailor this?

Tell me:

    ESP32 framework you prefer (Arduino / ESP-IDF / MicroPython)?
    Network mode (STA to your router or AP hotspot)?
    Commands you need beyond LED control (e.g., GPIO, ADC, PWM, file I/O)?
    Do you need encryption (TLS) or is plain TCP ok on your LAN?

I can then adapt the code (including ESP-IDF or MicroPython versions) and add a small, robust message protocol specific to your use case.
