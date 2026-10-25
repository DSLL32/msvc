#ifndef DECORATED_NAMES_H
#define DECORATED_NAMES_H
#include <recon.h>
/* VC1 returns this eight-byte POD in EAX (next) and EDX (remaining).
   The native implementation object uses -freg-struct-return to retain that
   original ABI; callers of the POD-returning helpers need the same option. */
typedef struct NameBuffer { char *next; int remaining; } NameBuffer;
typedef char NameBufferSizeCheck[sizeof(NameBuffer)==8?1:-1];
typedef struct NameSlice { char *text; int length; } NameSlice;
typedef char NameSliceSizeCheck[sizeof(NameSlice)==8?1:-1];
extern NameBuffer invalid_name_buffer;
extern NameSlice decorated_tokens[10];
extern int decorated_last_token,decorated_scope_mode;
extern const char *decorated_operator_names[41];
NameSlice __fastcall scan_decorated_token(char **source);
char *__fastcall undecorate_function_name(char *name);
NameBuffer __fastcall decode_decorated_name(char *destination,char *source,int capacity);
#endif
