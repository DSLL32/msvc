#ifndef WPP_GSTACK_H
#define WPP_GSTACK_H
#include "class.h"
#include <recon.h>
/* Reached GS_DECL_SPEC variant of the108B carved union object. Other
 * alternatives remain undefined until the actual parser encounters them. */
typedef struct global_stack GLOBAL_STACK;
#pragma pack(4)
struct global_stack {
    GLOBAL_STACK *next;
    unsigned char id;
    union { struct decl_info *dinfo; struct decl_spec *dspec; CLASS_DATA classdata; void *pointer; } u;
};
#pragma pack()
typedef char GlobalStackSize[(sizeof(GLOBAL_STACK)==108 && offsetof(GLOBAL_STACK,u)==8)?1:-1];
GLOBAL_STACK *GStackPush(GLOBAL_STACK **head,unsigned char id);
void GStackPop(GLOBAL_STACK **head);
#endif
