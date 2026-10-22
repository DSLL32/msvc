#ifndef PARAMETERS_H
#define PARAMETERS_H
#include "declarations.h"

typedef struct DeclarationParameterInfo {
    int count;
    unsigned short flags, field06;
    CType *types[1];
} DeclarationParameterInfo;
typedef struct DefaultArgument {
    unsigned field00, field04, field08, field0c;
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
extern int declaration_depth;
extern DefaultArgument *default_arguments[60];
extern CType *parameter_associated_type;
DeclarationParameterInfo *void_parameter_list(void);
DeclarationParameterInfo *intern_declaration_parameters(int count, CppSymbol *head);
FunctionParameters *__stdcall intern_function_parameters(FunctionParameters *parameters);
typedef char FunctionParametersSizeMustBe16[(sizeof(FunctionParameters) == 16) ? 1 : -1];
#endif
