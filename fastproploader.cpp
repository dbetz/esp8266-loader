#include <string.h>
#include <math.h>

#include <stdio.h>
#include <stdlib.h>
#include "fastproploader.h"

#define FAILSAFE_TIMEOUT        2.0         /* Number of seconds to wait for a packet from the host */
#define MAX_RX_SENSE_ERROR      23          /* Maximum number of cycles by which the detection of a start bit could be off (as affected by the Loader code) */

// Offset (in bytes) from end of Loader Image pointing to where most host-initialized values exist.
// Host-Initialized values are: Initial Bit Time, Final Bit Time, 1.5x Bit Time, Failsafe timeout,
// End of Packet timeout, and ExpectedID.  In addition, the image checksum at word 5 needs to be
// updated.  All these values need to be updated before the download stream is generated.
// NOTE: DAT block data is always placed before the first Spin method
#define RAW_LOADER_INIT_OFFSET_FROM_END (-(10 * 4) - 8)

// Raw loader image.  This is a memory image of a Propeller Application written in PASM that fits into our initial
// download packet.  Once started, it assists with the remainder of the download (at a faster speed and with more
// relaxed interstitial timing conducive of Internet Protocol delivery. This memory image isn't used as-is; before
// download, it is first adjusted to contain special values assigned by this host (communication timing and
// synchronization values) and then is translated into an optimized Propeller Download Stream understandable by the
// Propeller ROM-based boot loader.
#include "IP_Loader.h"

static uint8_t initCallFrame[] = {0xFF, 0xFF, 0xF9, 0xFF, 0xFF, 0xFF, 0xF9, 0xFF};

FastPropellerLoader::FastPropellerLoader(PropellerConnection &connection)
    : m_connection(connection)
{
}

FastPropellerLoader::~FastPropellerLoader()
{
}

int FastPropellerLoader::loadBegin(int imageSize, int initialBaudRate, int finalBaudRate)
{
    PropellerImage loaderImage;
    uint8_t response[8];
    int result, cnt;

    PropellerLoader slowLoader(m_connection);

    /* compute the packet ID (number of packets to be sent) */
    m_packetID = (imageSize + MAX_PACKET_SIZE - 1) / MAX_PACKET_SIZE;

    /* generate a loader packet */
    if (generateInitialLoaderImage(loaderImage, m_packetID, initialBaudRate, finalBaudRate) != 0) {
        AppendResponseText("error: generateInitialLoaderImage failed");
        return -1;
    }
 
    /* load the second-stage loader using the propeller ROM protocol */
    if (slowLoader.load(loaderImage.imageData(), loaderImage.imageSize(), ltDownloadAndRun) != 0) {
        AppendResponseText("error: failed to load second-stage loader");
        return -1;
    }

    /* wait for the second-stage loader to start */
    cnt = m_connection.receiveDataExactTimeout(response, sizeof(response), 2000);
    AppendResponseText("response: %02x %02x %02x %02x %02x %02x %02x %02x", response[0], response[1], response[2], response[3], response[4], response[5], response[6], response[7]); 
    result = getLong(&response[0]);
    if (cnt != 8 || result != m_packetID) {
        AppendResponseText("error: second-stage loader failed to start - cnt %d, packetID %d, result %d", cnt, m_packetID, result);
        return -1;
    }

    /* switch to the final baud rate */
    if (m_connection.setBaudRate(finalBaudRate) != 0) {
        AppendResponseText("error: setting final baud rate failed");
        return -1;
    }

    /* initialize the checksum */
    m_checksum = 0;
    
    /* return successfully */
    return 0;
}

int FastPropellerLoader::loadData(uint8_t *data, int size)
{
    /* transmit the image */
    uint8_t *p = data;
    int remaining = size;
    while (remaining > 0) {
        int cnt, result;
        if ((cnt = remaining) > MAX_PACKET_SIZE)
            cnt = MAX_PACKET_SIZE;
        AppendResponseText("Sending %d byte packet", cnt);
        if (transmitPacket(m_packetID, p, cnt, &result) != 0) {
            AppendResponseText("error: transmitPacket failed");
            return -1;
        }
        if (result != m_packetID - 1) {
            AppendResponseText("error: unexpected result: expected %d, received %d", m_packetID - 1, result);
            return -1;
        }
        remaining -= cnt;
        p += cnt;
        --m_packetID;
    }

    /* update the checksum */
    for (int i = 0; i < size; ++i)
        m_checksum += data[i];

    /* return successfully */
    return 0;
}

int FastPropellerLoader::loadEnd(LoadType loadType)
{
    int result, i;
    
    /* finish computing the image checksum */
    for (i = 0; i < (int)sizeof(initCallFrame); ++i)
        m_checksum += initCallFrame[i];

    /* transmit the RAM verify packet and verify the checksum */
    if (transmitPacket(m_packetID, verifyRAM, sizeof(verifyRAM), &result) != 0) {
        AppendResponseText("error: transmitPacket failed");
        return -1;
    }
    if (result != -m_checksum) {
        AppendResponseText("error: bad checksum");
        return -1;
    }
    m_packetID = -m_checksum;

    /* program the eeprom if requested */
    if (loadType & ltDownloadAndProgram) {
        if (transmitPacket(m_packetID, programVerifyEEPROM, sizeof(programVerifyEEPROM), &result, 8000) != 0) {
            AppendResponseText("error: transmitPacket failed");
            return -1;
        }
        if (result != -m_checksum*2) {
            AppendResponseText("error: bad checksum");
            return -1;
        }
        m_packetID = -m_checksum*2;
    }

    /* transmit the readyToLaunch packet */
    if (transmitPacket(m_packetID, readyToLaunch, sizeof(readyToLaunch), &result) != 0) {
        AppendResponseText("error: transmitPacket failed");
        return -1;
    }
    if (result != m_packetID - 1) {
        AppendResponseText("error: readyToLaunch failed");
        return -1;
    }
    --m_packetID;

    /* transmit the launchNow packet which actually starts the downloaded program */
    if (transmitPacket(0, launchNow, sizeof(launchNow), NULL) != 0) {
        AppendResponseText("error: transmitPacket failedp");
        return -1;
    }

    /* return successfully */
    return 0;
}

