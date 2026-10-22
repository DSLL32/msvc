#ifndef WORK_LIST_H
#define WORK_LIST_H
#include <recon.h>
/* Compiler work lists pair a link with one machine-word payload. */
typedef struct WorkList {
    struct WorkList *next;
    union { void *pointer; unsigned word; unsigned char byte; } value;
} WorkList;
WorkList *__stdcall push_work_list(WorkList **stack, int arena);
WorkList *__stdcall pop_work_list(WorkList **stack);
extern WorkList *scope_cleanup_stack;
typedef char WorkListSizeMustBe8[(sizeof(WorkList) == 8) ? 1 : -1];
#endif
