#ifndef LITERAL_WORKSPACE_H
#define LITERAL_WORKSPACE_H
#include <stddef.h>
#include <recon.h>
/* Captured empty literals can inspect text[-1], the preceding browser
   pointer's high byte. Keep that defined byte view and the shared pointer. */
typedef struct LiteralWorkspaceFields {
    char *browser;
    unsigned char text[2052];
} LiteralWorkspaceFields;
typedef union LiteralWorkspace {
    LiteralWorkspaceFields data;
    unsigned char bytes[2056];
} LiteralWorkspace;
extern LiteralWorkspace literal_workspace;
#define browser_filename (literal_workspace.data.browser)
#define expression_storage (literal_workspace.bytes + 4)
typedef char LiteralWorkspaceTextOffsetMustBe4[(offsetof(LiteralWorkspaceFields, text) == 4) ? 1 : -1];
typedef char LiteralWorkspaceSizeMustBe2056[(sizeof(LiteralWorkspace) == 2056) ? 1 : -1];
#endif
