#ifndef __PROPCONNECTION_H__
#define __PROPCONNECITON_H__

#include <stdint.h>

#define DEF_BAUD_RATE 115200
#define DEF_RESET_PIN 12

class PropellerConnection
{
public:
    PropellerConnection();
    ~PropellerConnection() {}
    int generateResetSignal();
    int sendData(uint8_t *buffer, int size);
    int receiveDataExactTimeout(uint8_t *buffer, int size, int timeout);
    int receiveChecksumAck(int byteCount, int delay);
    int baudRate() { return m_baudRate; }
    int setBaudRate(int baudRate);
    int resetPin() { return m_resetPin; }
    int setResetPin(int pin);
private:
    int m_baudRate;
    int m_resetPin;
};

void AppendResponseText(const char *fmt, ...);

#endif
