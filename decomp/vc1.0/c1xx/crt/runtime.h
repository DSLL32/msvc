#ifndef RUNTIME_H
#define RUNTIME_H
#include <time.h>
#include <recon.h>
/* Private CRT entries whose versions precede the shipped library. */
void __tzset(void);
int _isindst(struct tm *time);
#endif
