#ifndef __PROPLOADER_H__
#define __PROPLOADER_H__

#include <stdint.h>

#define INITIAL_BAUD_RATE   115200
#define FINAL_BAUD_RATE     921600

#define PROPELLER_RESET_PIN 2

#define MAX_PACKET_SIZE     1024

// number of milliseconds between attempts to read the checksum ack
#define CALIBRATE_PAUSE     10

class ByteArray {
public:
    ByteArray(int maxSize = MAX_PACKET_SIZE);
    ~ByteArray();
    void clear() { m_size = 0; }
    bool append(uint8_t *data, int size);
    bool append(int data);
    uint8_t *data() { return m_data; }
    int maxSize() { return m_maxSize; }
    int size() { return m_size; }
    void setSize(int size) { m_size = size; }
private:
    uint8_t *m_data;
    int m_maxSize;
    int m_size;
};

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
    virtual int maxDataSize() { return MAX_PACKET_SIZE; }
private:
    int m_baudRate;
};

enum LoadType {
    ltNone = 0,
    ltDownloadAndRun = (1 << 0),
    ltDownloadAndProgram = (1 << 1),
    ltDownloadAndProgramAndRun = ltDownloadAndRun | ltDownloadAndProgram
};

class PropellerLoader
{
public:
    PropellerLoader(PropellerConnection &connection);
    ~PropellerLoader();
    int load(uint8_t *image, int imageSize, LoadType loadType = ltDownloadAndRun);

private:
    static void generateIdentifyPacket(ByteArray &packet);
    static int generateLoaderPacket(ByteArray &packet, const uint8_t *image, int imageSize, LoadType loadType);
    static void encodeBytes(ByteArray &packet, const uint8_t *inBytes, int inCount);

    PropellerConnection &m_connection;
};

#endif
