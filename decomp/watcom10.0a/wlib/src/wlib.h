/* Application declarations for the 1994 WLIB reconstruction.
   Names other than main and documented resource/CRT APIs are descriptive. */
#ifndef WLIB_RECONSTRUCTION_H
#define WLIB_RECONSTRUCTION_H
#include <malloc.h>
#include <signal.h>
#include <setjmp.h>
#include <recon.h>

typedef char WlibJumpBufferSize[(sizeof(jmp_buf) == 52) ? 1 : -1];
extern jmp_buf Env;                 /* Original VA 0x430014. */
extern unsigned long ToolFlags;     /* Original VA 0x4302b8. */

typedef struct handle_info {
    int handle;
    const char *filename;
} HANDLE_INFO;
typedef char WlibResourceHandleSize[(sizeof(HANDLE_INFO) == 8) ? 1 : -1];
#pragma pack(1)
typedef struct WResID {
    unsigned char IsName;
    union {
        unsigned short Num;
        struct {
            unsigned char NumChars;
            char Name[1];
        } Name;
    } ID;
} WResID;
typedef struct WResTypeInfo {
    unsigned short NumResources;
    WResID TypeName;
} WResTypeInfo;
typedef struct WResTypeNode {
    struct WResTypeNode *Next, *Prev;
    struct WResResNode *Head, *Tail;
    WResTypeInfo Info;
} WResTypeNode;
typedef struct WResResInfo {
    unsigned short NumResources;
    WResID ResName;
} WResResInfo;
typedef struct WResResInfo1 {
    unsigned short MemoryFlags;
    unsigned long Offset, Length;
    WResID ResName;
} WResResInfo1;
typedef struct WResResNode {
    struct WResResNode *Next, *Prev;
    struct WResLangNode *Head, *Tail;
    WResResInfo Info;
} WResResNode;
typedef struct WResLangNode {
    struct WResLangNode *Next, *Prev;
    void *data;
    struct {
        struct {
            unsigned short lang;
            unsigned char sublang;
        } lang;
        unsigned short MemoryFlags;
        unsigned long Offset, Length;
    } Info;
} WResLangNode;
typedef struct WResDirHead {
    unsigned short NumResources;
    unsigned short NumTypes;
    struct WResTypeNode *Head;
    struct WResTypeNode *Tail;
    unsigned short TargetOS;
} WResDirHead;
typedef struct WResHeader {
    unsigned long Magic[2];
    unsigned long DirOffset;
    unsigned short NumResources, NumTypes, WResVer;
} WResHeader;
typedef struct WResExtHeader {
    unsigned short TargetOS;
    unsigned short reserved[4];
} WResExtHeader;
#pragma pack()
typedef char WlibResourceResInfoSize[(sizeof(WResResInfo) == 5) ? 1 : -1];
typedef char WlibResourceResInfo1Size[(sizeof(WResResInfo1) == 13) ? 1 : -1];
typedef char WlibResourceResNodeSize[(sizeof(WResResNode) == 21) ? 1 : -1];
typedef char WlibResourceLangNodeSize[(sizeof(WResLangNode) == 25) ? 1 : -1];
typedef char WlibResourceIDSize[(sizeof(WResID) == 3) ? 1 : -1];
typedef char WlibResourceTypeInfoSize[(sizeof(WResTypeInfo) == 5) ? 1 : -1];
typedef char WlibResourceTypeNodeSize[(sizeof(WResTypeNode) == 21) ? 1 : -1];
typedef char WlibResourceHeaderSize[(sizeof(WResHeader) == 18) ? 1 : -1];
typedef char WlibResourceExtHeaderSize[(sizeof(WResExtHeader) == 10) ? 1 : -1];
typedef char WlibResourceDirectorySize[(sizeof(WResDirHead) == 14) ? 1 : -1];
typedef WResDirHead *WResDir;
extern WResDir MainDir;            /* Original VA 0x430444. */

/* Only the encountered prefix of the resource callback table is defined. */
struct WResRoutines {
    int (__cdecl *open)( const char *, int, ... );
    int (*close)( int );
    int (*write)( int, const void *, unsigned );
    int (*read)( int, void *, unsigned );
    long (*seek)( int, long, int );
    long (*tell)( int );
    void *(*alloc)( unsigned );
    void (*free)( void * );
};
extern struct WResRoutines WResRtns;
extern long FileShift;             /* Original VA 0x430440. */
typedef enum {
    WRS_BAD_SIG = 3,
    WRS_BAD_VERSION = 4,
    WRS_OPEN_FAILED = 5,
    WRS_SEEK_FAILED = 9,
    WRS_READ_FAILED = 8,
    WRS_READ_INCOMPLETE = 14,
    WRS_MALLOC_FAILED = 13
} WResStatus;

int OpenResFile( HANDLE_INFO *instance );
int ResOpenFileRO( const char *filename );
void WresRecordError( WResStatus status );
int FindResources( HANDLE_INFO *instance );
long res_seek( int handle, long position, int where );
int InitResources( HANDLE_INFO *instance );
WResDir WResInitDir( void );
int WResReadDir( int handle, WResDir dir, int *dup_discarded );
void __FreeTypeList( WResDir dir );
int WResIsWResFile( int handle );
int ResReadUint32( unsigned long *value, int handle );
int readWResDir( int handle, WResDir dir );
int WResReadHeaderRecord( WResHeader *header, int handle );
int WResReadExtHeader( WResExtHeader *header, int handle );
int readTypeList( int handle, WResDir dir, unsigned short version );
int WResReadFixedTypeRecord( WResTypeInfo *info, int handle );
int WResIDExtraBytes( const WResID *id );
int WResReadExtraWResID( WResID *id, int handle );
void ResAddLLItemAtEnd( void **head, void **tail, void *item );
int readResList( int handle, WResTypeNode *type, unsigned short version );
int WResReadFixedResRecord1( WResResInfo1 *info, int handle );
int WResReadFixedResRecord( WResResInfo *info, int handle );
int readLangInfoList( int handle, WResResNode *node );
int readMResDir( int handle, WResDir dir, int *dup_discarded );

void CheckMemoryHook( void );       /* Original VA 0x411fdf; name unknown. */
int InitMsg( const char *image_name );
void HandleSignal( int signal_number );
void InitToolState( int argc, char **argv );
unsigned char ProcessCommands( void );
void FiniToolState( void );
int FiniMsg( void );

#if defined(__WATCOMC__)
/* Inline frontier marker; adds no helper vertex to the function graph. */
void reconstruction_trap( void );
#pragma aux reconstruction_trap = 0xcc;
#define WLIB_FRONTIER() reconstruction_trap()
#else
#define WLIB_FRONTIER() __asm__ volatile ("int3")
#endif
#endif
