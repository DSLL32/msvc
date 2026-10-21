/* Serialized default MZ stub used when no /STUB image is supplied.
 * The 0x80 PE-signature offset and fixed message capacity are file-format
 * fields; this object contains no adjacent LINK data or section padding.
 */
#include "dos-stub.h"
DosStub default_dos_stub = {
    {0x5a4d,0x90,3,0,4,0,0xffff,0,0xb8,0,0,0,0x40,0,{0},0,0,{0},0x80},
    {0x0e,0x1f,0xba,14,0xb4,9,0xcd,0x21,0xb8,0x4c01,0xcd,0x21},
    "This program cannot be run in DOS mode.\r\r\n$", 0x4550
};
char *dos_stub_template = (char *)&default_dos_stub;
unsigned long dos_stub_size = sizeof(DosStub);

/* Macintosh output uses the same MZ shell with its own diagnostic. */
DosStub mac_dos_stub = {
    {0x5a4d,0x90,3,0,4,0,0xffff,0,0xb8,0,0,0,0x40,0,{0},0,0,{0},0x80},
    {0x0e,0x1f,0xba,14,0xb4,9,0xcd,0x21,0xb8,0x4c01,0xcd,0x21},
    "This program is a Macintosh EXE.\r\r\n$", 0x4550
};
