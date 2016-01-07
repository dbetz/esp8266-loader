#include "Arduino.h"
#include "proploader.h"

/////////////////////
// PropellerLoader //
/////////////////////

#define LENGTH_FIELD_SIZE       11          /* number of bytes in the length field */

// Propeller Download Stream Translator array.  Index into this array using the "Binary Value" (usually 5 bits) to translate,
// the incoming bit size (again, usually 5), and the desired data element to retrieve (encoding = translation, bitCount = bit count
// actually translated.

// first index is the next 1-5 bits from the incoming bit stream
// second index is the number of bits in the first value
// the result is a structure containing the byte to output to encode some or all of the input bits
static struct {
    uint8_t encoding;   // encoded byte to output
    uint8_t bitCount;   // number of bits encoded by the output byte
} PDSTx[32][5] =

//  ***  1-BIT  ***        ***  2-BIT  ***        ***  3-BIT  ***        ***  4-BIT  ***        ***  5-BIT  ***
{ { /*%00000*/ {0xFE, 1},  /*%00000*/ {0xF2, 2},  /*%00000*/ {0x92, 3},  /*%00000*/ {0x92, 3},  /*%00000*/ {0x92, 3} },
  { /*%00001*/ {0xFF, 1},  /*%00001*/ {0xF9, 2},  /*%00001*/ {0xC9, 3},  /*%00001*/ {0xC9, 3},  /*%00001*/ {0xC9, 3} },
  {            {0,    0},  /*%00010*/ {0xFA, 2},  /*%00010*/ {0xCA, 3},  /*%00010*/ {0xCA, 3},  /*%00010*/ {0xCA, 3} },
  {            {0,    0},  /*%00011*/ {0xFD, 2},  /*%00011*/ {0xE5, 3},  /*%00011*/ {0x25, 4},  /*%00011*/ {0x25, 4} },
  {            {0,    0},             {0,    0},  /*%00100*/ {0xD2, 3},  /*%00100*/ {0xD2, 3},  /*%00100*/ {0xD2, 3} },
  {            {0,    0},             {0,    0},  /*%00101*/ {0xE9, 3},  /*%00101*/ {0x29, 4},  /*%00101*/ {0x29, 4} },
  {            {0,    0},             {0,    0},  /*%00110*/ {0xEA, 3},  /*%00110*/ {0x2A, 4},  /*%00110*/ {0x2A, 4} },
  {            {0,    0},             {0,    0},  /*%00111*/ {0xFA, 3},  /*%00111*/ {0x95, 4},  /*%00111*/ {0x95, 4} },
  {            {0,    0},             {0,    0},             {0,    0},  /*%01000*/ {0x92, 3},  /*%01000*/ {0x92, 3} },
  {            {0,    0},             {0,    0},             {0,    0},  /*%01001*/ {0x49, 4},  /*%01001*/ {0x49, 4} },
  {            {0,    0},             {0,    0},             {0,    0},  /*%01010*/ {0x4A, 4},  /*%01010*/ {0x4A, 4} },
  {            {0,    0},             {0,    0},             {0,    0},  /*%01011*/ {0xA5, 4},  /*%01011*/ {0xA5, 4} },
  {            {0,    0},             {0,    0},             {0,    0},  /*%01100*/ {0x52, 4},  /*%01100*/ {0x52, 4} },
  {            {0,    0},             {0,    0},             {0,    0},  /*%01101*/ {0xA9, 4},  /*%01101*/ {0xA9, 4} },
  {            {0,    0},             {0,    0},             {0,    0},  /*%01110*/ {0xAA, 4},  /*%01110*/ {0xAA, 4} },
  {            {0,    0},             {0,    0},             {0,    0},  /*%01111*/ {0xD5, 4},  /*%01111*/ {0xD5, 4} },
  {            {0,    0},             {0,    0},             {0,    0},             {0,    0},  /*%10000*/ {0x92, 3} },
  {            {0,    0},             {0,    0},             {0,    0},             {0,    0},  /*%10001*/ {0xC9, 3} },
  {            {0,    0},             {0,    0},             {0,    0},             {0,    0},  /*%10010*/ {0xCA, 3} },
  {            {0,    0},             {0,    0},             {0,    0},             {0,    0},  /*%10011*/ {0x25, 4} },
  {            {0,    0},             {0,    0},             {0,    0},             {0,    0},  /*%10100*/ {0xD2, 3} },
  {            {0,    0},             {0,    0},             {0,    0},             {0,    0},  /*%10101*/ {0x29, 4} },
  {            {0,    0},             {0,    0},             {0,    0},             {0,    0},  /*%10110*/ {0x2A, 4} },
  {            {0,    0},             {0,    0},             {0,    0},             {0,    0},  /*%10111*/ {0x95, 4} },
  {            {0,    0},             {0,    0},             {0,    0},             {0,    0},  /*%11000*/ {0x92, 3} },
  {            {0,    0},             {0,    0},             {0,    0},             {0,    0},  /*%11001*/ {0x49, 4} },
  {            {0,    0},             {0,    0},             {0,    0},             {0,    0},  /*%11010*/ {0x4A, 4} },
  {            {0,    0},             {0,    0},             {0,    0},             {0,    0},  /*%11011*/ {0xA5, 4} },
  {            {0,    0},             {0,    0},             {0,    0},             {0,    0},  /*%11100*/ {0x52, 4} },
  {            {0,    0},             {0,    0},             {0,    0},             {0,    0},  /*%11101*/ {0xA9, 4} },
  {            {0,    0},             {0,    0},             {0,    0},             {0,    0},  /*%11110*/ {0xAA, 4} },
  {            {0,    0},             {0,    0},             {0,    0},             {0,    0},  /*%11111*/ {0x55, 5} }
 };

