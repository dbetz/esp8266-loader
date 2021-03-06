#ifndef FASTPROPLOADER_H
#define FASTPROPLOADER_H

#include <stdint.h>

#include "propimage.h"
#include "proploader.h"

// size of data buffer in the second-stage loader
#define MAX_PACKET_SIZE     1024

class FastPropellerLoader
{
public:
    FastPropellerLoader(PropellerConnection &connection);
    ~FastPropellerLoader();
    int loadBegin(int imageSize, int initialBaudRate = INITIAL_BAUD_RATE, int finalBaudRate = FINAL_BAUD_RATE);
    int loadData(uint8_t *data, int size);
    int loadEnd(LoadType loadType);

private:
    int transmitPacket(int id, uint8_t *payload, int payloadSize, int *pResult, int timeout = 2000);
    int generateInitialLoaderImage(PropellerImage &image, int packetID, int initialBaudRate, int finalBaudRate);

    static int32_t getLong(const uint8_t *buf);
    static void setLong(uint8_t *buf, uint32_t value);

    PropellerConnection &m_connection;
    int32_t m_packetID;
    int32_t m_checksum;
};

#endif // FASTPROPELLERLOADER_H
