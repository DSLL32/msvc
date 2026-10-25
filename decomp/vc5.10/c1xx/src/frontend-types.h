#ifndef C1XX_FRONTEND_TYPES_H
#define C1XX_FRONTEND_TYPES_H
#include "c1xx.h"
#include <recon.h>
struct CompilerTypeShape;
struct CompilerInternedType;
struct CompilerMappedType;
struct CompilerCompositeType;
struct CompilerTypeSequence;
/* The head counts follow the actual hash masks in each lookup consumer.
 * Entry payloads stay opaque until those functions are reconstructed. */
typedef struct CompilerTypeHash { struct CompilerInternedType *buckets[2048]; } CompilerTypeHash;
typedef struct CompilerTypeMap { struct CompilerMappedType *buckets[1024]; } CompilerTypeMap;
typedef struct CompilerCompositeTypeHash { struct CompilerCompositeType *buckets[1024]; } CompilerCompositeTypeHash;
typedef struct CompilerTypeSequenceHash { struct CompilerTypeSequence *buckets[512]; } CompilerTypeSequenceHash;
typedef struct CompilerTypeTables {
    CompilerTypeHash *types; /* original ccb38 */
    CompilerTypeMap *mapped; /* cc434 */
    CompilerCompositeTypeHash *composite; /* cc42c */
    CompilerTypeSequenceHash *sequences; /* cc6bc */
} CompilerTypeTables;
typedef struct CompilerType {
    WORD flags, attributes;
    struct CompilerTypeShape *shape;
    WORD auxiliary;
    /* Original upper auxiliary WORD and final DWORD remain unmodeled. */
} CompilerType;
typedef struct CompilerBuiltinTypes {
    CompilerType *type_0000, *type_4041_first, *type_0063, *type_4063_first;
    CompilerType *type_0041, *type_4041_second, *type_0042, *type_4042_first;
    CompilerType *type_0043, *type_4043, *type_004c, *type_404c;
    CompilerType *type_0060, *type_0013;
    CompilerType *type_4063_second, *type_0042_second, *type_4042_second;
    CompilerType *type_0044, *type_4044, *type_0106, *type_0107, *type_0108;
    CompilerType *type_0802, *type_0080;
    CompilerType *type_0041_q0100, *type_4041_q0100, *type_0063_q0100;
    CompilerType *implicit_int_function, *tagged_0080;
} CompilerBuiltinTypes;
typedef struct CompilerLexerBackend {
    int enabled;
    /* Original +4 receives a later value; it is not initialized here. */
    struct CompilerBackend *backend;
} CompilerLexerBackend;
struct CompilerTextInput;
#endif
