#include "global-optimizer.h"
#include <recon.h>












































void __fastcall finish_global_region(C2FlowRegion *region)
{
    __asm { int 3 }
}

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

void __fastcall swap_global_operands(C2Expression *record)
{
    __asm { int 3 }
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

















int __fastcall classify_global_register(C2RegisterSet *available, C2FunctionRegister *reg)
{
    __asm { int 3 }
    return 0;
}







C2Expression * __fastcall optimize_limited_global_record(C2Expression *record, C2BasicBlock *block, C2Function *function)
{
    __asm { int 3 }
    return NULL;
}





void __fastcall merge_global_register_identity(C2FunctionRegister *destination, C2FunctionRegister *source)
{
    __asm { int 3 }
    
}


int __fastcall global_operand_rejects_substitution(C2Expression *operand)
{
    __asm { int 3 }
    return 0;
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

C2Expression *__fastcall new_global_intrinsic_expression(unsigned opcode, unsigned short type, C2Expression *result, C2Expression *first, C2Expression *second, C2Expression *third, C2Expression *fourth, unsigned short intrinsic, C2InputLink link, C2Record *prior)
{
    __asm { int 3 }
    return NULL;
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



void __fastcall finish_region_selection(C2Expression *record, C2BasicBlock *block)
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










int __fastcall register_may_alias_index(C2FunctionRegister *reg, unsigned index)
{
    __asm { int 3 }
    return 0;
}

int __fastcall optimizer_register_may_alias_index(C2FunctionRegister *reg, unsigned index)
{
    __asm { int 3 }
    return 0;
}




C2Expression *__fastcall previous_global_code_record(C2Expression *record)
{
    __asm { int 3 }
    return 0;
}

void __fastcall normalize_global_call_operands(C2Expression *record)
{
    __asm { int 3 }
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






void __fastcall rewrite_final_binary_assignment(C2Expression *record,
    C2Expression *first, C2Expression *second, C2Expression *definition)
{
    __asm { int 3 }
}


C2Expression *__fastcall assignment_identity_operand(C2Expression *record)
{
    __asm { int 3 }
    return 0;
}

C2Expression *__fastcall simplify_assignment_rewrite(C2Expression *record)
{
    __asm { int 3 }
    return 0;
}





void __fastcall compact_final_block(C2BasicBlock *block)
{
    __asm { int 3 }
}

void __fastcall rewrite_wide_final_multiply(C2Expression *record)
{
    __asm { int 3 }
}
int __fastcall simplify_final_record(C2Expression *record, C2Expression *operand)
{
    __asm { int 3 }
    return 0;
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






void __fastcall rewrite_final_record_width(C2Expression *record, int bytes)
{
    __asm { int 3 }
}














/* Distinct genuine region-candidate callees. Names and unused caller-register
 * views are provisional until each actual body is recovered; no ABI/body
 * qualification follows from the INT3 entry declarations. */






int __fastcall region_candidate_pass_3d4a4(C2FlowRegion *region, C2Function *function)
{
    __asm { int 3 }
    return 0;
}

unsigned __fastcall region_candidate_pass_3d93c(C2FlowRegion *region, C2Function *function)
{
    __asm { int 3 }
    return 0;
}

void __fastcall region_candidate_pass_3de8b(C2FlowRegion *region, C2Function *function, unsigned changes)
{
    __asm { int 3 }
}

void __fastcall region_candidate_pass_3c245(C2FlowRegion *region, C2Function *function)
{
    __asm { int 3 }
}

void __fastcall region_candidate_pass_3c33f(C2FlowRegion *region)
{
    __asm { int 3 }
}

void __fastcall region_candidate_pass_3c3a0(C2FlowRegion *region, C2Function *function)
{
    __asm { int 3 }
}

void __fastcall region_candidate_pass_3c4a5(C2FlowRegion *region, C2Function *function)
{
    __asm { int 3 }
}

void __fastcall region_candidate_pass_3c87c(C2FlowRegion *region, C2Function *function)
{
    __asm { int 3 }
}

void __fastcall region_candidate_pass_3bbb3(C2FlowRegion *region)
{
    __asm { int 3 }
}

void __fastcall region_candidate_pass_3e5ed(C2FlowRegion *region, C2Function *function)
{
    __asm { int 3 }
}

void __fastcall region_candidate_pass_3e73e(C2FlowRegion *region, C2Function *function)
{
    __asm { int 3 }
}

void __fastcall region_candidate_pass_3e91d(C2FlowRegion *region)
{
    __asm { int 3 }
}

void __fastcall region_candidate_pass_3e9fe(C2FlowRegion *region, C2Function *function)
{
    __asm { int 3 }
}

void __fastcall region_candidate_pass_3ea57(C2FlowRegion *region, C2Function *function)
{
    __asm { int 3 }
}

void __fastcall region_candidate_pass_3ec06(C2FlowRegion *region, C2Function *function)
{
    __asm { int 3 }
}

void __fastcall region_candidate_pass_3ec67(C2FlowRegion *region, C2Function *function)
{
    __asm { int 3 }
}

void __fastcall region_candidate_pass_3edd3(C2FlowRegion *region)
{
    __asm { int 3 }
}

C2Expression *__fastcall new_region_binary_expression(unsigned opcode,
    unsigned short type, C2Expression *left, C2Expression *right,
    C2InputLink link, C2Record *prior)
{
    __asm { int 3 }
    return NULL;
}






