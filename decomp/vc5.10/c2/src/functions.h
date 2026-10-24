#ifndef C2_FUNCTIONS_H
#define C2_FUNCTIONS_H
#include "c2.h"
#include <recon.h>

/* The controller proves these register arguments. The pass bodies and their
 * more specific names remain pending; hexadecimal suffixes identify the
 * original application entries without guessing their algorithms. */
extern C2FunctionSymbol *pending_functions;
void initialize_backend(void);
void initialize_pass_functions(void);
void __fastcall initialize_source_output(char *source);
unsigned char get_assembly_comment_character(void);
void __fastcall write_assembly_header(char *source);
extern int assembly_output_active;
extern unsigned char assembly_comment_character;
C2DebugLink *__fastcall prepare_function_records(C2Function *function);
void __fastcall process_records(C2DebugLink *records);
C2DebugLink *prepare_global_records(void);
int __fastcall has_local_symbol(unsigned serial);
int check_extended_backend(void);
unsigned short __fastcall listing_source_index(unsigned serial, unsigned source);
int prepare_deferred_function(void);
void finish_deferred_function(void);
void begin_function_input(void);
C2Function *__fastcall read_function(C2FunctionSymbol *symbol);
void __fastcall activate_scope(C2ScopeHash *scope);
void __fastcall canonicalize_function(C2Function *function);
int __fastcall skip_global_optimization(C2Function *function);
void __fastcall function_pass_409dac(C2Function *function, int mode);
void __fastcall function_pass_40d9be(C2Function *function, int mode);
void __fastcall function_pass_456857(C2Function *function, int mode);
void __fastcall function_pass_4570e0(C2Function *function, int mode);
void finish_function(void);
void __fastcall function_pass_463ff1(C2Function *function);
void __fastcall function_pass_456b13(C2Function *function);
void __fastcall function_pass_45729f(C2Function *function);
void __fastcall function_pass_458c9f(C2Function *function);
void __fastcall function_pass_46481c(C2Function *function);
void __fastcall function_pass_483f46(C2Function *function);
void __fastcall function_pass_4088c7(C2Function *function);
void __fastcall function_pass_427d89(C2Function *function);
void __fastcall function_pass_4828f8(C2Function *function);
void __fastcall function_pass_421bdf(C2Function *function);
void __fastcall function_pass_421e34(C2Function *function);
void __fastcall function_pass_409c5c(C2Function *function);
void __fastcall function_pass_42127d(C2Function *function);
void __fastcall function_pass_4180e8(C2Function *function);
void __fastcall function_pass_413245(C2Function *function);
void __fastcall function_pass_4132fa(C2Function *function);
void __fastcall function_pass_413369(C2Function *function);
void __fastcall function_pass_412292(C2Function *function);
void __fastcall function_pass_412412(C2Function *function);
void __fastcall function_pass_464b8a(C2Function *function);
void __fastcall function_pass_412457(C2Function *function);
void __fastcall function_pass_4120d4(C2Function *function);
void __fastcall function_pass_412d5c(C2Function *function);
void __fastcall function_pass_4144ef(C2Function *function);
void __fastcall function_pass_414862(C2Function *function);
void __fastcall function_pass_40d254(C2Function *function);
void __fastcall function_pass_414a64(C2Function *function);
void __fastcall function_pass_41759e(C2Function *function);
void __fastcall function_pass_416e6a(C2Function *function);
void __fastcall function_pass_42a8aa(C2Function *function);
void __fastcall function_pass_42b3e2(C2Function *function);
void __fastcall function_pass_42b750(C2Function *function);
void __fastcall function_pass_42e689(C2Function *function);
void __fastcall function_pass_483b6f(C2Function *function);
void __fastcall function_pass_42e6a6(C2Function *function);
void __fastcall function_pass_402cb6(C2Function *function);
void __fastcall function_pass_402b6e(C2Function *function);
void __fastcall function_pass_42d635(C2Function *function);
void __fastcall function_pass_42d65a(C2Function *function);
void __fastcall function_pass_42f060(C2Function *function);
void __fastcall function_pass_402c2c(C2Function *function);
void __fastcall function_pass_42f3e4(C2Function *function);
void __fastcall function_pass_42f446(C2Function *function);
void __fastcall function_pass_4330f7(C2Function *function);
void __fastcall function_pass_433546(C2Function *function);
void __fastcall function_pass_4338ab(C2Function *function);
void __fastcall function_pass_465486(C2Function *function);
void __fastcall function_pass_47d1d3(C2Function *function);
void __fastcall function_pass_433906(C2Function *function);
#endif
