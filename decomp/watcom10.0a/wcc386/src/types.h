#ifndef WCC_TYPES_H
#define WCC_TYPES_H
#include <recon.h>
/* This version predates long-long; plain char is a later typedef alias. */
typedef enum DATA_TYPE {
    TYPE_CHAR,TYPE_UCHAR,TYPE_SHORT,TYPE_USHORT,TYPE_INT,TYPE_UINT,
    TYPE_LONG,TYPE_ULONG,TYPE_FLOAT,TYPE_DOUBLE,TYPE_POINTER,TYPE_ARRAY,
    TYPE_STRUCT,TYPE_UNION,TYPE_FUNCTION,TYPE_FIELD,TYPE_VOID,TYPE_ENUM,
    TYPE_TYPEDEF,TYPE_UFIELD,TYPE_DOT_DOT_DOT,TYPE_PLAIN_CHAR
} DATA_TYPE;
typedef struct type_definition TYPEDEFN;
typedef TYPEDEFN *TYPEPTR;
/* The 1994 node is 20 bytes: its enum and state occupy one byte each.
 * Explicit byte fields retain that representation in the native build too. */
struct type_definition {
    unsigned char decl_type;
    unsigned char type_flags;
    unsigned short decl_flags;
    TYPEPTR object;
    TYPEPTR next_type;
    unsigned type_index;
    union {
        void *tag;
        unsigned short typedefn;
        struct {
            short segment;
            unsigned short based_sym;
        } p;
    } u;
};
extern TYPEPTR TypeHead,FuncTypeHead[6],BaseTypes[22];
extern TYPEPTR StringType,StringArrayType,VoidParmList[2],ConstCharType;
extern int CTypeCounts[21];
TYPEPTR TypeNode(DATA_TYPE base_type,TYPEPTR object);
TYPEPTR TypeDefault(void);
TYPEPTR DupType(TYPEPTR typ,unsigned short flags,int force_duplicate);
TYPEPTR GetType(DATA_TYPE base_type);
TYPEPTR PtrNode(TYPEPTR object,unsigned flags,int segment);
TYPEPTR MkPtrNode(TYPEPTR object,unsigned flags,int segment,unsigned short based_sym);
#endif
