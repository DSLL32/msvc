/* Serialized default MZ stub used when no /STUB image is supplied.
 * The 0x80 PE-signature offset and fixed message capacity are file-format
 * fields; this object contains no adjacent LINK data or section padding.
 */
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
DosStub default_dos_stub = {
    {0x5a4d,0x90,3,0,4,0,0xffff,0,0xb8,0,0,0,0x40,0,{0},0,0,{0},0x80},
    {0x0e,0x1f,0xba,14,0xb4,9,0xcd,0x21,0xb8,0x4c01,0xcd,0x21},
    "This program cannot be run in DOS mode.\r\r\n$", 0x4550
};
DosStub *dos_stub_template = &default_dos_stub;
unsigned long dos_stub_size = sizeof(DosStub);
