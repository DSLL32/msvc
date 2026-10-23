/* Exact mixed union shape from the encountered ParserValue ABI. */
#include <stdio.h>
#include <string.h>
#include <recon.h>
typedef struct CType CType;
typedef struct IrToken IrToken;
typedef struct FrontendSymbol FrontendSymbol;
typedef struct ArgumentNode ArgumentNode;
typedef struct TypeShape TypeShape;
typedef struct StringToken { unsigned char *text; unsigned short bytes,flags; } StringToken;
typedef union TokenValue {
    unsigned char bytes[8]; struct { unsigned low,high; } words;
    double floating; StringToken string; char *identifier;
    IrToken *literal; FrontendSymbol *symbol;
} TokenValue;
typedef struct DeclarationSpec { unsigned char storage[3],reserved; CType *type; } DeclarationSpec;
typedef union ParserValue {
    TokenValue token; DeclarationSpec declaration;
    struct { FrontendSymbol *symbol; CType *type; } type_name;
    struct { unsigned low,high; } words;
    unsigned char bytes[8];unsigned short modifier;char *name;
    FrontendSymbol *symbol;IrToken *node;ArgumentNode *arguments;TypeShape *shape;
} ParserValue;
RECON_RETURN8_TYPE(ParserValue) __stdcall return_value(unsigned code);
