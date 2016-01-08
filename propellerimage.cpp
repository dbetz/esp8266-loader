#include "propellerimage.h"

#define OFFSET_OF(_s, _f) ((int)&((_s *)0)->_f)

PropellerImage::PropellerImage()
  : m_imageData(0), m_imageSize(0)
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
    return getLong(OFFSET_OF(SpinHdr, clkfreq));
}

void PropellerImage::setClkFreq(uint32_t clkFreq)
{
    setLong(OFFSET_OF(SpinHdr, clkfreq), clkFreq);
}

uint8_t PropellerImage::clkMode()
{
    return getByte(OFFSET_OF(SpinHdr, clkmode));
}

void PropellerImage::setClkMode(uint8_t clkMode)
{
    setByte(OFFSET_OF(SpinHdr, clkmode), clkMode);
}

uint8_t PropellerImage::updateChecksum()
{
    SpinHdr *spinHdr = (SpinHdr *)m_imageData;
    uint8_t *p = m_imageData;
    int chksum, cnt;
    spinHdr->chksum = chksum = 0;
    for (cnt = m_imageSize; --cnt >= 0; )
        chksum += *p++;
    spinHdr->chksum = SPIN_TARGET_CHECKSUM - chksum;
    return chksum & 0xff;
}

uint8_t PropellerImage::getByte(int offset)
{
     uint8_t *buf = m_imageData + offset;
     return buf[0];
}

void PropellerImage::setByte(int offset, uint8_t value)
{
     uint8_t *buf = m_imageData + offset;
     buf[0] = value;
}

uint16_t PropellerImage::getWord(int offset)
{
     uint8_t *buf = m_imageData + offset;
     return (buf[1] << 8) | buf[0];
}

void PropellerImage::setWord(int offset, uint16_t value)
{
     uint8_t *buf = m_imageData + offset;
     buf[1] = value >>  8;
     buf[0] = value;
}

uint32_t PropellerImage::getLong(int offset)
{
     uint8_t *buf = m_imageData + offset;
     return (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
}

void PropellerImage::setLong(int offset, uint32_t value)
{
     uint8_t *buf = m_imageData + offset;
     buf[3] = value >> 24;
     buf[2] = value >> 16;
     buf[1] = value >>  8;
     buf[0] = value;
}
