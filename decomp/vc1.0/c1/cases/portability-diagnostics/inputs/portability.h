#ifndef PORTABILITY_AUDIT_H
#define PORTABILITY_AUDIT_H
/* Recoverable compatibility warnings precede declaration errors. */
#define defined 17
#undef __LINE__
#pragma alloc_text(123, header_callback)
#pragma comment(123)
#pragma warning invalid

extern int *api_link;
extern int **api_link;
int;
static int = 3;
int mixed_parameters(value, int);
__declspec(naked) int header_naked_object;
#endif
