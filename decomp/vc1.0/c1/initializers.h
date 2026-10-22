#ifndef INITIALIZERS_H
#define INITIALIZERS_H
#include "parser.h"
extern FrontendSymbol *initializer_symbol;
extern unsigned initializer_offset;
extern int initializer_runtime_override;
/* Encountered as three separately cleared words. Further meanings await
   reached consumers; preserve their shared contiguous reset storage. */
extern unsigned initializer_reset_words[3];
typedef struct InitializerFrame {
    FrontendSymbol *symbol;
    CType *type;
    unsigned offset;
    unsigned char flags, string_seen;
    unsigned short field0e;
} InitializerFrame;
extern InitializerFrame initializer_frames[32], *initializer_frame;
extern int initializer_depth, initializer_error_reported;
extern CType *initializer_inferred_type;
void __stdcall push_initializer_frame(FrontendSymbol *symbol);
int __stdcall is_scalar_initializer_symbol(FrontendSymbol *symbol);
#endif
