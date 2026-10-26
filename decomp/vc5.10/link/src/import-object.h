#ifndef LINK_IMPORT_OBJECT_H
#define LINK_IMPORT_OBJECT_H
#include <windows.h>
#include <stddef.h>
#include <recon.h>

/* Encountered 20-byte short-import record, separate from a COFF file header.
 * The last WORD carries the two-bit import kind and three-bit name encoding. */
typedef struct CoffImportHeader {
    WORD signature1,signature2,version,machine;
    DWORD timestamp,data_size;
    WORD ordinal_hint;
    WORD kind:2,name_type:3,reserved:11;
} CoffImportHeader;
typedef char CoffImportHeaderLayout[(sizeof(CoffImportHeader)==20 &&
    offsetof(CoffImportHeader,data_size)==12 && offsetof(CoffImportHeader,ordinal_hint)==16)?1:-1];

#endif
