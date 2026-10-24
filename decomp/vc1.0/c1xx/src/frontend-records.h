#ifndef FRONTEND_RECORDS_H
#define FRONTEND_RECORDS_H
#include "ir.h"
#include <recon.h>

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
/* Declaration records additionally retain a second payload word. */
typedef struct DeclarationFrontendRecord {
    FrontendRecord header;
    CppSymbol *symbol;
    unsigned field14;
} DeclarationFrontendRecord;
/* Temporary cleanup records carry an expression rather than a symbol. */
typedef struct TemporaryFrontendRecord {
    FrontendRecord header;
    IrToken *object;
    unsigned field14;
} TemporaryFrontendRecord;
typedef char TemporaryFrontendRecordSizeMustBe24[(sizeof(TemporaryFrontendRecord) == 24) ? 1 : -1];
typedef char TemporaryFrontendRecordObjectOffsetMustBe16[(offsetof(TemporaryFrontendRecord, object) == 16) ? 1 : -1];
/* Conditional records retain links to their two statement regions. */
typedef struct ConditionalFrontendRecord {
    FrontendRecord header;
    FrontendRecord *then_record, *else_record;
} ConditionalFrontendRecord;
typedef char ConditionalFrontendRecordSizeMustBe24[(sizeof(ConditionalFrontendRecord) == 24) ? 1 : -1];
typedef char ConditionalThenRecordOffsetMustBe16[(offsetof(ConditionalFrontendRecord, then_record) == 16) ? 1 : -1];
typedef char ConditionalElseRecordOffsetMustBe20[(offsetof(ConditionalFrontendRecord, else_record) == 20) ? 1 : -1];
typedef struct BranchBoundaryRecord {
    FrontendRecord header;
    unsigned goto_count;
} BranchBoundaryRecord;
/* Loop closure saves the detached condition/evaluation sequence at +16. */
typedef struct LoopFrontendRecord {
    FrontendRecord header;
    FrontendRecord *condition_records;
} LoopFrontendRecord;
typedef char LoopFrontendRecordSizeMustBe20[(sizeof(LoopFrontendRecord) == 20) ? 1 : -1];
typedef char LoopConditionRecordsOffsetMustBe16[(offsetof(LoopFrontendRecord, condition_records) == 16) ? 1 : -1];
/* Constant condition values precede the delimiter which consumes them. */
typedef struct ConstantFrontendRecord {
    FrontendRecord header;
    unsigned value;
} ConstantFrontendRecord;
typedef char ConstantFrontendRecordSizeMustBe20[(sizeof(ConstantFrontendRecord) == 20) ? 1 : -1];
typedef char ConstantFrontendValueOffsetMustBe16[(offsetof(ConstantFrontendRecord, value) == 16) ? 1 : -1];
/* A switch retains its enclosing switch and later case/type bookkeeping. */
typedef struct SwitchFrontendRecord {
    FrontendRecord header;
    FrontendRecord *outer_switch;
    unsigned field14, constant_condition;
    CppSymbol *enum_tag;
} SwitchFrontendRecord;
typedef char SwitchFrontendRecordSizeMustBe32[(sizeof(SwitchFrontendRecord) == 32) ? 1 : -1];
typedef char SwitchOuterRecordOffsetMustBe16[(offsetof(SwitchFrontendRecord, outer_switch) == 16) ? 1 : -1];
void begin_while_condition_region(void);
void begin_for_condition_region(void);
void __stdcall begin_switch_body_region(CType *type);
void __stdcall record_case_label(IrToken *expression);
void end_while_condition_region(void);
void end_while_body(void);
typedef char BranchBoundaryRecordSizeMustBe20[(sizeof(BranchBoundaryRecord) == 20) ? 1 : -1];
typedef char BranchBoundaryGotoCountOffsetMustBe16[(offsetof(BranchBoundaryRecord, goto_count) == 16) ? 1 : -1];
typedef char DeclarationFrontendRecordSizeMustBe24[(sizeof(DeclarationFrontendRecord) == 24) ? 1 : -1];
typedef char DeclarationFrontendRecordSymbolOffsetMustBe16[(offsetof(DeclarationFrontendRecord, symbol) == 16) ? 1 : -1];
typedef char DeclarationFrontendRecordPayloadOffsetMustBe20[(offsetof(DeclarationFrontendRecord, field14) == 20) ? 1 : -1];
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
/* Trailing symbol records detached into an expression-owned list. */
typedef struct ExpressionSymbolList {
    struct ExpressionSymbolList *next;
    CppSymbol *symbol;
} ExpressionSymbolList;
typedef char ExpressionSymbolListSizeMustBe8[(sizeof(ExpressionSymbolList) == 8) ? 1 : -1];
ExpressionSymbolList *detach_expression_symbols(void);
IrToken *__stdcall attach_expression_symbols(IrToken *expression, ExpressionSymbolList *symbols);
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
void begin_if_condition_region(void);
void end_if_condition_region(void);
void close_if_records(void);
void __stdcall remove_matching_frontend_record(unsigned char kind);
void __stdcall remove_frontend_record(FrontendRecord *record);
void __stdcall record_boolean_constant(unsigned value);
void end_frontend_scope(void);
void remove_last_frontend_record(void);
void __stdcall record_parameter_cleanup(CppSymbol *parameter);
#endif
