#ifndef WLINK_RESOURCE_H
#define WLINK_RESOURCE_H
#include "link.h"
#include <recon.h>
#pragma pack(1)
struct WResRoutines {
    int (*open)( const char *, int, ... );
    int (*close)( int );
    int (*write)( int, const void *, unsigned );
    int (*read)( int, void *, unsigned );
    long (*seek)( int, long, int );
    long (*tell)( int );
    void *(*alloc)( unsigned );
    void (*free)( void * );
};
struct WResDirHead {
    unsigned short NumResources, NumTypes;
    struct WResTypeNode *Head, *Tail;
    unsigned short TargetOS;
};
struct WResIDName {
    unsigned char NumChars;
    char Name[1];
};
struct WResID {
    unsigned char IsName;
    union {
        unsigned short Num;
        WResIDName Name;
    } ID;
};
struct WResTypeInfo {
    unsigned short NumResources;
    WResID TypeName;
};
struct WResTypeNode {
    WResTypeNode *Next, *Prev;
    struct WResResNode *Head, *Tail;
    WResTypeInfo Info;
};
struct WResResInfo {
    unsigned short NumResources;
    WResID ResName;
};
struct WResResInfo1 {
    unsigned short MemoryFlags;
    unsigned long Offset, Length;
    WResID ResName;
};
struct WResResNode {
    WResResNode *Next, *Prev;
    struct WResLangNode *Head, *Tail;
    WResResInfo Info;
};
struct WResLangNode {
    struct WResLangNode *Next, *Prev;
    void *data;
    WResLangInfo Info;
};
typedef union ResNameOrOrdinal {
    struct { unsigned char fFlag; unsigned short wOrdinalID; } ord;
    char name[1];
} ResNameOrOrdinal;
typedef struct MResResourceHeader {
    ResNameOrOrdinal *Type, *Name;
    unsigned short MemoryFlags;
    unsigned long Size;
} MResResourceHeader;
typedef struct VarString {
    char partString[64];
    int lastLoc;
    struct VarString *next;
} VarString;
#pragma pack()
extern struct WResRoutines WResRtns;
int readMResDir( int handle, WResDir dir, int *dup_discarded );
MResResourceHeader *MResReadResourceHeader( int handle );
void MResFreeResourceHeader( MResResourceHeader *header );
WResID *WResIDFromNameOrOrd( const ResNameOrOrdinal *name );
WResID *WResIDFromNum( long number );
WResID *WResIDFromStr( const char *name );
WResTypeNode *newTypeNode( const WResID *type );
WResResNode *newResNode( const WResID *name );
WResLangNode *newLangNode( unsigned short flags, unsigned long offset,                            unsigned long length, WResLangType *lang );
int WResAddResource( const WResID *type, const WResID *name,                      unsigned short flags, unsigned long offset,                      unsigned long length, WResDir dir, WResLangType *lang,                      int *duplicate );
int ResReadUint8( unsigned char *value, int handle );
int ResReadUint16( unsigned short *value, int handle );
ResNameOrOrdinal *ResReadNameOrOrdinal( int handle );
VarString *VarStringStart( void );
void VarStringAddChar( VarString *list, int ch );
int ComputeVarStringLen( VarString *list );
char *VarStringEnd( VarString *list, int *retlength );
char *ResReadString( int handle, int *length );
#endif
