#include "global-optimizer.h"
#include <recon.h>














































void __fastcall rewrite_global_aliases(C2Function *function, int mode)
{
    __asm { int 3 }
}








C2BranchTarget * __fastcall get_symbol_branch_target(C2BranchTarget *target, unsigned char kind)
{
    for (target = target->previous; target; target = target->previous)
        if (target->kind == kind) return target;
    return NULL;
}


void __fastcall release_global_record(C2Expression *record)
{
    __asm { int 3 }
}

void __fastcall remove_global_successor(C2BasicBlock *block, C2BasicBlock *destination, C2Function *function)
{
    __asm { int 3 }
}

int __fastcall evaluate_integer_comparison(int signed_compare, unsigned width, unsigned __int64 left, unsigned __int64 right, unsigned char condition)
{
    __asm { int 3 }
    return 0;
}







void __fastcall append_global_attribute(C2Expression *record, unsigned short attribute, unsigned char flag)
{
    __asm { int 3 }
}

int __fastcall global_rewrite_order_is_valid(C2Expression *use, C2Expression *definition)
{
    __asm { int 3 }
    return 0;
}

int __fastcall global_range_changes_operand(C2Expression *first, C2Expression *last, C2Expression *operand)
{
    __asm { int 3 }
    return 0;
}











int __fastcall global_range_uses_operand(C2Expression *first, C2Expression *last, C2Expression *operand)
{
    __asm { int 3 }
    return 0;
}


void __fastcall prune_global_rewrite_registers(C2FunctionRegister *reg)
{
    __asm { int 3 }
}


void __fastcall collect_global_definition_sets(C2FunctionRegister *reg)
{
    __asm { int 3 }
}










void __fastcall detach_global_result(C2Expression *record)
{
    __asm { int 3 }
    
}

C2FunctionRegister * __fastcall new_global_intrinsic_register(C2Expression *record)
{
    __asm { int 3 }
    return NULL;
}

C2FunctionRegister * __cdecl new_global_call_register(unsigned opcode, unsigned short type, unsigned count, ...)
{
    __asm { int 3 }
    return NULL;
}

























C2Expression * __fastcall optimize_limited_global_record(C2Expression *record, C2BasicBlock *block, C2Function *function)
{
    __asm { int 3 }
    return NULL;
}












C2Expression * __fastcall new_linked_global_expression(unsigned opcode, unsigned short type, C2Expression *operand, C2InputLink link, C2Record *prior)
{
    __asm { int 3 }
    return NULL;
}

void __fastcall make_global_register_explicit(C2FunctionRegister *reg)
{
    __asm { int 3 }
    
}






void __fastcall invalidate_global_assignment_uses(C2FunctionRegister *reg, C2FunctionRegister *canonical)
{
    __asm { int 3 }
}



C2Expression *__fastcall new_global_selection_expression(unsigned opcode, unsigned short type, C2Expression *result, C2Expression *condition, C2Expression *first, C2Expression *second, unsigned char selector, C2InputLink link, C2Record *prior)
{
    __asm { int 3 }
    return NULL;
}













void __fastcall invalidate_global_available_variants(C2FunctionRegister *reg, C2FunctionRegister *canonical)
{
    __asm { int 3 }
}















void __fastcall mark_global_loop_dependents(C2FunctionRegister *reg)
{
    __asm { int 3 }
}









unsigned __fastcall global_register_seen(C2FunctionRegister *reg, C2RegisterSet *available)
{
    __asm { int 3 }
    return 0;
}





















void __fastcall finish_global_detached_record(C2Expression *record)
{
    __asm { int 3 }
}

unsigned short __fastcall global_intrinsic_argument_type(C2Expression *record, unsigned *bytes)
{
    __asm { int 3 }
    return 0;
}

void __fastcall rewrite_global_intrinsic_type(C2Expression *record, unsigned short type)
{
    __asm { int 3 }
}











C2Expression *__fastcall build_final_shift(C2Expression *operand,
    C2Expression *previous, unsigned short type, int mode, unsigned shift)
{
    __asm { int 3 }
    return NULL;
}

















void __fastcall compact_final_block(C2BasicBlock *block)
{
    __asm { int 3 }
}

void __fastcall rewrite_wide_final_multiply(C2Expression *record)
{
    __asm { int 3 }
}





int __fastcall final_register_has_zero_bits(C2FunctionRegister *reg, unsigned bits)
{
    __asm { int 3 }
    return 0;
}
void __fastcall remove_final_conversion(C2Expression *record)
{
    __asm { int 3 }
}





















/* Distinct genuine region-candidate callees. Names and unused caller-register
 * views are provisional until each actual body is recovered; no ABI/body
 * qualification follows from the INT3 entry declarations. */


































































int __cdecl check_region_integer_operation(unsigned left, unsigned right, int sign, int bytes)
{
    __asm { int 3 }
    return 0;
}





























int __fastcall region_operand_is_narrow_candidate(C2Expression *operand)
{
    __asm { int 3 }
    return 0;
}





C2Expression * __fastcall region_intrinsic_operand(unsigned index, C2Expression *record)
{
    __asm { int 3 }
    return NULL;
}



