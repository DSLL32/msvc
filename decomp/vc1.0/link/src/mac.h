#ifndef LINK_MAC_H
#define LINK_MAC_H
#include <recon.h>

#pragma pack(2)
typedef struct MacSectionResource {
    unsigned char type[4];
    unsigned short id;
} MacSectionResource;
#pragma pack()
typedef char MacSectionResourceIs6[sizeof(MacSectionResource)==6?1:-1];
extern MacSectionResource *mac_section_resources;

#define MAC_BIG_LONG(x) (((unsigned long)(x)>>24)|(((unsigned long)(x)&0xff0000)>>8)| \
    (((unsigned long)(x)&0xff00)<<8)|((unsigned long)(x)<<24))
#define MAC_BIG_SHORT(x) ((unsigned short)(((unsigned short)(x)>>8)|((unsigned short)(x)<<8)))

#endif
