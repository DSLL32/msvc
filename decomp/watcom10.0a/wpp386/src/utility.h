#ifndef WPP_UTILITY_H
#define WPP_UTILITY_H
#include <recon.h>
char *stpcpy_after(char *target, const char **source);
char *stpcpy(char *target, const char *source);
char *stvcpy_after(char *target, const char **source, unsigned size);
char *stvcpy(char *target, const char *source, unsigned size);
char *strsave(const char *name);
int strpref(const char *prefix,const char *str);
char *vctsave(const char *vector, unsigned size);
unsigned char hex_dig(unsigned char c);
unsigned char octal_dig(unsigned char chr);
unsigned char escape_char(unsigned char chr);
unsigned char classify_escape_char(unsigned char chr);
#endif
