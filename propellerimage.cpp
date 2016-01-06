#include "propellerimage.h"

PropellerImage::PropellerImage()
{
}

PropellerImage::PropellerImage(uint8_t *imageData, int imageSize)
{
    setImage(imageData, imageSize);
}

PropellerImage::~PropellerImage()
{
}

void PropellerImage::setImage(uint8_t *imageData, int imageSize)
{
    m_imageData = imageData;
    m_imageSize = imageSize;
}

uint32_t PropellerImage::clkFreq()
{
    return getLong((uint8_t *)&((SpinHdr *)m_imageData)->clkfreq);
}

void PropellerImage::setClkFreq(uint32_t clkFreq)
{
    setLong((uint8_t *)&((SpinHdr *)m_imageData)->clkfreq, clkFreq);
}

uint8_t PropellerImage::clkMode()
{
    return *(uint8_t *)&((SpinHdr *)m_imageData)->clkfreq;
}

void PropellerImage::setClkMode(uint8_t clkMode)
{
    *(uint8_t *)&((SpinHdr *)m_imageData)->clkfreq = clkMode;
}

void PropellerImage::updateChecksum()
{
    SpinHdr *spinHdr = (SpinHdr *)m_imageData;
    uint8_t *p = m_imageData;
    int chksum, cnt;
    spinHdr->chksum = chksum = 0;
    for (cnt = m_imageSize; --cnt >= 0; )
        chksum += *p++;
    spinHdr->chksum = SPIN_TARGET_CHECKSUM - chksum;
}

uint16_t PropellerImage::getWord(const uint8_t *buf)
{
     return (buf[1] << 8) | buf[0];
}

void PropellerImage::setWord(uint8_t *buf, uint16_t value)
{
     buf[1] = value >>  8;
     buf[0] = value;
}

uint32_t PropellerImage::getLong(const uint8_t *buf)
{
     return (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
}

void PropellerImage::setLong(uint8_t *buf, uint32_t value)
{
     buf[3] = value >> 24;
     buf[2] = value >> 16;
     buf[1] = value >>  8;
     buf[0] = value;
}
