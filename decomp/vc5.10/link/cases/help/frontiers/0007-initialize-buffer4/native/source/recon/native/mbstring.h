#ifndef RECON_NATIVE_MBSTRING_H
#define RECON_NATIVE_MBSTRING_H
#include <stddef.h>
int _mbsncmp(const unsigned char *,const unsigned char *,size_t);
unsigned char *_mbsinc(const unsigned char *);
void _mbccpy(unsigned char *,const unsigned char *);
int _ismbcspace(unsigned int);
#endif