// After reset, the Propeller's exact clock rate is not known by either the host or the Propeller itself, so communication
// with the Propeller takes place based on a host-transmitted timing template that the Propeller uses to read the stream
// and generate the responses.  The host first transmits the 2-bit timing template, then transmits a 250-bit Tx handshake,
// followed by 250 timing templates (one for each Rx handshake bit expected) which the Propeller uses to properly transmit
// the Rx handshake sequence.  Finally, the host transmits another eight timing templates (one for each bit of the
// Propeller's version number expected) which the Propeller uses to properly transmit it's 8-bit hardware/firmware version
// number.
//
// After the Tx Handshake and Rx Handshake are properly exchanged, the host and Propeller are considered "connected," at
// which point the host can send a download command followed by image size and image data, or simply end the communication.
//
// PROPELLER HANDSHAKE SEQUENCE: The handshake (both Tx and Rx) are based on a Linear Feedback Shift Register (LFSR) tap
// sequence that repeats only after 255 iterations.  The generating LFSR can be created in Pascal code as the following function
// (assuming FLFSR is pre-defined Byte variable that is set to ord('P') prior to the first call of IterateLFSR).  This is
// the exact function that was used in previous versions of the Propeller Tool and Propellent software.
//
//      function IterateLFSR: Byte;
//      begin //Iterate LFSR, return previous bit 0
//      Result := FLFSR and 0x01;
//      FLFSR := FLFSR shl 1 and 0xFE or (FLFSR shr 7 xor FLFSR shr 5 xor FLFSR shr 4 xor FLFSR shr 1) and 1;
//      end;
//
// The handshake bit stream consists of the lowest bit value of each 8-bit result of the LFSR described above.  This LFSR
// has a domain of 255 combinations, but the host only transmits the first 250 bits of the pattern, afterwards, the Propeller
// generates and transmits the next 250-bits based on continuing with the same LFSR sequence.  In this way, the host-
// transmitted (host-generated) stream ends 5 bits before the LFSR starts repeating the initial sequence, and the host-
// received (Propeller generated) stream that follows begins with those remaining 5 bits and ends with the leading 245 bits
// of the host-transmitted stream.
//
// For speed and compression reasons, this handshake stream has been encoded as tightly as possible into the pattern
// described below.
//
// The TxHandshake array consists of 209 bytes that are encoded to represent the required '1' and '0' timing template bits,
// 250 bits representing the lowest bit values of 250 iterations of the Propeller LFSR (seeded with ASCII 'P'), 250 more
// timing template bits to receive the Propeller's handshake response, and more to receive the version.
static uint8_t txHandshake[] = {
    // First timing template ('1' and '0') plus first two bits of handshake ('0' and '1').
    0x49,
    // Remaining 248 bits of handshake...
    0xAA,0x52,0xA5,0xAA,0x25,0xAA,0xD2,0xCA,0x52,0x25,0xD2,0xD2,0xD2,0xAA,0x49,0x92,
    0xC9,0x2A,0xA5,0x25,0x4A,0x49,0x49,0x2A,0x25,0x49,0xA5,0x4A,0xAA,0x2A,0xA9,0xCA,
    0xAA,0x55,0x52,0xAA,0xA9,0x29,0x92,0x92,0x29,0x25,0x2A,0xAA,0x92,0x92,0x55,0xCA,
    0x4A,0xCA,0xCA,0x92,0xCA,0x92,0x95,0x55,0xA9,0x92,0x2A,0xD2,0x52,0x92,0x52,0xCA,
    0xD2,0xCA,0x2A,0xFF,
    // 250 timing templates ('1' and '0') to receive 250-bit handshake from Propeller.
    // This is encoded as two pairs per byte; 125 bytes.
    0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,
    0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,
    0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,
    0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,
    0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,
    0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,
    0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,
    0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,
    // 8 timing templates ('1' and '0') to receive 8-bit Propeller version; two pairs per byte; 4 bytes.
    0x29,0x29,0x29,0x29};

