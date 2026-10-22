#ifndef RC_PREPARE_H
#define RC_PREPARE_H
#include "driver.h"
#include <recon.h>
extern int input_encoding;
int __stdcall detect_encoding(FILE *);
BOOL __stdcall read_source_line(char *, int, FILE *);
BOOL __stdcall prefix_matches(char *, char *);
char * __stdcall trim_token(char *);
#endif
