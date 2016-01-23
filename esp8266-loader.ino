#include <stdio.h>
#include <stdarg.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUDP.h>
#include <FS.h>

#include "proploader.h"
#include "fastproploader.h"

#define AP_NAME_PREFIX  "ESP-PROP-PLUG"

// baud rate to use after a successful load
#define PROGRAM_BAUD_RATE 115200

//////////////////////
// WiFi Definitions //
//////////////////////
// the file ap.h should contain the following two lines:
//const char WiFiSSID[] = "my_ssid";
//const char WiFiPSK[] = "my_password";
#include "ap.h"

/*
/Users/dbetz/Library/Arduino15/packages/esp8266/tools/esptool/0.4.6/esptool -vv -cd ck -cb 115200 -cp /dev/cu.usbserial-A700fKXl -ca 0x00000 -cf esp8266-loader.bin
*/

WiFiServer server(80);
WiFiServer telnetServer(23);
WiFiClient telnetClient;
WiFiUDP discoverServer;
bool ffsMounted = false;

PropellerConnection connection;
PropellerLoader loader(connection);
FastPropellerLoader fastLoader(connection);

// spin .binary image buffer also used as a general purpose buffer
// this must be >= MAX_PACKET_SIZE defined in fastproploader.h
#define MAX_IMAGE_SIZE    8192

uint8_t image[MAX_IMAGE_SIZE]; // don't want big arrays on the stack

// HTTP GET request handlers
int handleDirReq(WiFiClient &client, String &req);

// HTTP POST request handlers
int handleLoadReq(WiFiClient &client, String &req, LoadType loadType);
int handlePacketReq(WiFiClient &client, String &req);
int handleLoadBeginReq(WiFiClient &client, String &req);
int handleLoadDataReq(WiFiClient &client, String &req);
int handleLoadEndReq(WiFiClient &client, String &req);
int handleFormatReq(WiFiClient &client, String &req);

void handleHTTP(WiFiClient &client);
const char *FindArg(String &req, const char *key);
void InitResponse();
void SendResponse(WiFiClient &client, int code, const char *fmt, ...);
void setupSoftAP();
void setupSTA();
void setupMDNS();

void setup() 
{
  Serial.begin(PROGRAM_BAUD_RATE);
  //setupSoftAP();
  setupSTA();
  setupMDNS();
  Serial.end();

  ffsMounted = SPIFFS.begin();
  server.begin();
  telnetServer.begin();
  discoverServer.begin(2000);
}

void loop() 
{
  // handle HTTP connections
  WiFiClient client = server.available();
  if (client)
    handleHTTP(client);

  // handle telnet connections
  if (telnetServer.hasClient()) {
    if (telnetClient && telnetClient.connected())
      telnetClient.stop();
    telnetClient = telnetServer.available();
  }

  // handle telnet traffic
  if (telnetClient && telnetClient.connected()) {
    while (Serial.available())
      telnetClient.write(Serial.read());
    while (telnetClient.available())
      Serial.write(telnetClient.read());
  }

  // discovery experiments
#define DISCOVER_REPLY  "ESP8266 here!\n"
int cnt;
  if ((cnt = discoverServer.parsePacket()) > 0) {
    discoverServer.beginPacket(discoverServer.remoteIP(), discoverServer.remotePort());
    discoverServer.write(DISCOVER_REPLY, strlen(DISCOVER_REPLY));
    discoverServer.endPacket();
  }

  delay(1);
}

void handleHTTP(WiFiClient &client)
{
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  if (client.peek() == '\n')
    client.read();
 
  // skip the rest of the header
  while (client.available() > 0) {
    String hdr = client.readStringUntil('\r');
    if (client.peek() == '\n')
      client.read();
    if (hdr.length() == 0)
      break;
  }

  InitResponse();
  
  if (req.indexOf("GET") == 0) {
    if (req.indexOf("/dir") != -1)
      handleDirReq(client, req);
    else
      SendResponse(client, 404, "Not Found");
  }
  
  else if (req.indexOf("POST") == 0) {
    if (req.indexOf("/program-and-run") != -1)
      handleLoadReq(client, req, ltDownloadAndProgramAndRun);
    else if (req.indexOf("/program") != -1)
      handleLoadReq(client, req, ltDownloadAndProgram);
    else if (req.indexOf("/run") != -1)
      handleLoadReq(client, req, ltDownloadAndRun);
    else if (req.indexOf("/load-begin") != -1)
      handleLoadBeginReq(client, req);
    else if (req.indexOf("/load-data") != -1)
      handleLoadDataReq(client, req);
    else if (req.indexOf("/load-end") != -1)
      handleLoadEndReq(client, req);
    else if (req.indexOf("/packet") != -1)
      handlePacketReq(client, req);
    else if (req.indexOf("/format") != -1)
      handleFormatReq(client, req);
    else
      SendResponse(client, 404, "Not Found");
  }
}

