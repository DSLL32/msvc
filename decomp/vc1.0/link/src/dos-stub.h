#ifndef LINK_DOS_STUB_H
#define LINK_DOS_STUB_H
#pragma pack(1)
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef struct DosHeader {
    WORD magic, bytes_on_last_page, page_count, relocations, header_paragraphs;
    WORD minimum_allocation, maximum_allocation, ss, sp, checksum, ip, cs;
    WORD relocation_offset, overlay, reserved[4], oem_id, oem_info, reserved2[10];
    DWORD pe_offset;
} DosHeader;
typedef struct DosMessageProgram {
    unsigned char push_cs, pop_ds, mov_dx;
    WORD message_offset;
    unsigned char mov_ah, print_service, interrupt_print, dos_print;
    unsigned char mov_ax;
    WORD exit_status;
    unsigned char interrupt_exit, dos_exit;
} DosMessageProgram;
typedef struct DosStub {
    DosHeader header;
    DosMessageProgram program;
    char message[50];
    DWORD pe_signature;
} DosStub;
#pragma pack()
extern DosStub default_dos_stub,mac_dos_stub;
extern char *dos_stub_template;
extern unsigned long dos_stub_size;
#endif
