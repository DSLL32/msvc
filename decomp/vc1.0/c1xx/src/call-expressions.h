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
IrToken *__stdcall lower_member_pointer_call(IrToken *expression, IrToken **prefix);
unsigned __stdcall validate_call_arguments(CType *type, TypeShape *function,
    IrToken *arguments, unsigned count);
IrToken *__stdcall convert_call_arguments(CppSymbol *function, FunctionParameters *parameters,
    IrToken *arguments, int count);
IrToken *__stdcall finish_call_result(IrToken *call, CppSymbol *function, unsigned short flags);
int __stdcall member_call_is_nonvirtual(IrToken *expression);
int __stdcall is_this_object_expression(IrToken *expression);
typedef struct OverloadArgumentRank {
    unsigned rank;
    CType *parameter_type;
    CppSymbol *conversion;
} OverloadArgumentRank;
struct OverloadCandidate {
    CppSymbol *function;
    CType **parameter_cursor;
    QualifiedDeclarator *qualification;
    CType *current_parameter, *this_type;
    struct OverloadCandidate *next, *previous;
    OverloadArgumentRank *ranks;
    unsigned conversion_index, field24, field28, field2c;
    unsigned char flags, reserved31[3];
};
typedef struct OverloadSearch {
    unsigned total_count, viable_count, argument_count;
    unsigned char operation, reserved0d[3];
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
typedef char OverloadCandidateQualificationOffsetMustBe8[(offsetof(OverloadCandidate, qualification) == 8) ? 1 : -1];
typedef char OverloadSearchOperationOffsetMustBe12[(offsetof(OverloadSearch, operation) == 12) ? 1 : -1];
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
int __stdcall add_overload_candidates(OverloadSearch *search, CppSymbol *functions, QualifiedDeclarator *qualification);
void __stdcall begin_overload_candidate_cursor(OverloadSearch *search,
    OverloadCandidate **candidate, OverloadArgumentRank **ranks);
void __stdcall save_overload_candidate_cursor(OverloadSearch *search,
    OverloadCandidate *candidate, OverloadArgumentRank *ranks);
void set_overload_phase(OverloadSearch *search, unsigned phase, ...);
unsigned __stdcall rank_call_argument_count(TypeShape *function, unsigned count);
int __stdcall reject_overload_candidate(OverloadCandidate *candidate,
    OverloadSearch *search, IrToken *argument);
void __stdcall diagnose_overload_failure(OverloadCandidate *candidate,
    OverloadSearch *search, IrToken *argument);
int __stdcall prune_overload_candidates(OverloadSearch *search, unsigned char phase, int mode);
OverloadCandidate *__stdcall resolve_overload_tie(OverloadSearch *search);
OverloadCandidate *__stdcall first_active_overload(OverloadCandidate *candidate);
typedef char OverloadCandidateConversionIndexOffsetMustBe32[
    (offsetof(OverloadCandidate, conversion_index) == 32) ? 1 : -1];
typedef char OverloadRankConversionOffsetMustBe8[
    (offsetof(OverloadArgumentRank, conversion) == 8) ? 1 : -1];
unsigned __stdcall candidate_argument_rank(OverloadCandidate *candidate, unsigned index, int mode);
int __stdcall compare_candidate_ranks(unsigned left_rank, OverloadCandidate *left,
    unsigned right_rank, OverloadCandidate *right, unsigned index);
int __stdcall defer_overload_candidate(OverloadCandidate *candidate, OverloadSearch *search);
unsigned __stdcall coarse_conversion_rank(unsigned rank);
void *__stdcall first_vector_item(Vector *functions);
void __stdcall register_temporary_destruction(IrToken *object, CType *type);
IrToken *__stdcall prepare_member_call_object(IrToken *object, CppSymbol *function,
    CppSymbol *qualified, CppSymbol **dispatch_class);
CppSymbol *__stdcall class_tag_for_result_type(CType *type);
int __stdcall find_constructor_result_reference(IrToken *call, IrToken **reference);
void __stdcall cancel_temporary_cleanup(unsigned index);
IrToken *__stdcall try_elide_copy_construction(CppSymbol *function, IrToken *object, IrToken *arguments);
IrToken *__stdcall simplify_trivial_member_call(CppSymbol *function, IrToken *object, IrToken *arguments);
void __stdcall report_call_argument_error(unsigned code, CppSymbol *function, unsigned index);
IrToken *__stdcall append_default_call_argument(IrToken *arguments, void *expression);
QualifierBinding *__stdcall find_member_call_binding(int implicit, CppSymbol *tag,
    CppSymbol *function, CppSymbol **dispatch_class);
QualifierBinding *__stdcall find_named_class_binding(CppSymbol *tag, char *name, unsigned categories);
CppSymbol *__stdcall create_initializer_temporary(unsigned char storage, CType *type);
void __stdcall record_initialized_temporary(CppSymbol *symbol, CType *type);
IrToken *__stdcall materialize_call_result(IrToken *call);
IrToken *__stdcall finish_constructor_result(IrToken *call, CppSymbol *tag);
CppSymbol *__stdcall select_allocation_function(unsigned short flags, CppSymbol *functions);
#endif
