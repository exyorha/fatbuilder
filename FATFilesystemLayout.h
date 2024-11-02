#ifndef FAT_FILESYSTEM_LAYOUT_H
#define FAT_FILESYSTEM_LAYOUT_H

#include <cstring>

struct FATFilesystemLayout {
    const unsigned char *mbrCode = m_mbrCode;
    const unsigned char *pbrCode12_16 = m_pbrCode_12_16;
    const unsigned char *pbrCode32 = m_pbrCode_32;
private:

    static const unsigned char m_mbrCode[512];
    static const unsigned char m_pbrCode_12_16[512];
    static const unsigned char m_pbrCode_32[1536];

public:
    static constexpr size_t MBRCodeSize = sizeof(m_mbrCode);
    static constexpr size_t PBRCode12_16Size = sizeof(m_pbrCode_12_16);
    static constexpr size_t PBRCode32Size = sizeof(m_pbrCode_32);
};

#endif
