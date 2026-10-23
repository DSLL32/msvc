#ifndef RECON_NATIVE_MBSTRING_H
#define RECON_NATIVE_MBSTRING_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
int _mbsncmp(const unsigned char *,const unsigned char *,size_t);
unsigned char *_mbschr(const unsigned char *,unsigned int);
unsigned char *_mbsinc(const unsigned char *);
unsigned char *_mbsdec(const unsigned char *,const unsigned char *);
void _mbccpy(unsigned char *,const unsigned char *);
int _ismbcspace(unsigned int);
#ifdef __cplusplus
}
#endif
#endif
