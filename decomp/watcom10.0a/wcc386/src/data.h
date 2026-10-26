#ifndef WCC_DATA_H
#define WCC_DATA_H
#include "symbols.h"
#include <recon.h>
/* The1994 data stream precedes the later union-first record. */
#pragma pack(1)
typedef struct data_quad {
    unsigned char opr,flags;
    union {
        unsigned long long_values[2];
        double double_value;
        struct { SYM_HANDLE sym_handle; unsigned long offset; } var;
    } u;
} DATA_QUAD;
#pragma pack()
extern DATA_QUAD *DataQuadSegs[641],*DataQuadPtr;
extern int DataQuadIndex,DataQuadSegIndex;
extern SYMPTR InitSym;
void GenDataQuad(DATA_QUAD *quad);
int StartDataQuadAccess(void);
DATA_QUAD *NextDataQuad(void);
void EmitDQuad(DATA_QUAD *quad);
void InitSymData(TYPEPTR type,int level);
int CharArray(TYPEPTR type);
int WCharArray(TYPEPTR type);
void InitCharArray(TYPEPTR type);
void InitWCharArray(TYPEPTR type);
FIELDPTR InitBitField(FIELDPTR field);
void ZeroBytes(unsigned long count);
void StoreInt(unsigned char token,unsigned maximum);
void ChkConstant(unsigned value,unsigned maximum);
void StoreIValue(unsigned char token,unsigned value);
void StorePointer(TYPEPTR type,int token,unsigned maximum);
void StoreFloat(int token);
void SetFarHuge(SYMPTR sym,int initializing);
void SetSegment(SYMPTR sym);
int SymSegId(SYMPTR sym);
unsigned FESymAttr(SYMPTR sym);
int IsConstType(TYPEPTR type);
void SetSegAlign(SYMPTR sym);
extern unsigned SegAlignment[11];
#endif
