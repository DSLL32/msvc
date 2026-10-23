/* The real thiscall caller is the only reason this file is C++. */
#include <windows.h>
#include <stdio.h>
#include "link.h"
#include "initializers.h"
#include "crt-imports.h"
#include "link-driver.h"
#include <recon.h>

extern "C" {
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
void __stdcall configure_link_options(LinkContext *,void *,void *);
void __cdecl link_warning(const char *,unsigned,...);
void __cdecl link_fatal(const char *,unsigned,...);
void __cdecl link_error(const char *,unsigned,...);
void __stdcall validate_debug_options(LinkContext *);
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
    if (context->options.rom) context->options.emit_symbols = 0;
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
        context->options.discard_unreferenced = context->options.effective_reference_opt;
    if (context->options.discard_unreferenced) buffer4.remaining = 0;
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

LinkContext *RECON_THISCALL LinkContext::Initialize(int vxd)
{
    __asm { int 3 }
    return this;
}
int RECON_THISCALL LinkContext::FullLink(BYTE *pack_requested)
{
    __asm { int 3 }
    return 255;
}

/* Named original callees remain trapping until reached. */
extern "C" {
void __cdecl linker_usage(void) { __asm { int 3 }  }
void __stdcall parse_command_line(int argc,char **argv,const char *before,const char *after) { __asm { int 3 }  }
unsigned char __stdcall has_switch(const char *name) { __asm { int 3 } return 0; }
void *__stdcall allocate_link_memory(unsigned count,unsigned size) { __asm { int 3 } return NULL; }
void __stdcall configure_link_options(LinkContext *context,void *first,void *second) { __asm { int 3 }  }
void __cdecl link_warning(const char *file,unsigned message,...) { __asm { int 3 }  }
void __cdecl link_fatal(const char *file,unsigned message,...) { __asm { int 3 }  }
void __cdecl link_error(const char *file,unsigned message,...) { __asm { int 3 }  }
void __stdcall validate_debug_options(LinkContext *context) { __asm { int 3 }  }
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
