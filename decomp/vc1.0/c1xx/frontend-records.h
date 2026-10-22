#ifndef FRONTEND_RECORDS_H
#define FRONTEND_RECORDS_H
#include "ir.h"

/* Linked frontend work records, distinct from the twenty-byte parser IR
   tokens and from the serialized intermediate-file record formats. */
typedef struct FrontendRecord {
    unsigned char kind, flags;
    unsigned short field02;
    unsigned sequence;
    struct FrontendRecord *next, *previous;
} FrontendRecord;
typedef struct SymbolFrontendRecord {
    FrontendRecord header;
    CppSymbol *symbol;
} SymbolFrontendRecord;
typedef char SymbolFrontendRecordSizeMustBe20[(sizeof(SymbolFrontendRecord) == 20) ? 1 : -1];
typedef struct FunctionRecord {
    FrontendRecord header;
    CppSymbol *function;
    unsigned field14;
} FunctionRecord;
typedef char FrontendRecordSizeMustBe16[(sizeof(FrontendRecord) == 16) ? 1 : -1];
typedef char FrontendRecordNextOffsetMustBe8[(offsetof(FrontendRecord, next) == 8) ? 1 : -1];
typedef char FrontendRecordPreviousOffsetMustBe12[(offsetof(FrontendRecord, previous) == 12) ? 1 : -1];
typedef char FunctionRecordSizeMustBe24[(sizeof(FunctionRecord) == 24) ? 1 : -1];
typedef char FunctionRecordSymbolOffsetMustBe16[(offsetof(FunctionRecord, function) == 16) ? 1 : -1];
extern FrontendRecord *ir_record_head, *ir_record_tail, *function_ir_tail;
extern int ir_function_error_count, ir_records_suppressed;
extern FrontendRecord *ir_free_records, *ir_label_records, *ir_goto_records, *ir_switch_records;
extern unsigned ir_record_sequence, ir_goto_count;
extern int ir_expression_depth;
extern CppSymbol *initializing_symbol;
extern int initializing_reference, member_initializer_active;
/* Points to an original-toolchain jmp_buf during record error recovery. */
extern int *ir_record_error_context;
FrontendRecord *__stdcall reuse_frontend_record(unsigned char size_class);
FrontendRecord *__stdcall find_frontend_record(FrontendRecord *from, unsigned char kind,
    int recover_on_missing);
void __stdcall begin_function_ir(CppSymbol *function, CppSymbol *parameters);
void reset_frontend_records(void);
void __stdcall append_frontend_record(FrontendRecord *record, unsigned bytes);
void begin_frontend_scope(void);
void end_frontend_scope(void);
void remove_last_frontend_record(void);
void __stdcall record_parameter_cleanup(CppSymbol *parameter);
#endif
