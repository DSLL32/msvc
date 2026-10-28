#include <win32.h>
#include <setjmp.h>
struct State {unsigned char prefix[3];jmp_buf owner;unsigned char tail[5];};
extern struct State state;
extern int evaluations;
int initial_save(void);
int evaluated_save(void);
typedef int (__cdecl *Save)(jmp_buf);
typedef void (__cdecl *Jump)(jmp_buf,int);
Save save_address(void);
Jump jump_address(void);
void selected_jump(int);