int FastPropellerLoader::transmitPacket(int id, uint8_t *payload, int payloadSize, int *pResult, int timeout)
{
    uint8_t hdr[8], response[8];
    int retries, result, cnt;
    int32_t tag;

    /* initialize the packet header */
    setLong(&hdr[0], id);

    /* send the packet */
    retries = 3;
    while (--retries >= 0) {

        /* setup the packet header */
        tag = (int32_t)rand();
        setLong(&hdr[4], tag);
        if (m_connection.sendData(hdr, sizeof(hdr)) != sizeof(hdr)
        ||  m_connection.sendData(payload, payloadSize) != payloadSize) {
            AppendResponseText("error: sendData failed");
            return -1;
        }
        
        /* receive the response */
        if (pResult) {
            cnt = m_connection.receiveDataExactTimeout(response, sizeof(response), timeout);
            AppendResponseText("response: %02x %02x %02x %02x %02x %02x %02x %02x", response[0], response[1], response[2], response[3], response[4], response[5], response[6], response[7]); 
            result = getLong(&response[0]);
            if (cnt == 8 && getLong(&response[4]) == tag && result != id) {
                *pResult = result;
                return 0;
            }
            AppendResponseText("error: transmitPacket failed - cnt %d, tag %d, result %d, id %d", cnt, tag, result, id);
        }

        /* don't wait for a result */
        else
            return 0;
    }

    /* return timeout */
    return -1;
}

double ClockSpeed = 80000000.0;

int FastPropellerLoader::generateInitialLoaderImage(PropellerImage &image, int packetID, int initialBaudRate, int finalBaudRate)
{
    int initAreaOffset = sizeof(rawLoaderImage) + RAW_LOADER_INIT_OFFSET_FROM_END;
 
    // Make an image from the loader template
    image.setImage(rawLoaderImage, sizeof(rawLoaderImage));
 
    // Clock mode
    //image.setLong(initAreaOffset +  0, 0);

    // Initial Bit Time.
    image.setLong(initAreaOffset +  4, (int)trunc(80000000.0 / initialBaudRate + 0.5));

    // Final Bit Time.
    image.setLong(initAreaOffset +  8, (int)trunc(80000000.0 / finalBaudRate + 0.5));

    // 1.5x Final Bit Time minus maximum start bit sense error.
    image.setLong(initAreaOffset + 12, (int)trunc(1.5 * ClockSpeed / finalBaudRate - MAX_RX_SENSE_ERROR + 0.5));

    // Failsafe Timeout (seconds-worth of Loader's Receive loop iterations).
    image.setLong(initAreaOffset + 16, (int)trunc(FAILSAFE_TIMEOUT * ClockSpeed / (3 * 4) + 0.5));

    // EndOfPacket Timeout (2 bytes worth of Loader's Receive loop iterations).
    image.setLong(initAreaOffset + 20, (int)trunc((2.0 * ClockSpeed / finalBaudRate) * (10.0 / 12.0) + 0.5));

    // PatchLoaderLongValue(RawSize*4+RawLoaderInitOffset + 24, Max(Round(ClockSpeed * SSSHTime), 14));
    // PatchLoaderLongValue(RawSize*4+RawLoaderInitOffset + 28, Max(Round(ClockSpeed * SCLHighTime), 14));
    // PatchLoaderLongValue(RawSize*4+RawLoaderInitOffset + 32, Max(Round(ClockSpeed * SCLLowTime), 26));

    // Minimum EEPROM Start/Stop Condition setup/hold time (400 KHz = 1/0.6 µS); Minimum 14 cycles
    //image.setLong(initAreaOffset + 24, 14);

    // Minimum EEPROM SCL high time (400 KHz = 1/0.6 µS); Minimum 14 cycles
    //image.setLong(initAreaOffset + 28, 14);

    // Minimum EEPROM SCL low time (400 KHz = 1/1.3 µS); Minimum 26 cycles
    //image.setLong(initAreaOffset + 32, 26);

    // First Expected Packet ID; total packet count.
    image.setLong(initAreaOffset + 36, packetID);

    // Recalculate and update checksum so low byte of checksum calculates to 0.
    image.updateChecksum();

    /* return successfully */
    return 0;
}

int32_t FastPropellerLoader::getLong(const uint8_t *buf)
{
     return (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
}

void FastPropellerLoader::setLong(uint8_t *buf, uint32_t value)
{
     buf[3] = value >> 24;
     buf[2] = value >> 16;
     buf[1] = value >>  8;
     buf[0] = value;
}