// Shutdown command (0); 11 bytes.
static uint8_t shutdownCmd[] = {0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0xf2};

// Load RAM and Run command (1); 11 bytes.
static uint8_t loadRunCmd[] = {0xc9, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0xf2};

// Load RAM, Program EEPROM, and Shutdown command (2); 11 bytes.
static uint8_t programShutdownCmd[] = {0xca, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0xf2};

// Load RAM, Program EEPROM, and Run command (3); 11 bytes.
static uint8_t programRunCmd[] = {0x25, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0xfe};

// The RxHandshake array consists of 125 bytes encoded to represent the expected 250-bit (125-byte @ 2 bits/byte) response
// of continuing-LFSR stream bits from the Propeller, prompted by the timing templates following the TxHandshake stream.
static uint8_t rxHandshake[] = {
    0xEE,0xCE,0xCE,0xCF,0xEF,0xCF,0xEE,0xEF,0xCF,0xCF,0xEF,0xEF,0xCF,0xCE,0xEF,0xCF,
    0xEE,0xEE,0xCE,0xEE,0xEF,0xCF,0xCE,0xEE,0xCE,0xCF,0xEE,0xEE,0xEF,0xCF,0xEE,0xCE,
    0xEE,0xCE,0xEE,0xCF,0xEF,0xEE,0xEF,0xCE,0xEE,0xEE,0xCF,0xEE,0xCF,0xEE,0xEE,0xCF,
    0xEF,0xCE,0xCF,0xEE,0xEF,0xEE,0xEE,0xEE,0xEE,0xEF,0xEE,0xCF,0xCF,0xEF,0xEE,0xCE,
    0xEF,0xEF,0xEF,0xEF,0xCE,0xEF,0xEE,0xEF,0xCF,0xEF,0xCF,0xCF,0xCE,0xCE,0xCE,0xCF,
    0xCF,0xEF,0xCE,0xEE,0xCF,0xEE,0xEF,0xCE,0xCE,0xCE,0xEF,0xEF,0xCF,0xCF,0xEE,0xEE,
    0xEE,0xCE,0xCF,0xCE,0xCE,0xCF,0xCE,0xEE,0xEF,0xEE,0xEF,0xEF,0xCF,0xEF,0xCE,0xCE,
    0xEF,0xCE,0xEE,0xCE,0xEF,0xCE,0xCE,0xEE,0xCF,0xCF,0xCE,0xCF,0xCF};

