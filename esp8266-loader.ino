#include <stdio.h>
#include <stdarg.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include "proploader.h"
#include "fastproploader.h"

//////////////////////
// WiFi Definitions //
//////////////////////
// the file ap.h should contain the following two lines:
//const char WiFiSSID[] = "my_ssid";
//const char WiFiPSK[] = "my_password";
#include "ap.h"

WiFiServer server(80);
PropellerConnection connection;
PropellerLoader loader(connection);
FastPropellerLoader fastLoader(connection);

// spin .binary image buffer also used as a general purpose buffer
// this must be >= MAX_PACKET_SIZE defined in fastproploader.h
#define MAX_IMAGE_SIZE  8192

uint8_t image[MAX_IMAGE_SIZE]; // don't want big arrays on the stack

enum TransactionType {
  ttUnknown,
  ttLoad,
  ttLoadBegin,
  ttLoadData,
  ttLoadEnd,
  ttPacket
};

const char *FindArg(String &req, const char *key);
void InitResponse();
void SendResponse(WiFiClient &client, int code, const char *fmt, ...);
void SendTextResponse(WiFiClient &client, const char *result);
void connectWiFi();
void setupMDNS();

void setup() 
{
  Serial.begin(INITIAL_BAUD_RATE);
  connectWiFi();
  server.begin();
  setupMDNS();
}

void loop() 
{
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client)
    return;

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  if (client.peek() == '\n')
    client.read();
 
  // read the rest of the header
  while (client.available() > 0) {
    String hdr = client.readStringUntil('\r');
    if (client.peek() == '\n')
      client.read();
    if (hdr.length() == 0)
      break;
  }

  bool handled = false;
  const char *result = NULL;
  InitResponse();
  
  if (req.indexOf("POST") == 0) {
    TransactionType transactionType = ttUnknown;
    LoadType loadType;

    if (req.indexOf("/program-and-run") != -1) {
      transactionType = ttLoad;
      loadType = ltDownloadAndProgramAndRun;
    }
    else if (req.indexOf("/program") != -1) {
      transactionType = ttLoad;
      loadType = ltDownloadAndProgram;
    }
    else if (req.indexOf("/run") != -1) {
      transactionType = ttLoad;
      loadType = ltDownloadAndRun;
    }
    else if (req.indexOf("/load-begin") != -1)
      transactionType = ttLoadBegin;
    else if (req.indexOf("/load-data") != -1)
      transactionType = ttLoadData;
    else if (req.indexOf("/load-end") != -1)
      transactionType = ttLoadEnd;
    else if (req.indexOf("/packet") != -1)
      transactionType = ttPacket;

    switch (transactionType) {
      
    case ttLoad:
      {
        int imageSize = 0;
        while (client.available() > 0 && imageSize < sizeof(image))
          image[imageSize++] = client.read();
        if (loader.load(image, imageSize, loadType) == 0)
          result = "Success!\r\n";
        else
          result = "Load failed\r\n";
        SendTextResponse(client, result);
        handled = true;
      }
      break;
      
    case ttPacket: // write binary data to serial port and receive an 8 byte response
      {
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
          handled = true;
        }
      }
      break;
      
    case ttLoadBegin:
      {
        int initialBaudRate = INITIAL_BAUD_RATE;
        int finalBaudRate = FINAL_BAUD_RATE;
        int imageSize = -1;
        const char *arg;
        if ((arg = FindArg(req, "size=")) != NULL)
          imageSize = atoi(arg);
        if ((arg = FindArg(req, "initial-baud-rate=")) != NULL)
          initialBaudRate = atoi(arg);
        if ((arg = FindArg(req, "final-baud-rate=")) != NULL)
          finalBaudRate = atoi(arg);
        if (imageSize == -1)
          SendResponse(client, 403, "image size missing");
        else if (fastLoader.loadBegin(imageSize, initialBaudRate, finalBaudRate) == 0)
          SendResponse(client, 200, "OK");
        else
          SendResponse(client, 403, "loadBegin failed");
        handled = true;
      }
      break;
      
    case ttLoadData:
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
        handled = true;
      }
      break;
      
    case ttLoadEnd:
      {
        LoadType loadType = ltDownloadAndRun;
        if (req.indexOf("command=run") != -1)
          loadType = ltDownloadAndRun;
        else if (req.indexOf("command=program-and-run") != -1)
          loadType = ltDownloadAndProgramAndRun;
        else if (req.indexOf("command=program") != -1)
          loadType = ltDownloadAndProgram;
        if (fastLoader.loadEnd(loadType) == 0)
          SendResponse(client, 200, "OK");
        else
          SendResponse(client, 403, "loadEnd failed");
        handled = true;
      }
      break;
    }
  }
  
  client.flush();

  // send an error response for unknown requests
  if (!handled)
    client.print("HTTP/1.1 404 Not found\r\n");

  delay(1);
  //Serial.println("Client disonnected");

  // The client will actually be disconnected 
  // when the function returns and 'client' object is destroyed
}

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
  s += errorText;
  s += "</html>\r\n";
  client.print(s);
}

void SendTextResponse(WiFiClient &client, const char *result)
{
  String s;
  s += "HTTP/1.1 200 OK\r\n";
  s += "Content-Type: text/html\r\n\r\n";
  s += "<!DOCTYPE HTML>\r\n<html>\r\n";
  s += result;
  s += "</html>\r\n";
  client.print(s);
}

void connectWiFi()
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

