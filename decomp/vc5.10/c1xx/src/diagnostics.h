#ifndef C1XX_DIAGNOSTICS_H
#define C1XX_DIAGNOSTICS_H
#include "frontend.h"
#include "ir.h"
#include <recon.h>

/* The diagnostic table indexes records by a DWORD sequence number. These
 * handles are not pointers to the record returned by the later lookup. */
typedef unsigned CompilerDiagnosticContext;
/* Buffers grow at both ends of an initially centered empty string. The
 * allocator and the prepend/append readers establish these actual fields. */
struct CompilerDiagnosticRecord {
    CompilerDiagnosticContext id;
    char *text;
    unsigned prefix_cursor, suffix_cursor, capacity;
};
typedef char CompilerDiagnosticRecord_size[sizeof(CompilerDiagnosticRecord) == 20 ? 1 : -1];
extern const char *const compiler_token_spellings[246];
extern "C" {
void __cdecl begin_declaration_diagnostic(void);
void __cdecl end_declaration_diagnostic(void);
CompilerDiagnosticContext __cdecl allocate_diagnostic_context(void);
CompilerDiagnosticContext __cdecl describe_declaration_context(unsigned, CompilerDeclarationContext);
const char *__cdecl format_declaration_context(CompilerDiagnosticContext);
}
#endif
