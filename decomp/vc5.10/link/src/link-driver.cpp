/* The real thiscall caller is the only reason this file is C++. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "link.h"
#include "initializers.h"
#include "crt-imports.h"
#include "link-driver.h"
#include "options.h"
#include <recon.h>

extern "C" {
extern BYTE incremental_requested, incremental_switch_set;
extern const char *output_file_name;
unsigned char pdb_enabled = 1, debug_option_present;
unsigned char incremental_enabled, incremental_reuse, incremental_failed;
unsigned char reference_optimization_specified;
DWORD debug_formats, maximum_image_size;
FILE *map_output;
char *repro_directory;
int link_argc;
char **link_argv;
void __cdecl linker_usage(void);
void __cdecl initialize_repro(void);
void __stdcall collect_file_arguments(int,char **,const char *,const char *);
void __stdcall parse_command_line(int,char **,const char *,const char *);
unsigned char __stdcall has_switch(const char *);
void *__stdcall allocate_link_memory(unsigned,unsigned);
void __cdecl link_warning(const char *,unsigned,...);
void __cdecl link_fatal(const char *,unsigned,...);
void __cdecl link_error(const char *,unsigned,...);
void __stdcall validate_debug_options(LinkContext *);
BYTE __stdcall supports_incremental(LinkContext *);
void __stdcall load_incremental_state(LinkContext **);
void __cdecl initialize_incremental_inputs(void);
int __stdcall incremental_link(LinkContext **);
int __stdcall abort_incremental(int);
void __stdcall write_image_map(LinkContext *,FILE *);
void __stdcall save_incremental_state(LinkContext *);
void __cdecl close_files(void);
void __cdecl cleanup_pdb(void);
int __cdecl run_cvpack(void);
void __cdecl close_repro(void);
}

extern "C" int __stdcall run_linker(int argc, char **argv)
{
    LinkContext *context;
    int is_vxd, result;
    BYTE pack_requested;
    const char *subsystem;
    if (argc < 2) linker_usage();
    initialize_repro();
    collect_file_arguments(argc, argv, "LINK", "_LINK_");
    parse_command_line(argc, argv, "LINK", "_LINK_");
    is_vxd = has_switch("vxd") ? 1 : 0;
    logo_pending = !has_switch("nologo");
    context = (LinkContext *)allocate_link_memory(1, sizeof(LinkContext));
    context->Initialize(is_vxd);
    configure_link_options(context, NULL, NULL);
    context->options.debug_formats = debug_formats;
    if (!context->options.debug_formats) context->options.debug_formats = 2;
    if (context->is_vxd == 1) {
        context->options.debug_formats &= 2;
        if (!pdb_enabled || debug_option_present)
            context->options.debug_formats = 0;
    }
    if (context->options.debug_formats) context->options.debug_formats |= 0x14;
    if (!context->options.debug_level) context->options.debug_formats = 0;
    if ((context->options.debug_formats & 8) && !pdb_enabled) {
        context->options.debug_formats &= ~8;
        link_warning(NULL, 4075, "DEBUGTYPE:FIXUP", "PDB:NONE");
    }
    validate_debug_options(context);
    if (!(context->options.debug_formats & 14)) pdb_enabled = 0;
    subsystem = NULL;
    switch (context->optional_header.Subsystem) {
    case 1: subsystem = "SUBSYSTEM:NATIVE"; break;
    case 2: subsystem = "SUBSYSTEM:CONSOLE"; break;
    case 3: subsystem = "SUBSYSTEM:WINDOWS"; break;
    case 7: subsystem = "SUBSYSTEM:POSIX"; break;
    case 9: subsystem = "SUBSYSTEM:WINDOWSCE"; break;
    }
    if (context->is_vxd == 1) {
        if (context->optional_header.Subsystem)
            link_warning(NULL, 4075, subsystem, "VXD");
        context->optional_header.Subsystem = 1;
    }
    if (context->options.rom) {
        if (context->optional_header.Subsystem)
            link_warning(NULL, 4075, subsystem, "ROM");
        context->optional_header.Subsystem = 1;
    }
    if ((context->options.windowsce_emulation || context->options.windowsce_noconvert) &&
        subsystem && context->optional_header.Subsystem != 2) {
        if (context->options.windowsce_emulation) {
            context->options.windowsce_emulation = 0;
            link_warning(NULL, 4075, "WINDOWSCE:EMULATION", subsystem);
        }
        if (!context->options.reference_opt_specified && context->options.windowsce_noconvert) {
            context->options.windowsce_noconvert = 0;
            link_warning(NULL, 4075, "WINDOWSCE:NOCONVERT", subsystem);
        }
    }
    if (context->options.rom) context->options.checksum_enabled = 0;
    if (context->options.fixed_image) {
        if (context->options.profile) {
            context->options.fixed_image = 0;
            link_warning(NULL, 4075, "FIXED", "PROFILE");
        }
        if (context->is_vxd == 1) {
            context->options.fixed_image = 0;
            link_warning(NULL, 4075, "FIXED", "VXD");
        }
    }
    if (!context->options.retain_relocations && context->is_vxd != 1 &&
        !context->options.rom && (context->specified_switches & 1))
        link_warning(NULL, 4108);
    if (!(context->specified_switches & 0x80000) &&
        !(context->file_header.Characteristics & 0x2000) &&
        !context->options.retain_relocations && context->is_vxd != 1 &&
        !context->options.rom && !incremental_enabled && !context->options.profile &&
        context->optional_header.Subsystem != 1 &&
        !(context->optional_header.Subsystem == 9 &&
          context->optional_header.MajorSubsystemVersion == 1 &&
          context->optional_header.MinorSubsystemVersion == 0))
        context->options.fixed_image = 1;
    if (context->options.fixed_image) context->file_header.Characteristics |= 1;
    if (logo_pending) print_logo();
    link_argc = argc;
    link_argv = argv;
    if (incremental_enabled) {
        load_incremental_state(&context);
        if (context->options.order && (!incremental_enabled || !incremental_reuse))
            link_fatal(NULL, 1219);
        if (incremental_enabled) initialize_incremental_inputs();
    }
    if (!reference_optimization_specified)
        context->options.effective_reference_opt = !incremental_enabled && !context->options.debug_level;
    if (!(context->specified_switches & 0x400000))
        context->options.fold_identical_comdats = context->options.effective_reference_opt;
    if (context->options.fold_identical_comdats) buffer4.remaining = 0;
    if (incremental_enabled && !incremental_reuse && context->options.option142 && incremental_failed)
        link_error(NULL, 6021);
    result = -1;
    if (incremental_reuse) {
        result = incremental_link(&context);
        if (result == -1) return abort_incremental(1);
    }
    if (!incremental_enabled || !incremental_reuse || result) {
        result = context->FullLink(&pack_requested);
        if (incremental_enabled) {
            if (context->options.write_map) write_image_map(context, map_output);
            if (incremental_enabled) save_incremental_state(context);
        }
    }
    close_files();
    cleanup_pdb();
    if (incremental_enabled) return incremental_reuse ? (incremental_failed ? 6 : 0) : 0;
    if (pack_requested) {
        fflush(NULL);
        result = run_cvpack();
        if (result) link_warning(NULL, 4027);
    }
    if (context->options.write_map) write_image_map(context, map_output);
    if (maximum_image_size && context->optional_header.SizeOfImage > maximum_image_size)
        link_warning(NULL, 4013, context->optional_header.SizeOfImage, maximum_image_size);
    fclose(diagnostic_output);
    if (repro_directory) close_repro();
    return result;
}

/* Four independently declared output templates are constructed where their
 * fields are first encountered. The DOS bootstrap is file data, not native code. */
