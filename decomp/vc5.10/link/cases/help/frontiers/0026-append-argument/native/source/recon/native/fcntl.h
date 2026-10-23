#ifndef RECON_NATIVE_FCNTL_H
#define RECON_NATIVE_FCNTL_H
/* Keep host O_* values available to shim implementations. Reconstructed CRT
 * callers use the distinct Microsoft _O_* constants below. */
#include_next <fcntl.h>
#define _O_RDONLY 0x0000
#define _O_WRONLY 0x0001
#define _O_RDWR 0x0002
#define _O_APPEND 0x0008
#define _O_RANDOM 0x0010
#define _O_SEQUENTIAL 0x0020
#define _O_TEMPORARY 0x0040
#define _O_NOINHERIT 0x0080
#define _O_CREAT 0x0100
#define _O_TRUNC 0x0200
#define _O_EXCL 0x0400
#define _O_SHORT_LIVED 0x1000
#define _O_TEXT 0x4000
#define _O_BINARY 0x8000
#define _O_RAW _O_BINARY
#endif