PropellerLoader::PropellerLoader(PropellerConnection &connection)
    : m_connection(connection)
{
}

PropellerLoader::~PropellerLoader()
{
}

int PropellerLoader::load(uint8_t *image, int imageSize, LoadType loadType)
{
    ByteArray packet;

    AppendError("info: imageSize %d", imageSize);
    /* generate a single packet containing the tx handshake and the image to load */
    if (generateLoaderPacket(packet, image, imageSize, loadType) != 0)
        return -1;

    /* reset the Propeller */
    if (m_connection.generateResetSignal() != 0) {
        AppendError("error: generateResetSignal failed");
        return -1;
    }

    /* send the packet */
    if (m_connection.sendData(packet.data(), packet.size()) != 0) {
        AppendError("error: sendData failed");
        return -1;
    }

    /* receive the handshake response and the hardware version */
    int cnt = sizeof(rxHandshake) + 4;
    if (m_connection.receiveDataExactTimeout(packet.data(), cnt, 2000) != cnt) {
        AppendError("error: receiveDataExactTimeout failed");
        return -1;
    }

    /* verify the rx handshake */
    uint8_t *buf = packet.data();
    if (memcmp(buf, rxHandshake, sizeof(rxHandshake)) != 0) {
        AppendError("error: handshake failed");
        return -1;
    }

    /* verify the hardware version */
    int version = 0;
    for (int i = sizeof(rxHandshake); i < cnt; ++i)
        version = ((version >> 2) & 0x3F) | ((buf[i] & 0x01) << 6) | ((buf[i] & 0x20) << 2);
    if (version != 1) {
        AppendError("error: wrong propeller version");
        return -1;
    }

    /* receive and verify the checksum */
    if (m_connection.receiveChecksumAck(packet.size(), 250) != 0) {
        AppendError("error: checksum verification failed");
        return -1;
    }

    /* wait for eeprom programming and verification */
    if (loadType == ltDownloadAndProgram || loadType == ltDownloadAndProgramAndRun) {

        /* wait for an ACK indicating a successful EEPROM programming */
        if (m_connection.receiveChecksumAck(0, 5000) != 0) {
            AppendError("error: EEPROM programming failed");
            return -1;
        }

        /* wait for an ACK indicating a successful EEPROM verification */
        if (m_connection.receiveChecksumAck(0, 2000) != 0) {
            AppendError("error: EEPROM verification failed");
            return -1;
        }
    }

    /* return successfully */
    return 0;
}

void PropellerLoader::generateIdentifyPacket(ByteArray &packet)
{
    /* initialize the packet */
    packet.clear();

    /* copy the handshake image and the shutdown command to the packet */
    packet.append(txHandshake, sizeof(txHandshake));
    packet.append(shutdownCmd, sizeof(shutdownCmd));
}

int PropellerLoader::generateLoaderPacket(ByteArray &packet, const uint8_t *image, int imageSize, LoadType loadType)
{
    int imageSizeInLongs = (imageSize + 3) / 4;
    int tmp, i;

    /* initialize the packet */
    packet.clear();

    /* copy the handshake image and the command to the packet */
    packet.append(txHandshake, sizeof(txHandshake));

    /* append the loader command */
    switch (loadType) {
    case ltDownloadAndRun:
        packet.append(loadRunCmd, sizeof(loadRunCmd));
        break;
    case ltDownloadAndProgram:
        packet.append(programShutdownCmd, sizeof(loadRunCmd));
        break;
    case ltDownloadAndProgramAndRun:
        packet.append(programRunCmd, sizeof(loadRunCmd));
        break;
    default:
        return -1;
    }

    /* add the image size in longs */
    tmp = imageSizeInLongs;
    for (i = 0; i < LENGTH_FIELD_SIZE; ++i) {
        packet.append(0x92 | (i == 10 ? 0x60 : 0x00) | (tmp & 1) | ((tmp & 2) << 2) | ((tmp & 4) << 4));
        tmp >>= 3;
    }

    /* encode the image and insert it into the packet */
    encodeBytes(packet, image, imageSize);

    /* return successfully */
    return 0;
}

