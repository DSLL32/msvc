#ifndef C2_CODE_INPUT_H
#define C2_CODE_INPUT_H
#include "function-reader.h"
#include "function-records.h"
#include <recon.h>

/* The stack producer allocates 28 bytes. Each element owns a tree-list sentinel,
 * its last tree, and deferred before/after stacks. */
struct C2InputStack {
    C2InputStack *next;
    C2Expression *first, *last;
    unsigned char flags, pending0d[3];
    C2Expression *alternatives;
    C2InputStack *before, *after;
};
typedef char C2InputStackSizeCheck[sizeof(C2InputStack) == 28 ? 1 : -1];
typedef char C2InputStackFlagsCheck[offsetof(C2InputStack, flags) == 12 ? 1 : -1];
typedef char C2InputStackAfterCheck[offsetof(C2InputStack, after) == 24 ? 1 : -1];

/* Token payloads overlap according to the input opcode. The reader uses the
 * scalar words, symbol/filename argument and control selector separately. */
typedef struct C2ILToken {
    unsigned opcode;
    unsigned short type, flags;
    unsigned bytes, pending0c;
    union {
        struct {
            unsigned low, high;
            union { C2InputSymbol *symbol; char *text; void *pointer; } argument;
            union {
                unsigned control;
                unsigned char control_byte;
                short control_short;
                C2Symbol *second_symbol;
            };
        } scalar;
        C2Real real;
        unsigned __int64 integer;
    } value;
} C2ILToken;
typedef char C2ILTokenSizeCheck[sizeof(C2ILToken) == 32 ? 1 : -1];
typedef char C2ILTokenPayloadCheck[offsetof(C2ILToken, value) == 16 ? 1 : -1];

typedef struct C2InputClause {
    struct C2InputClause *next;
    unsigned pending04;
    C2Expression *first, *last, *start, *end;
    void *label;
    unsigned pending1c;
} C2InputClause;

typedef struct C2InputHandler {
    C2Expression *body;
    void *filter;
    unsigned kind, pending0c;
} C2InputHandler;

typedef char C2InputClauseSizeCheck[sizeof(C2InputClause) == 32 ? 1 : -1];
typedef char C2InputHandlerSizeCheck[sizeof(C2InputHandler) == 16 ? 1 : -1];

/* Constructors receive the genuine fastcall linking callback and its caller's
 * predecessor. The callback arguments are present even where decompiler type
 * inference omitted the final stack word. */
