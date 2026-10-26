#ifndef WLINK_DBGDWARF_H
#define WLINK_DBGDWARF_H

/* ELF32 records used by the old executable and appended DWARF image. */
typedef struct elf32_header {
    unsigned char ident[16];
    unsigned short type, machine;
    unsigned long version, entry, phoff, shoff, flags;
    unsigned short ehsize, phentsize, phnum, shentsize, shnum, shstrndx;
} elf32_header;
typedef struct elf32_dynamic {
    unsigned long tag, value;
} elf32_dynamic;
typedef struct elf32_symbol {
    unsigned long name, value, size;
    unsigned char info, other;
    unsigned short shndx;
} elf32_symbol;
typedef struct elf32_program {
    unsigned long type, offset, vaddr, paddr, filesz, memsz, flags, align;
} elf32_program;
typedef struct elf32_section {
    unsigned long name, type, flags, addr, offset, size;
    unsigned long link, info, align, entsize;
} elf32_section;
typedef char old_elf_header_52[(sizeof(elf32_header) == 52) ? 1 : -1];
typedef char old_elf_section_40[(sizeof(elf32_section) == 40) ? 1 : -1];
typedef char old_elf_program_32[(sizeof(elf32_program) == 32) ? 1 : -1];
extern unsigned long WriteELFData( elf32_program *, unsigned long, unsigned long * );
extern void WriteELFDynamic( elf32_program *, unsigned long, unsigned long );
#include <recon.h>
#endif
