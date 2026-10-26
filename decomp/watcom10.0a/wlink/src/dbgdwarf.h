#ifndef WLINK_DBGDWARF_H
#define WLINK_DBGDWARF_H

/* ELF32 containers used for the executable's appended DWARF image. */
typedef struct elf32_header {
    unsigned char ident[16];
    unsigned short type, machine;
    unsigned long version, entry, phoff, shoff, flags;
    unsigned short ehsize, phentsize, phnum, shentsize, shnum, shstrndx;
} elf32_header;
typedef struct elf32_section {
    unsigned long name, type, flags, addr, offset, size;
    unsigned long link, info, align, entsize;
} elf32_section;
typedef char old_elf_header_52[(sizeof(elf32_header) == 52) ? 1 : -1];
typedef char old_elf_section_40[(sizeof(elf32_section) == 40) ? 1 : -1];
#include <recon.h>
#endif