int handleLoadReq(WiFiClient &client, String &req, LoadType loadType)
{
  int baudRate = INITIAL_BAUD_RATE;
  int resetPin = DEF_RESET_PIN;
  int imageSize = 0;
  const char *arg;
  
  if ((arg = FindArg(req, "baud-rate=")) != NULL)
    baudRate = atoi(arg);
  if ((arg = FindArg(req, "reset-pin=")) != NULL)
    resetPin = atoi(arg);
    
  while (client.available() > 0 && imageSize < sizeof(image))
    image[imageSize++] = client.read();
    
  connection.setBaudRate(baudRate);
  connection.setResetPin(resetPin);
  if (loader.load(image, imageSize, loadType) == 0)
    SendResponse(client, 200, "OK");
  else
    SendResponse(client, 403, "Load failed");
}
      
int handlePacketReq(WiFiClient &client, String &req)
{
  bool handled = false;
  int cnt;
  
  while (client.available()) {
    if ((cnt = client.read(image, MAX_PACKET_SIZE)) > 0) {
      if (connection.sendData(image, cnt) != cnt) {
        client.print("HTTP/1.1 403 sendData failed\r\n");
        handled = true;
      }
    }
  }
  
  if (!handled) {
    if ((cnt = connection.receiveDataExactTimeout(image, 8, 2000)) == 8) {
      client.print("HTTP/1.1 200 OK\r\n");
      client.write((char *)image, cnt);
    }
    else
      client.print("HTTP/1.1 403 Timeout receiving response\r\n");
  }
}
      
int handleLoadBeginReq(WiFiClient &client, String &req)
{
  int initialBaudRate = INITIAL_BAUD_RATE;
  int finalBaudRate = FINAL_BAUD_RATE;
  int resetPin = DEF_RESET_PIN;
  int imageSize = -1;
  const char *arg;
  
  if ((arg = FindArg(req, "size=")) != NULL)
    imageSize = atoi(arg);
  if ((arg = FindArg(req, "initial-baud-rate=")) != NULL)
    initialBaudRate = atoi(arg);
  if ((arg = FindArg(req, "final-baud-rate=")) != NULL)
    finalBaudRate = atoi(arg);
  if ((arg = FindArg(req, "reset-pin=")) != NULL)
    resetPin = atoi(arg);
    
  if (imageSize == -1)
    SendResponse(client, 403, "image size missing");
  else {
    connection.setBaudRate(initialBaudRate);
    connection.setResetPin(resetPin);
    if (fastLoader.loadBegin(imageSize, initialBaudRate, finalBaudRate) == 0)
      SendResponse(client, 200, "OK");
    else
      SendResponse(client, 403, "loadBegin failed");
  }
}
      
int handleLoadDataReq(WiFiClient &client, String &req)
{
  int cnt = 0;
  while ((cnt = client.available()) > 0) {
    AppendResponseText("Data available: %d", cnt);
    if ((cnt = client.readBytes(image, sizeof(image))) > 0) {
      AppendResponseText("Loading %d bytes", cnt);
      if (fastLoader.loadData(image, cnt) != 0) {
        SendResponse(client, 403, "loadData failed");
        cnt = -1;
        break;
      }
    }
  }
  
  if (cnt >= 0 && !client.available())
    SendResponse(client, 200, "OK");
}
      
int handleLoadEndReq(WiFiClient &client, String &req)
{
  LoadType loadType = ltDownloadAndRun;
  
  if (req.indexOf("command=run") != -1)
    loadType = ltDownloadAndRun;
  else if (req.indexOf("command=program-and-run") != -1)
    loadType = ltDownloadAndProgramAndRun;
  else if (req.indexOf("command=program") != -1)
    loadType = ltDownloadAndProgram;
    
  if (fastLoader.loadEnd(loadType) == 0) {
    SendResponse(client, 200, "OK");
    connection.setBaudRate(PROGRAM_BAUD_RATE);
  }
  else
    SendResponse(client, 403, "loadEnd failed");
}

