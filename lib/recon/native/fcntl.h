#ifndef RECON_NATIVE_FCNTL_H
#define RECON_NATIVE_FCNTL_H
/* Keep host O_* values available to shim implementations. Reconstructed CRT
 * callers use the distinct Microsoft _O_* constants below. */
#include_next <fcntl.h>
/* Reached Watcom flags remain separate from host and Microsoft encodings. */
#if defined(RECON_CRT_PROFILE_WATCOM_100A) && !defined(RECON_SHIM_IMPL)
#undef O_WRONLY
#define O_WRONLY 0x0001
#undef O_RDWR
#define O_RDWR 0x0002
#undef O_EXCL
#define O_EXCL 0x0400
#undef O_CREAT
#define O_CREAT 0x0020
#undef O_TRUNC
#define O_TRUNC 0x0040
#define O_TEXT 0x0100
#define O_BINARY 0x0200
#endif
/* BC4 flat-model FCNTL.H. In particular APPEND is0800; DOS sharing
 * bits are not Microsoft's RANDOM/SEQUENTIAL flags. The adapter currently
 * accepts only explicit readonly+binary, leaving every other tuple denied. */
#if defined(RECON_BORLAND_400_OPEN) && !defined(RECON_SHIM_IMPL)
#undef O_RDONLY
#undef O_WRONLY
#undef O_RDWR
#undef O_ACCMODE
#undef O_APPEND
#undef O_CREAT
#undef O_TRUNC
#undef O_EXCL
#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR 0x0002
#define O_ACCMODE 0x0003
#define O_CREAT 0x0100
#define O_TRUNC 0x0200
#define O_EXCL 0x0400
#define O_APPEND 0x0800
#define O_CHANGED 0x1000
#define O_DEVICE 0x2000
#define O_TEXT 0x4000
#define O_BINARY 0x8000
#define O_NOINHERIT 0x0080
#define O_DENYALL 0x0010
#define O_DENYWRITE 0x0020
#define O_DENYREAD 0x0030
#define O_DENYNONE 0x0040
#endif
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
