#ifndef RECORD_PROCESSING_H
#define RECORD_PROCESSING_H
#include "frontend-records.h"
#include <recon.h>

/* Record kinds 0x0f..0x32 form eighteen pairs of nested delimiters. */
extern const unsigned char frontend_record_mates[36];
#define FRONTEND_RECORD_MATE(kind) \
    (((unsigned)(kind) >= 0x0f && (unsigned)(kind) <= 0x32) ? \
      frontend_record_mates[(unsigned)(kind) - 0x0f] : 0)

typedef struct ExpressionFrontendRecord {
    FrontendRecord header;
    IrToken *expression;
} ExpressionFrontendRecord;
typedef char ExpressionFrontendRecordPayloadOffsetMustBe16[
    (offsetof(ExpressionFrontendRecord, expression) == 16) ? 1 : -1];
/* The label lookup traverses this prefix through its separate label chain. */
typedef struct LabelFrontendRecord {
    FrontendRecord header;
    CppSymbol *label;
    struct LabelFrontendRecord *next_label;
} LabelFrontendRecord;
typedef char LabelFrontendRecordLabelOffsetMustBe16[
    (offsetof(LabelFrontendRecord, label) == 16) ? 1 : -1];
typedef char LabelFrontendRecordNextOffsetMustBe20[
    (offsetof(LabelFrontendRecord, next_label) == 20) ? 1 : -1];
/* The twenty-eight-byte forward-goto record retains a cleanup label
   and a separate goto-chain link in addition to its target symbol. */
typedef struct GotoFrontendRecord {
    FrontendRecord header;
    CppSymbol *label;
    IrToken *cleanup_label;
    struct GotoFrontendRecord *next_goto;
} GotoFrontendRecord;
typedef char GotoFrontendRecordSizeMustBe28[(sizeof(GotoFrontendRecord) == 28) ? 1 : -1];
typedef char GotoFrontendRecordLabelOffsetMustBe16[
    (offsetof(GotoFrontendRecord, label) == 16) ? 1 : -1];
typedef char GotoFrontendRecordNextOffsetMustBe24[
    (offsetof(GotoFrontendRecord, next_goto) == 24) ? 1 : -1];
typedef char GotoFrontendRecordCleanupOffsetMustBe20[
    (offsetof(GotoFrontendRecord, cleanup_label) == 20) ? 1 : -1];

IrToken *__stdcall collect_declaration_cleanup(IrToken *expression,
    FrontendRecord *from, int mode);
FrontendRecord *__stdcall find_forward_frontend_record(FrontendRecord *from,
    unsigned char kind, int recover_on_missing);
void __stdcall resolve_goto_cleanup(FrontendRecord *from, FrontendRecord *to, int mode);
IrToken *__stdcall append_symbol_destructor(IrToken *expression, CppSymbol *symbol);
IrToken *__stdcall append_expression_destructor(IrToken *expression, IrToken *object);
FrontendRecord *__stdcall find_label_record(CppSymbol *label);
FrontendRecord *__stdcall skip_declaration_records(FrontendRecord *from);
int has_unresolved_labels(void);
int function_paths_return(void);
unsigned __stdcall scan_return_paths(FrontendRecord **cursor, unsigned char stop_kind);
void finish_goto_cleanups(void);
#endif
