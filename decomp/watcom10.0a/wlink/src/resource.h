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
#pragma pack(1)
struct WResHeader {
    unsigned long Magic[2], DirOffset;
    unsigned short NumResources, NumTypes, WResVer;
};
struct WResExtHeader {
    unsigned short TargetOS, reserved[4];
};
#pragma pack()
typedef char WResHeader_is_18[(sizeof(WResHeader)==18)?1:-1];
typedef char WResExtHeader_is_10[(sizeof(WResExtHeader)==10)?1:-1];


int DefaultConversion( int, const char *, char * );
extern int (*ConvToUnicode)( int, const char *, char * );
int ResWriteUint8( const unsigned char *, int );
int ResWriteUint16( const unsigned short *, int );
int ResWriteUint32( const unsigned long *, int );
int WResWriteWResIDNameUni( const WResIDName *, unsigned char, int );
int WResWriteWResIDName( const WResIDName *, int );
int WResWriteWResID( const WResID *, int );
int WResWriteTypeRecord( const WResTypeInfo *, int );
int WResWriteResRecord( const WResResInfo *, int );
int WResWriteLangRecord( const WResLangInfo *, int );
int WResWriteHeaderRecord( const WResHeader *, int );
int WResWriteExtHeader( const WResExtHeader *, int );
int ResWriteString( const char *, unsigned char, int );
int ResWriteNameOrOrdinal( const ResNameOrOrdinal *, unsigned char, int );
int MResWriteResourceHeader( const MResResourceHeader *, int );
ResNameOrOrdinal *ResRead32NameOrOrdinal( int );
char *ResRead32String( int, int * );
#endif
