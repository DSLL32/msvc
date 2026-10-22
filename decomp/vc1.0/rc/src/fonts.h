#ifndef RC_FONTS_H
#define RC_FONTS_H
#include "resource.h"
#include <recon.h>
/* Only the file-position fields are interpreted by RC. The preceding metrics
 * and remaining header field are copied unchanged into its font directory. */
struct FontDirectoryHeader {
    BYTE metrics[136];
    DWORD device_offset, face_offset, reserved;
};
struct FontDirectoryEntry {
    WORD ordinal, length;
    struct FontDirectoryEntry *next;
    BYTE directory[1];
};
extern struct FontDirectoryEntry *first_font, *last_font;
#endif
