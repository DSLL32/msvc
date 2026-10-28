#include "storage.h"
#include <recon.h>
struct State state;
int evaluations;
static jmp_buf *owner_address(void){++evaluations;return &state.owner;}
__attribute__((noinline)) int initial_save(void){return setjmp(state.owner);}
int evaluated_save(void){return setjmp(*owner_address());}
Save save_address(void){return setjmp;}
Jump jump_address(void){return longjmp;}
void selected_jump(int value){longjmp(state.owner,value);}
