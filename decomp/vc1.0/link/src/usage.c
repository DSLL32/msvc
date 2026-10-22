/* Banner and linker help, original 000254f0 and 00020160. */
#include "linker-types.h"
#include <recon.h>
TextBuffer command_trace;
void print_banner(void)
{
    char *description;
    switch (tool_kind) {
    case 1: description="32-Bit Executable Linker"; break;
    case 2: description="32-Bit Library Manager"; break;
    case 3: description="COFF Binary File Dumper"; break;
    case 4: description="COFF Binary File Editor"; break;
    default: description=tool_name; break;
    }
    printf("Microsoft (R) %s Version 1.00\nCopyright (C) Microsoft Corp 1992-93. All rights reserved.\n",description);
    printf("\n");
    if (command_trace.data) {
        append_text(&command_trace,"",1);
        printf("%s",command_trace.data);
        release_text(&command_trace);
    }
    fflush(stdout);
    banner_pending=0;
}
static const char *help_lines[] = {
    "   options:\n\n",
    "      /ALIGN:#\n",
    "      /BASE:{address|@filename,key}\n",
    "      /COMMENT:comment\n",
    "      /DEBUG[:{NONE|FULL}]\n",
    "      /DEBUGTYPE:{CV|COFF|BOTH}\n",
    "      /DEF:filename\n",
    "      /DEFAULTLIB:library[,library]\n",
    "      /DLL\n",
    "      /ENTRY:symbol\n",
    "      /EXPORT:symbol\n",
    "      /FIXED\n",
    "      /FORCE\n",
    "      /IMPLIB:filename\n",
    "      /INCLUDE:symbol\n",
    "      /MAP[:filename]\n",
    "      /NODEFAULTLIB[:library[,library...]]\n",
    "      /NOENTRY\n",
    "      /NOLOGO\n",
    "      /OPT:{REF|NOREF}\n",
    "      /ORDER:@filename\n",
    "      /OUT:filename\n",
    "      /RELEASE\n",
    "      /SECTION:name,[E][R][W][S]\n",
    "      /STACK:reserve[,commit]\n",
    "      /STUB:filename\n",
    "      /SUBSYSTEM:{WINDOWS|CONSOLE}[,#[.#]]\n",
    "      /VERBOSE\n",
    "      /VERSION:#[.#]\n",
    "      /WARN[:warninglevel]\n"
};
void link_usage(void)
{
    unsigned i;
    setvbuf(stdout,NULL,0,1);
    if (banner_pending) print_banner();
    printf("usage: %s [options] [files] [@commandfile]\n\n",program_name);
    for (i=0;i<sizeof(help_lines)/sizeof(help_lines[0]);++i) printf(help_lines[i]);
    exit(3);
}
