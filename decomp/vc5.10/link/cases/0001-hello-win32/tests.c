/* Narrow live-state check for the real hello invocation, before object loading. */
#include <recon-tool.h>
#include <recon-native.h>
#include <string.h>
#include "../../src/input.h"
#include "../../src/memory.h"
#include "../../harness/startup.h"
#include <recon.h>

extern MemoryArena *section_header_arena;

static void __stdcall check_section_headers(LinkContext *context,void *parent,
    OutputModule *module,WORD flags)
{
    unsigned i;
    BYTE *bytes;
    int pass=context && !parent && module && !flags;
    if (!pass) recon_native_entry_finish(0);
    pass=module->library==context->command_line_library &&
        module->library->module_head==module && module->section_count==7 &&
        !strcmp(module->name,"hello.obj") && !module->section_headers &&
        module->saved_section_headers && section_header_arena &&
        !section_header_arena->previous && section_header_arena->used==280 &&
        section_header_arena->remaining==4096-280 &&
        section_header_arena->base==(BYTE *)module->saved_section_headers;
    if (!pass) recon_native_entry_finish(0);
    bytes=(BYTE *)module->saved_section_headers;
    for (i=0;i<280;++i) if (bytes[i]) recon_native_entry_finish(0);
    recon_native_entry_finish(1);
}
static void invoke_hello(void *unused)
{
    char *args[]={"LINK.EXE","/nologo","/incremental:no","/out:hello.exe",
        "hello.obj","kernel32.lib",NULL};
    (void)unused;
    recon_link_invoke(6,args);
}
static int section_headers(void)
{
    return recon_native_at_entry(load_object_module,check_section_headers,
        invoke_hello,NULL)==1;
}
/* C harness spelling of the genuine native member entry: ECX module, one
   stack output-pointer argument. No adapter enters the application graph. */
extern IMAGE_SECTION_HEADER *RECON_THISCALL native_read_section_headers(
    OutputModule *,IMAGE_SECTION_HEADER **)
    __asm__("_ZN12OutputModule18ReadSectionHeadersEPP20IMAGE_SECTION_HEADER");
extern OutputLibrary *current_input_library;
static void RECON_THISCALL check_section_header_call(OutputModule *module,
    IMAGE_SECTION_HEADER **headers)
{
    recon_native_entry_finish(module && headers &&
        module->library==current_input_library &&
        module->library->module_head==module && module->section_count==7 &&
        module->saved_section_headers && !module->section_headers &&
        sizeof(*headers)==4);
}
static int section_header_call(void)
{
    return recon_native_at_entry(native_read_section_headers,check_section_header_call,
        invoke_hello,NULL)==1;
}
const ReconTest hello_tests[]={
    {"section-header-allocation",1,section_headers},
    {"loader-section-header-call",1,section_header_call}
};
