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

#endif
