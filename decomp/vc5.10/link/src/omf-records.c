#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "link.h"
#include "memory.h"
#include "omf.h"
#include <recon.h>

WORD omf_external_count,omf_external_symbol_count,omf_group_count;
WORD omf_segdef_count,omf_name_count,omf_module_count;
WORD omf_data_record,omf_data_segment,omf_default_line_segment;
WORD omf_section_symbol_count,omf_comment_count;
SHORT omf_record_type,omf_record_length;
WORD omf_record_is32;
BYTE omf_module_name_changed,omf_record_checksum;
DWORD omf_compile_flags,omf_data_offset,omf_pass;
DWORD omf_line_count,omf_comment_bytes;
OmfLine *omf_line_buffer;
/* These two actual 16KiB maps are cleared by this owner before record input.
 * Their pending consumers do not yet grant a wider element-role claim. */
DWORD omf_external_map[4096],omf_public_map[4096];

#pragma intrinsic(memset)
void __stdcall read_omf_module(const char *input)
{
    BYTE module_ended=0;
    unsigned kind;
    omf_external_symbol_count=0;
    omf_external_count=0;
    omf_group_count=0;
    omf_segdef_count=0;
    omf_name_count=0;
    omf_module_count=0;
    omf_module_name_changed=0;
    omf_record_type=0;
    omf_record_is32=0;
    omf_record_length=0;
    omf_compile_flags=0;
    omf_data_record=0;
    omf_data_segment=0;
    omf_data_offset=0;
    omf_pass=1;
    initialize_omf_hashes();
    append_omf_symbol("@comp.id",0x31c87,(DWORD)-1,3,0);
    omf_line_buffer=allocate_memory(512);
    memset(omf_public_map,0,sizeof(omf_public_map));
    memset(omf_external_map,0,sizeof(omf_external_map));
    omf_line_count=0;
    omf_default_line_segment=0;
    omf_comment_count=0;
    omf_section_symbol_count=0;
    omf_comment_bytes=0;
    rewind(omf_stream);
    omf_record_type=(SHORT)getc(omf_stream);
    while(omf_record_type!=-1) {
        omf_record_is32=(BYTE)omf_record_type&1;
        omf_record_length=(SHORT)read_omf_word();
        kind=(unsigned)omf_record_type&0xfe;
        switch(kind) {
        case 0x80: read_omf_theadr();break;
        case 0x82: read_omf_lheadr();break;
        case 0x88: read_omf_comment(input);break;
        case 0x8a: module_ended=1;read_omf_modend();break;
        case 0x8c: read_omf_extdefs(0,2);break;
        case 0x90: read_omf_pubdefs(2);break;
        case 0x94: read_omf_linnum();break;
        case 0x96: read_omf_lnames(0);break;
        case 0x98: read_omf_segdef();break;
        case 0x9a: read_omf_grpdef();break;
        case 0x9c:
            remember_omf_fixup_position(ftell(omf_stream)-3);
            read_omf_fixupp();
            break;
        case 0xa0: read_omf_ledata();break;
        case 0xa2: read_omf_lidata();break;
        case 0xb0: read_omf_comdefs(2);break;
        case 0xb2: read_omf_bakpat();break;
        case 0xb4: read_omf_extdefs(0,3);break;
        case 0xb6: read_omf_lpubdefs(3);break;
        case 0xb8: read_omf_comdefs(3);break;
        case 0xbc: read_omf_extdefs(1,2);break;
        case 0xc2: read_omf_comdat();break;
        case 0xc4: read_omf_linsym();break;
        case 0xc8: read_omf_alias();break;
        case 0xca: read_omf_lnames((DWORD)-1);break;
        default: omf_error("Unknown or bad record type %x",kind);break;
        }
        finish_omf_record();
        omf_record_checksum=read_omf_byte();
        if(module_ended)break;
        omf_record_type=(SHORT)getc(omf_stream);
    }
    if(!module_ended)link_fatal(input,1178);
    finish_omf_external_symbols();
    resolve_omf_segments();
    write_omf_lines();
    omf_pass=2;
    replay_omf_fixups();
}
