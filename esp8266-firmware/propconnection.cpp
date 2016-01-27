#include <Arduino.h>
#include "propconnection.h"

// number of milliseconds between attempts to read the checksum ack
#define CALIBRATE_PAUSE     10

PropellerConnection::PropellerConnection()
    : m_baudRate(-1), m_resetPin(-1)
{
}

int PropellerConnection::generateResetSignal()
{
    if (m_resetPin == -1)
        return -1;
    Serial.flush();
    delay(10);
    digitalWrite(m_resetPin, LOW);
    delay(10);
    digitalWrite(m_resetPin, HIGH);
    delay(100);
    while (Serial.available())
        Serial.read();
    return 0;
}

int PropellerConnection::sendData(uint8_t *buf, int len)
{
    return Serial.write(buf, len) == len ? len : -1;
}

int PropellerConnection::receiveDataExactTimeout(uint8_t *buf, int len, int timeout)
{
    int remaining = len;

    /* return only when the buffer contains the exact amount of data requested */
    while (remaining > 0) {
        int cnt;

        /* read the next bit of data */
        Serial.setTimeout(timeout);
        if ((cnt = (int)Serial.readBytes(buf, remaining)) <= 0)
            return -1;

        /* update the buffer pointer */
        remaining -= cnt;
        buf += cnt;
    }

    /* return the full size of the buffer */
    return len;
}

int PropellerConnection::receiveChecksumAck(int byteCount, int delay)
{
    static uint8_t calibrate[1] = { 0xF9 };
    int msSendTime = (byteCount * 10 * 1000) / m_baudRate;
    int retries = (msSendTime / CALIBRATE_PAUSE) + (delay / CALIBRATE_PAUSE);
    uint8_t buf[1];

    do {
        Serial.write(calibrate, sizeof(calibrate));
        if (receiveDataExactTimeout(buf, 1, CALIBRATE_PAUSE) == 1)
            return buf[0] == 0xFE ? 0 : -1;
    } while (--retries > 0);

    AppendResponseText("error: timeout waiting for checksum ack");
    return -1;
}

int PropellerConnection::setBaudRate(int baudRate)
{
    if (baudRate != m_baudRate) {
        if (m_baudRate != -1)
          Serial.end();
        if ((m_baudRate = baudRate) != -1)
          Serial.begin(m_baudRate);
    }
    return 0;
}

int PropellerConnection::setResetPin(int resetPin)
{
    if (resetPin != m_resetPin) {
        if (m_resetPin != -1)
            pinMode(m_resetPin, INPUT);
        if ((m_resetPin = resetPin) != -1) {
            pinMode(m_resetPin, OUTPUT);
            digitalWrite(m_resetPin, HIGH);
        }
    }
    return 0;
}

