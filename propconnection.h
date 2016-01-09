#ifndef __PROPCONNECTION_H__
#define __PROPCONNECITON_H__

#include <stdint.h>

#define DEF_BAUD_RATE 115200

class PropellerConnection
{
public:
    PropellerConnection();
    ~PropellerConnection() {}
    virtual int generateResetSignal();
    virtual int sendData(uint8_t *buffer, int size);
    virtual int receiveDataExactTimeout(uint8_t *buffer, int size, int timeout);
    virtual int receiveChecksumAck(int byteCount, int delay);
    virtual int setBaudRate(int baudRate);
private:
    int m_baudRate;
};

void AppendResponseText(const char *fmt, ...);

#endif