int handleDirReq(WiFiClient &client, String &req)
{
  if (!ffsMounted)
    AppendResponseText("FFS not mounted");
  else{
    FSInfo info;
    if (!SPIFFS.info(info))
      AppendResponseText("Failed to get FFS info");
    else {
      AppendResponseText("totalBytes: %ld", info.totalBytes);
      AppendResponseText("usedBytes: %ld", info.usedBytes);
      AppendResponseText("blockSize: %ld", info.blockSize);
      AppendResponseText("pageSize: %ld", info.pageSize);
      AppendResponseText("maxOpenFiles: %ld", info.maxOpenFiles);
      AppendResponseText("maxPathLength: %ld", info.maxPathLength);
      Dir dir = SPIFFS.openDir("/");
      while (dir.next())
          AppendResponseText("%s %d", dir.fileName().c_str(), dir.openFile("r").size());
    }
  }
  SendResponse(client, 200, "OK");
}
      
int handleFormatReq(WiFiClient &client, String &req)
{
  if (!SPIFFS.format())
    AppendResponseText("Format failed");
  else {
    if (!ffsMounted)
      ffsMounted = SPIFFS.begin();
  }
  SendResponse(client, 200, "OK");
}
      
#ifdef SUPPORT_STAMP
int handleStampReq(WiFiClient &client, String &req)
{
  if (isBS2())
    AppendResponseText("Found a BS2");
  else
    AppendResponseText("Unknown type of stamp");
  SendResponse(client, 200, "OK");
}
#endif

const char *FindArg(String &req, const char *key)
{
  int i;
  if ((i = req.indexOf(key)) == -1)
    return NULL;
  return req.c_str() + i + strlen(key);
}

String errorText;

void InitResponse()
{
  errorText.remove(0);
  AppendResponseText("FFS is%s mounted.", ffsMounted ? "" : " not");
}

void AppendResponseText(const char *fmt, ...)
{
  char buf[1024];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  errorText += "<p>";
  errorText += buf;
  errorText += "</p>\r\n";
}

void SendResponse(WiFiClient &client, int code, const char *fmt, ...)
{
  char buf[1024];
  va_list ap;
  String s;
  snprintf(buf, sizeof(buf), "HTTP/1.1 %d ", code);
  s += buf;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  s += buf;
  s += "\r\nContent-Type: text/html\r\n\r\n";
  s += "<!DOCTYPE HTML>\r\n<html>\r\n";
  s += "<body>\r\n";
  s += errorText;
  s += "</body>\r\n";
  s += "</html>\r\n";
  client.print(s);
}

void setupSoftAP()
{
  WiFi.mode(WIFI_AP);
  
  uint8_t macAddr[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(macAddr);

  char apName[sizeof(AP_NAME_PREFIX) + 3 * WL_MAC_ADDR_LENGTH];
  strcpy(apName, AP_NAME_PREFIX);

  char *p = apName + strlen(apName);
  char *end = &apName[sizeof(apName)];
  for (int i = 0; i < WL_MAC_ADDR_LENGTH; ++i) {
    int remaining = end - p;
    snprintf(p, remaining, "%c%02x", (i == 0 ? '-' : ':'), macAddr[i]);
    p += 3;
  }
  
  WiFi.softAP(apName);
}

void setupSTA()
{
//  byte ledStatus = LOW;
  
  Serial.println();
  Serial.println("Connecting to: " + String(WiFiSSID));
  
  // Set WiFi mode to station (as opposed to AP or AP_STA)
  WiFi.mode(WIFI_STA);

  // WiFI.begin([ssid], [passkey]) initiates a WiFI connection
  // to the stated [ssid], using the [passkey] as a WPA, WPA2,
  // or WEP passphrase.
  WiFi.begin(WiFiSSID, WiFiPSK);

  // Use the WiFi.status() function to check if the ESP8266
  // is connected to a WiFi network.
  while (WiFi.status() != WL_CONNECTED) {
    
    // Blink the LED
//    digitalWrite(LED_PIN, ledStatus); // Write LED high/low
//    ledStatus = (ledStatus == HIGH) ? LOW : HIGH;

    // Delays allow the ESP8266 to perform critical tasks
    // defined outside of the sketch. These tasks include
    // setting up, and maintaining, a WiFi connection.
    delay(100);
    
    // Potentially infinite loops are generally dangerous.
    // Add delays -- allowing the processor to perform other
    // tasks -- wherever possible.
  }
  
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setupMDNS()
{
  // set up mDNS to point to "thing2.local"
  if (!MDNS.begin("thing2")) {
    Serial.println("Error setting up MDNS responder!");
    while (1)
      delay(1000);
  }
  Serial.println("mDNS responder started");
}

