#ifndef __PROPLOADER_H__
#define __PROPLOADER_H__

#include <stdint.h>
#include "propconnection.h"

#define INITIAL_BAUD_RATE   115200
#define FINAL_BAUD_RATE     921600

#define DEF_BYTEARRAY_SIZE  8192

class ByteArray;

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