extern "C" const DefaultDosImage default_dos_image = {
    {0x5a4d,0x90,3,0,4,0,0xffff,0,0xb8,0,0,0,0x40,0,
     {0,0,0,0},0,0,{0,0,0,0,0,0,0,0,0,0},0},
    {0x0e,0x1f,0xba,0x0e,0x00,0xb4,0x09,0xcd,0x21,0xb8,0x01,0x4c,0xcd,0x21},
    "This program cannot be run in DOS mode.\r\r\n$"
};
HeaderSizeHandler image_header_size;
SectionWriter image_write_section;
HeaderWriter image_write_headers;
extern "C" StringTable *__stdcall create_string_table(StringTable **,unsigned,unsigned);
extern "C" void *__stdcall allocate_memory(unsigned);

#pragma intrinsic(strcpy,memset)
LinkAuxiliaryData *RECON_THISCALL LinkContext::Initialize(int vxd)
{
    is_vxd=vxd;
    self=this;
    database_type=6;
    database_flags=0;
    strcpy(database_signature,"Microsoft Linker Database\n\a\x1a");
    memset(&file_header,0,sizeof(file_header));
    file_header.SizeOfOptionalHeader=sizeof(optional_header);
    file_header.Characteristics=0x100;
    memset(&optional_header,0,sizeof(optional_header));
    optional_header.Magic=0x10b;
    optional_header.MajorLinkerVersion=5;
    optional_header.MinorLinkerVersion=10;
    optional_header.ImageBase=0x400000;
    optional_header.SectionAlignment=0x1000;
    optional_header.FileAlignment=0x200;
    optional_header.MajorOperatingSystemVersion=4;
    optional_header.SizeOfStackReserve=0x100000;
    optional_header.SizeOfStackCommit=0x1000;
    optional_header.SizeOfHeapReserve=0x100000;
    optional_header.SizeOfHeapCommit=0x1000;
    optional_header.NumberOfRvaAndSizes=16;
    memset(&options,0,sizeof(options));
    options.bind_delay_imports=1;
    options.legacy_import_library=1;
    options.mac_bundle=1;
    options.option142=1;
    options.default144=1;
    options.default_extension=".dll";
    options.default180=1;
    specified_switches=0;
    memset(&reserved194,0,32);
    create_string_table(&symbols,1024,128);
    create_string_table(&strings,128,32);
    pointer_list.first=NULL;
    pointer_list.last=&pointer_list.first;
    pointer_list.active=0;
    pointer_list.count=0;
    dos_image=&default_dos_image;
    dos_image_size=128;
    objects=NULL;
    objects_tail=&objects;
    if (vxd==1) {
        image_header_size=&LinkContext::VxdHeaderSize;
        image_write_section=&LinkContext::VxdWriteSection;
        image_write_headers=&LinkContext::VxdWriteHeaders;
        InitializeVxd();
    } else if (vxd==0) {
        image_header_size=&LinkContext::PeHeaderSize;
        image_write_section=&LinkContext::PeWriteSection;
        image_write_headers=&LinkContext::PeWriteHeaders;
    }
    auxiliary=(LinkAuxiliaryData *)(incremental_enabled?
        allocate_link_memory(1,sizeof(LinkAuxiliaryData)):
        allocate_memory(sizeof(LinkAuxiliaryData)));
    if (auxiliary) memset(auxiliary,0,sizeof(LinkAuxiliaryData));
    auxiliary_flags=0;
    /* Actual original EAX is this auxiliary allocation, not the receiver.
     * The only discovered caller ignores that return value. */
    return auxiliary;
}
void RECON_THISCALL LinkContext::InitializeVxd(void) { __asm { int 3 } }
DWORD RECON_THISCALL LinkContext::PeHeaderSize(DWORD *offset) { __asm { int 3 } return 0; }
DWORD RECON_THISCALL LinkContext::VxdHeaderSize(DWORD *offset) { __asm { int 3 } return 0; }
void RECON_THISCALL LinkContext::PeWriteSection(int file,void *section,void *header) { __asm { int 3 } }
void RECON_THISCALL LinkContext::VxdWriteSection(int file,void *section,void *header) { __asm { int 3 } }
void RECON_THISCALL LinkContext::PeWriteHeaders(int file) { __asm { int 3 } }
void RECON_THISCALL LinkContext::VxdWriteHeaders(int file) { __asm { int 3 } }

