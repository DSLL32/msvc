#ifndef C1XX_PARSER_H
#define C1XX_PARSER_H
#include <stddef.h>
#include "frontend.h"
#include "parser-semantics.h"
#include <recon.h>

/* Separate declaration classifier. Run's40-byte semantic stride and2-byte
 * states establish this partition. The final state position would overlap
 * the semantic stack when both grow to150: that unresolved overflow domain
 * traps before the write in both builds. No initial stack contents are claimed.
 * The two pre-top addresses are control values, not storage before this owner. */
struct CompilerDeclarationParser {
    struct ParserTokenView *lookahead;
    unsigned syntax_errors;
    int recovery_shifts;
    short state;
    WORD unmodeled_header_0e;
    short *state_top;
    TokenValue *value_top;
    TokenValue values[150];
    short states[151];
    CompilerDeclarationParser *RECON_THISCALL Initialize(void);
    void RECON_THISCALL Reset(void);
    unsigned RECON_THISCALL Run(void);
    void RECON_THISCALL Finish(void);
};
typedef char CompilerDeclarationParser_layout[(sizeof(CompilerDeclarationParser) == 6328 && offsetof(CompilerDeclarationParser, state_top) == 16 && offsetof(CompilerDeclarationParser, value_top) == 20 && offsetof(CompilerDeclarationParser, values) == 24 && offsetof(CompilerDeclarationParser, states) == 0x1788) ? 1 : -1];

/* The three known header fields and logical parser stacks. Original-only
 * unknown header bytes retain the authentic allocation partition; native
 * semantic slots use the real typed declaration arm, whose member pointer
 * requires44 bytes natively rather than the original40. */
struct CompilerParser {
    unsigned reset_value; /* +0: only the actual zero writer is understood */
    unsigned syntax_errors; /* +4: incremented by recovery, returned by Run */
    int recovery_shifts; /* +8:3 after error, decremented after shifts */
#ifndef RECON_NATIVE_TEST
    BYTE original_unmodeled_header[12];
#endif
    ParserSemanticView values[1024];
    DWORD states[1024];
    CompilerParser *RECON_THISCALL Initialize(void);
    void RECON_THISCALL Reset(void);
    int RECON_THISCALL Run(void);
    void RECON_THISCALL Finish(void);
    int RECON_THISCALL Recover(void);
    void RECON_THISCALL ClearRecovery(void);
};
#ifndef RECON_NATIVE_TEST
typedef char CompilerParser_original_allocation[(sizeof(CompilerParser) == 45080 && offsetof(CompilerParser, values) == 24 && offsetof(CompilerParser, states) == 40984) ? 1 : -1];
#else
typedef char CompilerParser_native_allocation[(sizeof(CompilerParser) == 49164 && offsetof(CompilerParser, values) == 12 && offsetof(CompilerParser, states) == 45068) ? 1 : -1];
#endif
#endif