typedef void (__fastcall *C2InputLink)(C2Record *, C2Record *);
void __fastcall release_small_node(void *node);
C2Expression * __fastcall new_integer_expression(unsigned short type, unsigned __int64 value);
C2Expression * __fastcall new_binary_expression(unsigned opcode, unsigned short type, C2Expression *right, C2Expression *left, C2InputLink link, C2Record *prior);
C2Expression * __fastcall copy_input_expression(C2Expression *expression);
C2Expression * __fastcall attach_input_expression(C2Expression *owner, C2Expression **slot, C2Expression *expression);
void __fastcall unlink_input_record(C2Record *record);
C2Expression * __fastcall copy_input_operand(C2Expression *owner, C2Expression *value);
C2Expression * __fastcall new_temporary_operand(unsigned short type);
C2Expression * __fastcall new_assignment_expression(unsigned opcode, unsigned short type, C2Expression *operand, C2InputLink link, C2Record *prior);
C2Expression * __fastcall new_control_expression(unsigned opcode);
C2FunctionRegister * __fastcall new_address_register(C2FunctionRegister *source, unsigned short type, unsigned attributes, C2FunctionRegister *base);
C2Expression * __fastcall convert_input_expression(C2Expression **expression, unsigned short type, C2Expression **prior);
C2Expression * __fastcall new_call_expression(unsigned opcode, unsigned char convention, C2Expression *target, C2Expression *arguments, C2Expression *result, C2InputLink link, C2Record *prior);
C2Expression * __fastcall symbol_input_expression(C2InputSymbol *symbol);
C2Expression * __fastcall new_aggregate_expression(unsigned opcode, unsigned short type, C2Expression *target, void *detail, C2Expression *source, C2Expression *destination, unsigned attributes);
int __fastcall set_input_expression_type(C2Expression *expression, unsigned short type);
C2Expression * __fastcall new_function_exit(C2Function *function);
void __fastcall read_input_type(C2ILToken *token);
void __fastcall read_control_fields(C2ILToken *token);
short __fastcall read_counted_input_text(char *text, int capacity);
char *__fastcall lookup_input_filename(unsigned serial);
unsigned __int64 read_input_longlong(void);
unsigned __int64 read_input_qword(void);
C2Real read_input_real(void);
int input_real_kind(C2Real *value);
int input_real_to_float(C2Real *input, float *output);
int input_real_to_double(C2Real *input, double *output);
void input_real_from_float(float *input, C2Real *output);
void input_real_from_double(double *input, C2Real *output);
extern int input_end_of_module;
C2ILToken *__fastcall decode_input_token(unsigned opcode, C2Function *function, C2ILToken *token);
C2ILToken * __fastcall read_input_token(C2Function *function, C2ILToken *token);
C2InputStack *new_input_stack(void);
void __fastcall push_input_trees(C2InputStack **stack, C2Expression *trees);
C2Record * __fastcall append_input_trees(C2InputStack *stack, C2Record *prior);
C2Expression * __fastcall pop_input_trees(C2InputStack **stack);
unsigned __fastcall resolve_input_source_line(unsigned serial);
unsigned __fastcall adjust_input_return_size(unsigned short type, unsigned bytes);
C2Expression * __fastcall convert_input_operand(C2InputStack *stack, unsigned short type, unsigned bytes, int flags);
int __fastcall needs_input_return_storage(unsigned convention, unsigned short type, unsigned bytes);
C2Expression * __fastcall pop_input_expression(C2InputStack *stack);
C2Expression * __fastcall variable_input_expression(C2InputSymbol *symbol);
C2Expression * __fastcall new_register_expression(unsigned opcode, C2FunctionRegister *reg);
void __fastcall mark_first_input_expression(C2Expression *expression);
unsigned __fastcall set_input_filename(char *filename);
C2Expression * __fastcall new_function_entry(C2Function *function);
void __fastcall set_function_input_options(unsigned options);
C2Expression * __fastcall input_symbol_operand(C2InputSymbol *symbol);
void __fastcall finish_input_expression(C2InputStack *stack);
void __fastcall input_operand_pair(C2InputStack **stack, C2Expression **left, C2Expression **right, int condition);
void __fastcall input_binary_operator(C2InputStack **stack, unsigned opcode);
C2Expression * __fastcall new_input_expression(unsigned opcode, unsigned short type);
C2Expression * __fastcall new_transfer_expression(unsigned opcode, unsigned short type, C2Expression *left, C2Expression *right, C2InputLink link, C2Record *prior);
void __fastcall swap_input_stacks(C2InputStack **stack);
void __fastcall input_shift_operator(C2InputStack **stack, C2ILToken *token);
unsigned char __fastcall input_comparison(C2InputStack **stack);
void __fastcall adjust_input_operand(C2Expression *operand);
void __fastcall input_call(C2InputStack **stack, C2ILToken *token, int depth, C2Function *function, int arguments);
void __fastcall input_pragma(C2InputStack *stack, C2ILToken *token);
void __fastcall set_input_operand_type(C2InputStack *stack, unsigned short type);
C2Expression * __fastcall save_input_expression(C2InputStack *stack);
void __fastcall input_assignment_operator(C2InputStack **stack, unsigned opcode, C2ILToken *token);
C2Expression * __fastcall new_input_temporary(unsigned short type);
void __fastcall bind_input_temporary(C2FunctionRegister *reg, C2Expression *expression);
void __fastcall input_argument_list(C2InputStack **stack);
C2Expression * __fastcall finish_input_clauses(C2InputClause *clauses, C2Expression *expression);
C2InputSymbol * __fastcall new_input_local(C2ScopeHash *scope, unsigned short attributes, unsigned alignment, short kind, int pool, unsigned char source_kind, unsigned bytes);
C2Expression * __fastcall convert_input_pointer(C2InputStack *stack, unsigned short type);
void __fastcall set_input_warning_state(unsigned short *state);
C2Expression * __fastcall new_block_copy_expression(unsigned opcode, unsigned short type, C2Expression *left, C2Expression *right, C2Expression *bytes, C2InputLink link, C2Record *prior);
unsigned __fastcall input_type_alignment(unsigned type);
C2Expression * __fastcall new_scalar_expression(unsigned opcode, unsigned value);
void __fastcall append_deferred_input(C2InputStack *stack);
void __fastcall finish_input_temporaries(C2InputStack *stack);
C2InputSymbol * __fastcall new_input_storage(C2Function *function, unsigned kind, unsigned alignment, unsigned bytes);
void __fastcall input_unary_operator(C2InputStack **stack, unsigned opcode);
void __fastcall input_condition(C2InputStack **stack, C2Expression *previous, C2ILToken *token);
C2Expression * __fastcall copy_input_expression_list(C2Expression *expressions);
C2Expression * __fastcall new_real_expression(unsigned short type, C2Real *value);
void __fastcall input_exception(C2InputStack **stack, int depth, C2Function *function);
void __fastcall input_sequence_point(C2InputStack *stack, C2Expression *expression);
C2Expression * __fastcall parse_input_assembly(unsigned char *text, C2ScopeHash *scope);
C2Expression * __fastcall new_input_assembly_record(unsigned opcode, unsigned short type);
void __fastcall input_aggregate_return(C2InputStack **stack, C2ILToken *token);
void __fastcall finish_input_declaration(C2InputStack *stack);
#endif