int RECON_THISCALL LinkContext::FullLink(BYTE *pack_requested)
{
    __asm { int 3 }
    return 255;
}

/* Named original callees remain trapping until reached. */
extern "C" {
void __cdecl linker_usage(void)
{
    if (logo_pending) print_logo();
    puts(
        "usage: LINK [options] [files] [@commandfile]\n"
        "\n"
        "   options:\n"
        "\n"
        "      /ALIGN:#\n"
        "      /BASE:{address|@filename,key}\n"
        "      /COMMENT:comment\n"
        "      /DEBUG\n"
        "      /DEBUGTYPE:{CV|COFF}\n"
        "      /DEF:filename\n"
        "      /DEFAULTLIB:library\n"
        "      /DLL\n"
        "      /DRIVER[:{UPONLY|WDM}]\n"
        "      /ENTRY:symbol\n"
        "      /EXETYPE:DYNAMIC\n"
        "      /EXPORT:symbol\n"
        "      /FIXED[:NO]\n"
        "      /FORCE[:{MULTIPLE|UNRESOLVED}]\n"
        "      /GPSIZE:#\n"
        "      /HEAP:reserve[,commit]\n"
        "      /IMPLIB:filename\n"
        "      /INCLUDE:symbol\n"
        "      /INCREMENTAL:{YES|NO}\n"
        "      /LARGEADDRESSAWARE\n"
        "      /LIBPATH:dir\n"
        "      /MACHINE:{IX86|ALPHA|ARM|MIPS|MIPSR41XX|PPC|SH3|SH4}\n"
        "      /MAP[:filename]\n"
        "      /MAPINFO:{EXPORTS|FIXUPS|LINES}\n"
        "      /MERGE:from=to\n"
        "      /NODEFAULTLIB[:library]\n"
        "      /NOENTRY\n"
        "      /NOLOGO\n"
        "      /OPT:{ICF[,iterations]|NOICF|NOREF|REF}\n"
        "      /ORDER:@filename\n"
        "      /OUT:filename\n"
        "      /PDB:{filename|NONE}\n"
        "      /PDBTYPE:{CON[SOLIDATE]|SEPT[YPES]}\n"
        "      /PROFILE\n"
        "      /RELEASE\n"
        "      /SECTION:name,[E][R][W][S][D][K][L][P][X]\n"
        "      /STACK:reserve[,commit]\n"
        "      /STUB:filename\n"
        "      /SUBSYSTEM:{NATIVE|WINDOWS|CONSOLE|WINDOWSCE|POSIX}[,#[.##]]\n"
        "      /SWAPRUN:{CD|NET}\n"
        "      /VERBOSE[:LIB]\n"
        "      /VERSION:#[.#]\n"
        "      /VXD\n"
        "      /WARN[:warninglevel]\n"
        "      /WINDOWSCE:{EMULATION|NOCONVERT}\n"
        "      /WS:AGGRESSIVE"
    );
    exit(1100);
}
void __cdecl link_warning(const char *file,unsigned message,...) { __asm { int 3 }  }
void __cdecl link_fatal(const char *file,unsigned message,...) { __asm { int 3 }  }
void __cdecl link_error(const char *file,unsigned message,...) { __asm { int 3 }  }
void __stdcall validate_debug_options(LinkContext *context)
{
    const char *reason;
    incremental_enabled=incremental_requested;
    /* The /RE option requires an output name and forces incremental use. */
    if (context->options.order) {
        if (!output_file_name) link_fatal(NULL,1220,"/RE");
        incremental_enabled=1;
        return;
    }
    if (context->is_vxd) {
        if (!incremental_requested) return;
        reason="VXD";
        goto incompatible_option;
    }
    if (context->options.profile) {
        if (incremental_requested) {
            link_warning(NULL,4075,"INCREMENTAL","PROFILE");
            incremental_enabled=0;
        }
        context->options.write_map=1;
        context->options.map_fixups=1;
        return;
    }
    if (pdb_enabled && debug_formats!=2 && debug_formats &&
        context->options.debug_level) {
        if (!incremental_requested) return;
        reason="DEBUGTYPE";
        goto incompatible_option;
    }
    if (!incremental_switch_set)
        incremental_enabled=context->options.debug_level!=0;
    if (!incremental_enabled) return;
    if (!pdb_enabled && context->options.debug_level) {
        if (!incremental_switch_set) goto disable_incremental;
        reason="PDB:NONE";
        goto incompatible_option;
    }
    if ((debug_option_present && context->options.effective_reference_opt) ||
        ((context->specified_switches & 0x400000) &&
         context->options.fold_identical_comdats)) {
        if (!incremental_switch_set) goto disable_incremental;
        reason="OPT";
        goto incompatible_option;
    }
    if (context->options.order_file) {
        if (!incremental_switch_set) goto disable_incremental;
        reason="ORDER";
        goto incompatible_option;
    }
    if (context->options.checksum_enabled) {
        if (!incremental_switch_set) goto disable_incremental;
        reason="RELEASE";
        goto incompatible_option;
    }
    if (context->options.force) {
        if (!incremental_switch_set) goto disable_incremental;
        reason="FORCE";
        goto incompatible_option;
    }
    if (!supports_incremental(context)) goto disable_incremental;
    return;
incompatible_option:
    link_warning(NULL,4075,"INCREMENTAL",reason);
disable_incremental:
    incremental_enabled=0;
}
BYTE __stdcall supports_incremental(LinkContext *context) { __asm { int 3 } return 0; }
void __stdcall load_incremental_state(LinkContext **context) { __asm { int 3 }  }
void __cdecl initialize_incremental_inputs(void) { __asm { int 3 }  }
int __stdcall incremental_link(LinkContext **context) { __asm { int 3 } return 0; }
int __stdcall abort_incremental(int failed) { __asm { int 3 } return 0; }
void __stdcall write_image_map(LinkContext *context,FILE *output) { __asm { int 3 }  }
void __stdcall save_incremental_state(LinkContext *context) { __asm { int 3 }  }
void __cdecl close_files(void) { __asm { int 3 }  }
void __cdecl cleanup_pdb(void) { __asm { int 3 }  }
int __cdecl run_cvpack(void) { __asm { int 3 } return 0; }
void __cdecl close_repro(void) { __asm { int 3 }  }
}
