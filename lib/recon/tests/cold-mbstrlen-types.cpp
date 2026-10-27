#include <stdlib.h>
#include <recon.h>
static size_t (__cdecl *typed)(const char *)=_mbstrlen;
int main(void) {return typed==0;}
