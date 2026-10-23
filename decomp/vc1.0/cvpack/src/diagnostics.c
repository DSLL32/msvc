/* This is the original indexed diagnostic table at 38bf0, shared by the
   warning/fatal handlers. Its bounded indices and string contents are audited. */
#include "cvpack.h"
#include <recon.h>

const char *diagnostics[40] = {
    "",
    "file already packed",
    "duplicate public symbol %s in module %s",
    "error in lexical scopes for module %s; symbols deleted",
    "unrecognized symbol discarded; recompile module %s",
    "unrecognized type converted to NULL; recompile module %s",
    "type length limit exceeded; converted to NULL",
    "unrecognized option /%s; option ignored",
    "section length limit exceeded; %s in module %s",
    "exceeded 64K types at %s; discarding subsequent types",
    "cannot update checksum",
    "",
    "",
    "out of memory",
    "out of memory",
    "cannot open file: %s",
    "%s is read-only",
    "invalid executable file",
    "invalid module %s",
    "invalid %s table in module %s",
    "cannot write output file",
    "",
    "",
    "type information corrupt, recompile module %s",
    "no debugging information",
    "incompatible version of debugging information",
    "",
    "too many segments in module %s",
    "cannot execute MPC.EXE",
    "cannot find precompiled types file; relink with %s",
    "%s cannot use precompiled types file %s : type counts do not match",
    "new module declared during incremental link",
    "",
    "%s cannot use precompiled types file %s : signatures do not match",
    "cannot open program database %s",
    "%s cannot use program database %s : module newer than database",
    "%s cannot use program database %s : signatures do not match",
    "incorrect COFF symbol table",
    "symbol table for module %s is too large",
    "Usage: cvpack [/help] [/minimum] [/nologo] [/pcode] [/strip [dbgfile]] [/verbose] exefile"
};

void print_banner(void)
{
    printf("Microsoft (R) Debugging Information Compactor  Version %d.%02d.%02d\n",
           4, 25, 0);
    printf("Copyright (c) Microsoft Corp 1987-1993. All rights reserved.\n\n");
    banner_pending = 0;
}

void fatal_error(int code, const char *first, const char *second)
{
    char message[300];
    int length = 0;
    if (show_logo == 1 && banner_pending == 1) print_banner();
    if (code >= 40) exit(1);
    if (code == 14) code = 13;
    if (code != 39)
        length = sprintf(message, "CVPACK : fatal error CK1%03d: ", code-12);
    length += sprintf(message+length, diagnostics[code], first, second);
    sprintf(message+length, "\n");
    printf(message);
    exit(1);
}