C2Expression * __fastcall find_region_index_operand(C2Expression *operand)
{
    __asm { int 3 }
    return NULL;
}
























C2FunctionRegister *__fastcall convert_region_register(C2FunctionRegister *reg,
    unsigned short type, unsigned first, unsigned second)
{
    __asm { int 3 }
    return NULL;
}








C2Expression * __fastcall find_region_address_update(C2FunctionRegister *reg)
{
    __asm { int 3 }
    return NULL;
}



unsigned short __fastcall region_copy_intrinsic_type(C2Expression *record, unsigned *flags)
{
    __asm { int 3 }
    return 0;
}

int __fastcall region_copy_conflicts(C2Expression *destination, C2Expression *source, C2Expression *record, C2FlowRegion *region)
{
    __asm { int 3 }
    return 0;
}

C2Expression * __fastcall build_region_copy_count(C2FlowRegion *region, C2Function *function, unsigned __int64 bytes)
{
    __asm { int 3 }
    return NULL;
}

void __fastcall emit_region_memory_copy(C2Expression *destination, unsigned destination_attributes, C2Expression *source, unsigned source_attributes, unsigned short type, unsigned char flags, C2Expression *count, C2Record *prior)
{
    __asm { int 3 }
    
}

C2Expression *__fastcall find_region_independent_value(C2Expression *first,
    C2Expression *second)
{
    __asm { int 3 }
    return NULL;
}
C2Expression *__fastcall reduce_region_difference(C2Expression *value,
    C2Expression *first, C2Expression *difference)
{
    __asm { int 3 }
    return NULL;
}
int __fastcall region_value_depends_on(C2Expression *value, C2Expression *input)
{
    __asm { int 3 }
    return 0;
}
C2Expression *__fastcall new_region_iteration(C2Expression *value, unsigned short type,
    C2Expression *step, unsigned opcode, C2FunctionRegister *source,
    unsigned mode, C2FlowRegion *region, C2Function *function)
{
    __asm { int 3 }
    return NULL;
}








struct C2RegionComparisonChoice;


C2Expression *__fastcall build_region_comparison_operand(unsigned short type,
    unsigned opcode, C2Expression *left, C2Expression *right, C2FlowRegion *region)
{
    __asm { int 3 }
    return NULL;
}
int __fastcall region_definition_is_available(C2Expression *record, C2FunctionRegister *reg)
{
    __asm { int 3 }
    return 0;
}






int __fastcall region_operation_is_safe(unsigned short type, unsigned opcode,
    C2Expression *left, C2Expression *right, C2FlowRegion *region)
{
    __asm { int 3 }
    return 0;
}
C2Expression *__fastcall find_region_initial_expression(C2Expression *operand, C2FlowRegion *region)
{
    __asm { int 3 }
    return NULL;
}
struct C2RegionComparisonChoice *__fastcall new_region_comparison_choice(C2FunctionRegister *reg,
    C2Expression *operand, unsigned opcode, C2FunctionRegister *source)
{
    __asm { int 3 }
    return NULL;
}



int __fastcall compare_region_residues(C2Expression *value, C2Expression *initial, unsigned opcode)
{
    __asm { int 3 }
    return 0;
}
C2Expression *__fastcall combine_region_offsets(C2Expression *other, C2Expression *operand)
{
    __asm { int 3 }
    return NULL;
}
int __fastcall rewrite_region_aggregate_use(C2Expression *record, C2Expression *owner,
    C2Expression *operand, unsigned opcode, C2Expression *source, C2Expression *initial, unsigned apply)
{
    __asm { int 3 }
    return 0;
}
void __fastcall clear_region_value_uses(C2FunctionRegister *reg)
{
    __asm { int 3 }
}
void __fastcall remove_region_update_use(C2FunctionRegister *reg, C2Expression *record)
{
    __asm { int 3 }
}
void __fastcall rewrite_region_limit(C2Expression *record, C2Expression *bound,
    C2Expression *initial, unsigned opcode, C2Expression *source)
{
    __asm { int 3 }
}
void __fastcall materialize_region_saved_value(C2Expression *operand, C2FlowRegion *region)
{
    __asm { int 3 }
}
C2Expression *__fastcall materialize_region_aggregate(C2Expression *record, C2Expression *operand)
{
    __asm { int 3 }
    return NULL;
}



C2Expression *__fastcall new_region_typed_counter(C2FlowRegion *region,
    unsigned short type, C2Function *function)
{
    __asm { int 3 }
    return NULL;
}

















int __fastcall final_operand_canonical_zero_bits(C2Expression *operand)
{
    __asm { int 3 }
    return 0;
}

C2Expression *__fastcall resolve_final_single_use_value(C2Expression *record)
{
    __asm { int 3 }
    return NULL;
}

unsigned __fastcall final_parameter_stack_size(C2Expression *record)
{
    __asm { int 3 }
    return 0;
}


void __fastcall spill_final_float_call_stack(C2Expression *record, unsigned count, int mode)
{
    __asm { int 3 }
}





