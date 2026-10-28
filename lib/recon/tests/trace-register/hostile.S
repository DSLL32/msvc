.text
.globl __cyg_profile_func_enter
.type __cyg_profile_func_enter,@function
__cyg_profile_func_enter:
 call 1f
1: popl %eax
 addl $_GLOBAL_OFFSET_TABLE_+(.-1b),%eax
 movl hook_calls@GOT(%eax),%ecx
 incl (%ecx)
 movl observed_function@GOT(%eax),%ecx
 movl 4(%esp),%edx
 movl %edx,(%ecx)
 movl observed_caller@GOT(%eax),%ecx
 movl 8(%esp),%edx
 movl %edx,(%ecx)
 fninit
 fldpi
 pcmpeqd %xmm0,%xmm0
 pcmpeqd %xmm7,%xmm7
 movl $0xfedcba98,%eax
 movl $0xdeadbeef,%ecx
 movl $0x87654321,%edx
 clc
 ret
.size __cyg_profile_func_enter,.-__cyg_profile_func_enter
.globl __cyg_profile_func_exit
.type __cyg_profile_func_exit,@function
__cyg_profile_func_exit:
 ud2
.size __cyg_profile_func_exit,.-__cyg_profile_func_exit
.section .note.GNU-stack,"",@progbits