/* encodeBytes
    parameters:
        packet is a packet to receive the encoded bytes
        inBytes is a pointer to a buffer of bytes to be encoded
        inCount is the number of bytes in inBytes
    returns the number of bytes written to the outBytes buffer or -1 if the encoded data does not fit
*/
void PropellerLoader::encodeBytes(ByteArray &packet, const uint8_t *inBytes, int inCount)
{
    static uint8_t masks[] = { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f };
    int bitCount = inCount * 8;
    int nextBit = 0;

    /* encode all bits in the input buffer */
    while (nextBit < bitCount) {
        int bits, bitsIn;

        /* encode 5 bits or whatever remains in inBytes, whichever is smaller */
        bitsIn = bitCount - nextBit;
        if (bitsIn > 5)
            bitsIn = 5;

        /* extract the next 'bitsIn' bits from the input buffer */
        bits = ((inBytes[nextBit / 8] >> (nextBit % 8)) | (inBytes[nextBit / 8 + 1] << (8 - (nextBit % 8)))) & masks[bitsIn];

        /* store the encoded value */
        packet.append(PDSTx[bits][bitsIn - 1].encoding);

        /* advance to the next group of bits */
        nextBit += PDSTx[bits][bitsIn - 1].bitCount;
    }
}

/////////////////////////
// PropellerConnection //
/////////////////////////

PropellerConnection::PropellerConnection()
  : m_baudRate(INITIAL_BAUD_RATE)
{
    pinMode(PROPELLER_RESET_PIN, OUTPUT);
    digitalWrite(PROPELLER_RESET_PIN, HIGH);
}

int PropellerConnection::generateResetSignal()
{
    Serial.flush();
    delay(10);
    digitalWrite(PROPELLER_RESET_PIN, LOW);
    delay(10);
    digitalWrite(PROPELLER_RESET_PIN, HIGH);
    delay(100);
    while (Serial.available())
      Serial.read();
    return 0;
}

int PropellerConnection::sendData(uint8_t *buf, int len)
{
    return Serial.write(buf, len) == len ? 0 : -1;
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
    int retries = (msSendTime / CALIBRATE_PAUSE) | (delay / CALIBRATE_PAUSE) + 20;
    uint8_t buf[1];

    do {
        Serial.write(calibrate, sizeof(calibrate));
        if (receiveDataExactTimeout(buf, 1, CALIBRATE_PAUSE) == 1) {
            AppendError("info: received ack %02x", buf[0]);
            return buf[0] == 0xFE ? 0 : -1;
        }
    } while (--retries > 0);

    AppendError("error: timeout waiting for checksum ack");
    return -1;
}

int PropellerConnection::setBaudRate(int baudRate)
{
    Serial.begin(baudRate);
    return 0;
}

///////////////
// ByteArray //
///////////////

ByteArray::ByteArray(int maxSize)
  : m_data(NULL), m_size(0), m_maxSize(maxSize)
{
    m_data = (uint8_t *)malloc(maxSize);
    if (!m_data) {
        Serial.println("error: out of memory");
        for (;;)
            ;
    }
}

ByteArray::~ByteArray()
{
    if (m_data)
        free(m_data);
}

bool ByteArray::append(uint8_t *data, int size)
{
    if (!m_data || m_size + size > m_maxSize)
        return false;
    memcpy(&m_data[m_size], data, size);
    m_size += size;
    return true;
}

bool ByteArray::append(int data)
{
    if (!m_data || m_size + 1 > m_maxSize)
        return false;
    m_data[m_size++] = data;
    return true;
}


