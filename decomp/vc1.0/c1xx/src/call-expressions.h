#ifndef CALL_EXPRESSIONS_H
#define CALL_EXPRESSIONS_H
#include "parser.h"
#include <recon.h>
unsigned __stdcall count_call_arguments(IrToken *arguments);
CppSymbol *__stdcall resolve_function_overload(CppSymbol *functions, IrToken *object,
    IrToken *arguments, unsigned count, unsigned char diagnose);
void __stdcall bind_selected_overload(QualifiedDeclarator *reference, CppSymbol *function);
IrToken *__stdcall make_resolved_call(CppSymbol *function, IrToken *object,
    IrToken *arguments, unsigned count, CppSymbol *qualified);
IrToken *__stdcall cast_to_type(CType *type, IrToken *operand);
int __stdcall resolve_function_style_cast(IrToken *expression);
IrToken *__stdcall find_class_conversion(CType *target, CType *source, unsigned *rank);
IrToken *__stdcall lower_member_pointer_call(IrToken *expression, IrToken **prefix);
unsigned __stdcall validate_call_arguments(CType *type, TypeShape *function,
    IrToken *arguments, unsigned count);
IrToken *__stdcall convert_call_arguments(CppSymbol *function, FunctionParameters *parameters,
    IrToken *arguments, unsigned count);
IrToken *__stdcall finish_call_result(IrToken *call, CppSymbol *function, unsigned short flags);
int __stdcall member_call_is_nonvirtual(IrToken *expression);
int __stdcall is_this_object_expression(IrToken *expression);
typedef struct OverloadArgumentRank {
    unsigned rank;
    CType *parameter_type;
    unsigned field08;
} OverloadArgumentRank;
typedef struct OverloadCandidate {
    CppSymbol *function;
    CType **parameter_cursor;
    unsigned origin;
    CType *current_parameter, *this_type;
    struct OverloadCandidate *next, *previous;
    OverloadArgumentRank *ranks;
    unsigned field20, field24, field28, field2c;
    unsigned char flags, reserved31[3];
} OverloadCandidate;
typedef struct OverloadSearch {
    unsigned total_count, viable_count, argument_count;
    unsigned char field0c, reserved0d[3];
    CppSymbol *functions;
    OverloadCandidate *storage, *candidates;
    OverloadArgumentRank *rank_storage;
    OverloadCandidate *current;
    OverloadArgumentRank *rank_cursor;
    unsigned field28, field2c;
    unsigned char flags, phase, reserved32[2];
} OverloadSearch;
typedef char OverloadCandidateSizeMustBe52[(sizeof(OverloadCandidate) == 52) ? 1 : -1];
typedef char OverloadSearchSizeMustBe52[(sizeof(OverloadSearch) == 52) ? 1 : -1];
typedef char OverloadRankSizeMustBe12[(sizeof(OverloadArgumentRank) == 12) ? 1 : -1];
typedef char OverloadSearchCurrentOffsetMustBe32[(offsetof(OverloadSearch, current) == 32) ? 1 : -1];
typedef char OverloadCandidateFlagsOffsetMustBe48[(offsetof(OverloadCandidate, flags) == 48) ? 1 : -1];
typedef OverloadCandidate *(__stdcall *OverloadFailure)(OverloadSearch *search);
OverloadSearch *new_overload_search(CppSymbol *functions, unsigned count,
    unsigned flags, ...);
int __stdcall validate_overload_counts(OverloadSearch *search);
int __stdcall rank_overload_arguments(OverloadSearch *search, IrToken *object, IrToken *arguments);
OverloadCandidate *__stdcall select_overload_candidate(OverloadSearch *search,
    unsigned *rank, OverloadFailure failure);
void __stdcall destroy_overload_search(OverloadSearch *search);
OverloadCandidate *__stdcall report_call_overload_failure(OverloadSearch *search);
unsigned __stdcall count_overload_functions(CppSymbol *functions);
int __stdcall add_overload_candidates(OverloadSearch *search, CppSymbol *functions, unsigned origin);
void __stdcall begin_overload_candidate_cursor(OverloadSearch *search,
    OverloadCandidate **candidate, OverloadArgumentRank **ranks);
void __stdcall save_overload_candidate_cursor(OverloadSearch *search,
    OverloadCandidate *candidate, OverloadArgumentRank *ranks);
void set_overload_phase(OverloadSearch *search, unsigned phase, ...);
unsigned __stdcall rank_call_argument_count(TypeShape *function, unsigned count);
int __stdcall reject_overload_candidate(OverloadCandidate *candidate,
    OverloadSearch *search, IrToken *argument);
#endif
