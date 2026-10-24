#include "function-records.h"
#include "code-records.h"
#include <recon.h>

C2RegisterBlock **function_register_blocks;
C2RegisterBlock **next_function_register_block, **end_function_register_blocks;
C2RegisterBlock *first_function_register_block, *last_function_register_block;
unsigned next_function_register_number;
C2RegisterBlock *tracked_register_block, *secondary_register_block;
C2RegisterBlock *plain_register_block, *default_register_block, *extra_register_block;
C2FunctionRegister *free_plain_register_nodes, *free_tracked_register_nodes;
C2RegisterSet *secondary_register_set, *tracked_register_set;
C2FunctionRegister *empty_function_register;

void initialize_function_records(void)
{
    function_register_blocks = c2_pool_allocate(1, 1000 * sizeof(C2RegisterBlock *));
    next_function_register_block = function_register_blocks;
    end_function_register_blocks = function_register_blocks + 1000;
    next_function_register_number = 0;
    first_function_register_block = NULL;
    last_function_register_block = NULL;
    tracked_register_block = new_register_block();
    secondary_register_block = new_register_block();
    plain_register_block = new_register_block();
    default_register_block = new_register_block();
    extra_register_block = NULL;
    free_plain_register_nodes = NULL;
    free_tracked_register_nodes = NULL;
    pending_debug_symbols = NULL;
    secondary_register_set = new_register_set(1);
    tracked_register_set = new_register_set(1);
    empty_function_register = new_function_register(4);
    empty_function_register->kind = 0x10;
    clear_register_set(tracked_register_set);
}
