#ifndef PROPELLERIMAGE_H
#define PROPELLERIMAGE_H

#include <stdint.h>

/* target checksum for a binary file */
#define SPIN_TARGET_CHECKSUM    0x14

/* spin object file header */
typedef struct {
    uint32_t clkfreq;
    uint8_t clkmode;
    uint8_t chksum;
    uint16_t pbase;
    uint16_t vbase;
    uint16_t dbase;
    uint16_t pcurr;
    uint16_t dcurr;
} SpinHdr;

class PropellerImage
{
public:
    PropellerImage();
    PropellerImage(uint8_t *imageData, int imageSize);
    ~PropellerImage();
    void setImage(uint8_t *imageData, int imageSize);
    uint8_t updateChecksum();
    uint8_t *imageData() { return m_imageData; }
    int imageSize() { return m_imageSize; }
    uint32_t clkFreq();
    void setClkFreq(uint32_t clkFreq);
    uint8_t clkMode();
    void setClkMode(uint8_t clkMode);
    uint8_t getByte(int offset);
    void setByte(int offset, uint8_t value);
    uint16_t getWord(int offset);
    void setWord(int offset, uint16_t value);
    uint32_t getLong(int offset);
    void setLong(int offset, uint32_t value);

private:

    uint8_t *m_imageData;
    int m_imageSize;
};

#endif // PROPELLERIMAGE_H
