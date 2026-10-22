#ifndef INITIALIZERS_H
#define INITIALIZERS_H
#include "parser.h"
extern FrontendSymbol *initializer_symbol;
extern unsigned initializer_offset;
extern int initializer_runtime_override;
/* Encountered as three separately cleared words. Further meanings await
   reached consumers; preserve their shared contiguous reset storage. */
extern unsigned initializer_reset_words[3];
typedef struct InitializerFrame {
    FrontendSymbol *symbol;
    CType *type;
    unsigned offset;
    unsigned char flags, string_seen;
    unsigned short field0e;
} InitializerFrame;
typedef union InitializerFrameStorage {
    struct {
        unsigned bitfield_value;
        int bitfield_active, error_reported;
        InitializerFrame *frame;
        InitializerFrame frames[32];
    } fields;
    InitializerFrame with_sentinel[33];
} InitializerFrameStorage;
extern InitializerFrameStorage initializer_frame_storage;
#define initializer_frames (initializer_frame_storage.with_sentinel + 1)
#define initializer_frame initializer_frame_storage.fields.frame
#define initializer_bitfield_value initializer_frame_storage.fields.bitfield_value
#define initializer_bitfield_active initializer_frame_storage.fields.bitfield_active
#define initializer_error_reported initializer_frame_storage.fields.error_reported
extern int initializer_depth;
extern CType *initializer_inferred_type;
void __stdcall push_initializer_frame(FrontendSymbol *symbol);
int __stdcall is_scalar_initializer_symbol(FrontendSymbol *symbol);
extern unsigned initializer_bitfield_offset;
IrToken *__stdcall normalize_initializer_expression(IrToken *value);
int __stdcall select_initializer_target(IrToken *value);
void __stdcall check_runtime_initializer(FrontendSymbol *symbol, IrToken *value);
int __stdcall check_static_initializer(FrontendSymbol *symbol, IrToken **value);
int advance_initializer(void);
void __stdcall emit_runtime_initializer(IrToken *value);
unsigned __stdcall fit_string_initializer(FrontendSymbol *symbol, IrToken *value);
void __stdcall emit_runtime_symbol_initializer(FrontendSymbol *symbol, IrToken *value);
void __stdcall emit_initializer_constant(FrontendSymbol *symbol);
int __stdcall descend_initializer_target(IrToken *value);
FrontendSymbol *__stdcall next_initializer_member(FrontendSymbol *container, FrontendSymbol *member);
void __stdcall emit_member_padding(FrontendSymbol *member);
IrToken *__stdcall make_initializer_lvalue(CType *type);
void __stdcall emit_memory_initializer_call(unsigned operation, ArgumentNode *arguments);
void __stdcall emit_inline_memory_initializer(unsigned operation, IrToken *target, IrToken *value, IrToken *bytes);
int __stdcall emit_initializer_assignment(IrToken *target, IrToken *value);
int __stdcall emit_checked_initializer_assignment(IrToken *target, IrToken *value);
#endif
