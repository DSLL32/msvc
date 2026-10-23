#ifndef PARAMETERS_H
#define PARAMETERS_H
#include "declarations.h"
#include "frontend-records.h"
#include <recon.h>

typedef struct DeclarationParameterInfo {
    int count;
    unsigned short debug_index, field06;
    CType *types[1];
} DeclarationParameterInfo;
typedef struct DefaultArgument {
    unsigned field00, field04;
    struct DefaultArgument *following;
    void *expression;
    struct DefaultArgument *next;
} DefaultArgument;
struct FunctionParameters {
    CType *associated_type;
    unsigned char default_count, flags;
    unsigned short field06;
    DefaultArgument *defaults;
    DeclarationParameterInfo *declarations;
};
struct DeclarationParameterEntry {
    DeclarationParameterEntry *next;
    DeclarationParameterInfo parameters;
};
unsigned __stdcall hash_declaration_parameters(DeclarationParameterInfo *parameters);
struct ParameterEntry {
    ParameterEntry *next;
    FunctionParameters parameters;
};
/* A preceding declaration may set flags[-1], the high depth byte. Keep
   that boundary overlap rather than separate independently placed globals. */
typedef union DeclarationControl {
    struct {
        int depth;
        unsigned char flags[60];
    } data;
    unsigned char bytes[64];
} DeclarationControl;
extern DeclarationControl declaration_control;
#define declaration_depth (declaration_control.data.depth)
#define declaration_spec_flags (declaration_control.bytes + 4)
extern DefaultArgument *default_argument_storage[61];
#define default_arguments (default_argument_storage + 1)
extern CType *parameter_associated_type;
DeclarationParameterInfo *void_parameter_list(void);
DeclarationParameterInfo *intern_declaration_parameters(int count, CppSymbol *head);
FunctionParameters *__stdcall intern_function_parameters(FunctionParameters *parameters);
typedef char FunctionParametersSizeMustBe16[(sizeof(FunctionParameters) == 16) ? 1 : -1];
IrToken *__stdcall wrap_default_argument_symbols(IrToken *expression, ExpressionSymbolList *symbols);
IrToken *__stdcall wrap_default_argument_members(IrToken *expression, MemberReferenceGroup *groups);
void __stdcall collect_member_references(MemberReferenceGroup **groups, IrToken *expression);
#endif
