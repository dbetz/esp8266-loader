#ifdef SUPPORT_STAMP

#include "MySoftwareSerial.h"
MySoftwareSerial softSerial(3, 1, true); // RX, TX, Invert

// UART TX pin
#define TXPIN 1

void resetStamp();
bool isBS2();

int handleStampReq(WiFiClient &client, String &req)
{
  if (isBS2())
    AppendResponseText("Found a BS2");
  else
    AppendResponseText("Unknown type of stamp");
  SendResponse(client, 200, "OK");
}

void resetStamp()
{
  Serial.end();       // stop the serial object to use the TX pin
  pinMode(TXPIN, OUTPUT);
  digitalWrite(TXPIN, LOW);
  digitalWrite(PROPELLER_RESET_PIN, LOW);
  delay(2);
  digitalWrite(TXPIN, HIGH);
  delay(2);
  digitalWrite(PROPELLER_RESET_PIN, HIGH);
  delay(2);
  digitalWrite(PROPELLER_RESET_PIN, LOW);
  delay(36);
  digitalWrite(TXPIN, LOW);
  softSerial.begin(9600); // restart serial object
  delay(20);
  while (softSerial.available())
    softSerial.read();
}

int nextByte()
{
  int cnt = 100;
  while (--cnt >= 0) {
    if (softSerial.available())
      return softSerial.read();
    delay(10);
  }
  return -1;
}

bool isBS2()
{
  int byte;
  
  resetStamp();
  
  softSerial.write('B');
//  if ((byte = nextByte()) != 'B') {
//    AppendResponseText("No echo of 'B' %02x", byte);
//    return false;
//  }
  if ((byte = nextByte()) != (-'B' & 0xff)) {
    AppendResponseText("No complement of 'B' %02x", byte);
    return false;
  }
  
  softSerial.write('S');
//  if ((byte = nextByte()) != 'S') {
//    AppendResponseText("No echo of 'S' %02x", byte);
//    return false;
//  }
  if ((byte = nextByte()) != (-'S' & 0xff)) {
    AppendResponseText("No complement of 'S' %02x", byte);
    return false;
  }
  
  softSerial.write('2');
//  if ((byte = nextByte()) != '2') {
//    AppendResponseText("No echo of '2' %02x", byte);
//    return false;
//  }
  if ((byte = nextByte()) != (-'2' & 0xff)) {
    AppendResponseText("No complement of '2' %02x", byte);
    return false;
  }
  
  softSerial.write((uint8_t)0x00);
//  if ((byte = nextByte()) != '0') {
//    AppendResponseText("No echo of 0x00 %02x", byte);
//    return false;
//  }
  AppendResponseText("BS2 version is %02x", nextByte());
  
  return true;
}

#endif
