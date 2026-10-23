#ifndef RECON_WIN32_H
#define RECON_WIN32_H
/* The Win32/PE32 declarations used by the reconstructed command-line tools.
 * Keep long and pointers 32 bits: this interface is for i386 Linux builds. */
#include <stddef.h>
#if !defined(__i386__)
#error RECON_NATIVE_TEST requires an i386 build (-m32)
#endif
#define WINAPI __attribute__((stdcall))
#define CALLBACK WINAPI
#define __cdecl __attribute__((cdecl))
#define __stdcall __attribute__((stdcall))
#define __fastcall __attribute__((fastcall))
#define __thiscall __attribute__((thiscall))
#define TRUE 1
#define FALSE 0
#define MAX_PATH 260
#define HEAP_NO_SERIALIZE 1
#define HEAP_ZERO_MEMORY 8
#define CONST const
#define VOID void
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
typedef unsigned char BYTE;
typedef unsigned short WORD, WCHAR;
typedef unsigned short USHORT;
typedef unsigned long DWORD;
typedef long LONG;
typedef short SHORT;
typedef unsigned int WPARAM;
typedef long LPARAM,LRESULT;
typedef int BOOL;
typedef unsigned int UINT;
typedef void *HANDLE, *HMODULE, *HINSTANCE, *HWND, *HGLOBAL, *LPVOID;
typedef const void *LPCVOID;
typedef char *LPSTR;
typedef const char *LPCSTR;
typedef DWORD *LPDWORD;
typedef WCHAR *LPWSTR;
typedef const WCHAR *LPCWSTR;
typedef void (WINAPI *FARPROC)(void);
typedef struct { DWORD nLength; LPVOID lpSecurityDescriptor; BOOL bInheritHandle; } SECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;
typedef struct { WORD wYear,wMonth,wDayOfWeek,wDay,wHour,wMinute,wSecond,wMilliseconds; } SYSTEMTIME;
typedef struct { DWORD dwLowDateTime,dwHighDateTime; } FILETIME;
typedef struct _SYSTEM_INFO {
 union { DWORD dwOemId; struct { WORD wProcessorArchitecture,wReserved; }; };
 DWORD dwPageSize; LPVOID lpMinimumApplicationAddress,lpMaximumApplicationAddress;
 DWORD dwActiveProcessorMask,dwNumberOfProcessors,dwProcessorType,dwAllocationGranularity;
 WORD wProcessorLevel,wProcessorRevision;
} SYSTEM_INFO,*LPSYSTEM_INFO;
typedef struct _MEMORYSTATUS {
 DWORD dwLength,dwMemoryLoad,dwTotalPhys,dwAvailPhys,dwTotalPageFile,dwAvailPageFile;
 DWORD dwTotalVirtual,dwAvailVirtual;
} MEMORYSTATUS,*LPMEMORYSTATUS;
typedef struct _cpinfo {
 UINT MaxCharSize; BYTE DefaultChar[2]; BYTE LeadByte[12];
} CPINFO, *LPCPINFO;
typedef struct _COORD { SHORT X,Y; } COORD;
typedef struct _SMALL_RECT { SHORT Left,Top,Right,Bottom; } SMALL_RECT;
typedef struct _CONSOLE_SCREEN_BUFFER_INFO {
 COORD dwSize,dwCursorPosition; WORD wAttributes;
 SMALL_RECT srWindow; COORD dwMaximumWindowSize;
} CONSOLE_SCREEN_BUFFER_INFO, *PCONSOLE_SCREEN_BUFFER_INFO;
/* No reconstructed exception filter currently reads the CPU context. Keep
 * its genuine tag opaque until a consumer encounters the i386 context ABI. */
/* Classic i386 SDK context; declaring layout does not emulate Win32 SEH. */
#define SIZE_OF_80387_REGISTERS 80
typedef struct _FLOATING_SAVE_AREA {
 DWORD ControlWord,StatusWord,TagWord,ErrorOffset,ErrorSelector,DataOffset,DataSelector;
 BYTE RegisterArea[SIZE_OF_80387_REGISTERS];
 DWORD Cr0NpxState;
} FLOATING_SAVE_AREA,*PFLOATING_SAVE_AREA;
typedef struct _CONTEXT {
 DWORD ContextFlags,Dr0,Dr1,Dr2,Dr3,Dr6,Dr7;
 FLOATING_SAVE_AREA FloatSave;
 DWORD SegGs,SegFs,SegEs,SegDs;
 DWORD Edi,Esi,Ebx,Edx,Ecx,Eax;
 DWORD Ebp,Eip,SegCs,EFlags,Esp,SegSs;
} CONTEXT,*PCONTEXT;
typedef struct _EXCEPTION_RECORD {
 DWORD ExceptionCode,ExceptionFlags;
 struct _EXCEPTION_RECORD *ExceptionRecord;
 LPVOID ExceptionAddress; DWORD NumberParameters;
 DWORD ExceptionInformation[15];
} EXCEPTION_RECORD;
typedef struct _EXCEPTION_POINTERS {
 EXCEPTION_RECORD *ExceptionRecord; CONTEXT *ContextRecord;
} EXCEPTION_POINTERS;
/* RC owns these counted-string structures, including their field names. */
struct AnsiString;
struct UnicodeString;
#define INVALID_HANDLE_VALUE ((HANDLE)-1)
#define DLL_PROCESS_DETACH 0
#define DLL_PROCESS_ATTACH 1
#define DLL_THREAD_ATTACH 2
#define DLL_THREAD_DETACH 3
#define GENERIC_READ 0x80000000UL
#define GENERIC_WRITE 0x40000000UL
#define DELETE 0x10000UL
#define FILE_SHARE_READ 1
#define FILE_SHARE_WRITE 2
#define CREATE_NEW 1
#define CREATE_ALWAYS 2
#define OPEN_EXISTING 3
#define OPEN_ALWAYS 4
#define TRUNCATE_EXISTING 5
#define FILE_ATTRIBUTE_READONLY 1
#define FILE_ATTRIBUTE_NORMAL 0x80
#define FILE_ATTRIBUTE_TEMPORARY 0x100
#define FILE_FLAG_DELETE_ON_CLOSE 0x04000000
#define FILE_FLAG_SEQUENTIAL_SCAN 0x08000000
#define FILE_FLAG_RANDOM_ACCESS 0x10000000
#define FILE_TYPE_UNKNOWN 0
#define FILE_TYPE_DISK 1
#define FILE_TYPE_CHAR 2
#define FILE_TYPE_PIPE 3
#define PAGE_READONLY 2
#define PAGE_READWRITE 4
#define FILE_MAP_WRITE 2
#define FILE_MAP_READ 4
#define FILE_MAP_ALL_ACCESS 0x000f001f
#define FILE_BEGIN 0
#define FILE_CURRENT 1
#define FILE_END 2
#define MEM_COMMIT 0x1000
#define MEM_RESERVE 0x2000
#define MEM_RELEASE 0x8000
#define STD_INPUT_HANDLE ((DWORD)-10)
#define STD_OUTPUT_HANDLE ((DWORD)-11)
#define STD_ERROR_HANDLE ((DWORD)-12)
#define GMEM_FIXED 0
#define GMEM_ZEROINIT 0x40
#define ERROR_SUCCESS 0
#define ERROR_FILE_NOT_FOUND 2
#define ERROR_PATH_NOT_FOUND 3
#define ERROR_ACCESS_DENIED 5
#define ERROR_INVALID_HANDLE 6
#define ERROR_NOT_ENOUGH_MEMORY 8
#define ERROR_INVALID_DATA 13
#define ERROR_NO_MORE_FILES 18
#define ERROR_NOT_SUPPORTED 50
#define ERROR_MOD_NOT_FOUND 126
#define ERROR_INVALID_PARAMETER 87
#define ERROR_NEGATIVE_SEEK 131
#define ERROR_ALREADY_EXISTS 183
#define ERROR_INSUFFICIENT_BUFFER 122
#define ERROR_NO_UNICODE_TRANSLATION 1113
#define LCMAP_LOWERCASE 0x100
#define LCMAP_UPPERCASE 0x200
#define CTRL_C_EVENT 0
#define CTRL_BREAK_EVENT 1

typedef struct {
 DWORD biSize; LONG biWidth,biHeight; WORD biPlanes,biBitCount;
 DWORD biCompression,biSizeImage; LONG biXPelsPerMeter,biYPelsPerMeter;
 DWORD biClrUsed,biClrImportant;
} BITMAPINFOHEADER,*PBITMAPINFOHEADER;
typedef struct { DWORD bcSize; WORD bcWidth,bcHeight,bcPlanes,bcBitCount; }
 BITMAPCOREHEADER,*PBITMAPCOREHEADER;

#pragma pack(push,2)
typedef struct {
 WORD e_magic,e_cblp,e_cp,e_crlc,e_cparhdr,e_minalloc,e_maxalloc,e_ss,e_sp,e_csum,e_ip,e_cs,e_lfarlc,e_ovno;
 WORD e_res[4],e_oemid,e_oeminfo,e_res2[10]; LONG e_lfanew;
} IMAGE_DOS_HEADER;
typedef struct { WORD Machine,NumberOfSections; DWORD TimeDateStamp,PointerToSymbolTable,NumberOfSymbols; WORD SizeOfOptionalHeader,Characteristics; } IMAGE_FILE_HEADER;
typedef struct { DWORD VirtualAddress,Size; } IMAGE_DATA_DIRECTORY;
typedef struct {
 WORD Magic; BYTE MajorLinkerVersion,MinorLinkerVersion;
 DWORD SizeOfCode,SizeOfInitializedData,SizeOfUninitializedData,AddressOfEntryPoint,BaseOfCode,BaseOfData;
 DWORD ImageBase,SectionAlignment,FileAlignment;
 WORD MajorOperatingSystemVersion,MinorOperatingSystemVersion,MajorImageVersion,MinorImageVersion,MajorSubsystemVersion,MinorSubsystemVersion;
 DWORD Reserved1,SizeOfImage,SizeOfHeaders,CheckSum; WORD Subsystem,DllCharacteristics;
 DWORD SizeOfStackReserve,SizeOfStackCommit,SizeOfHeapReserve,SizeOfHeapCommit,LoaderFlags,NumberOfRvaAndSizes;
 IMAGE_DATA_DIRECTORY DataDirectory[16];
} IMAGE_OPTIONAL_HEADER;
typedef struct { DWORD Signature; IMAGE_FILE_HEADER FileHeader; IMAGE_OPTIONAL_HEADER OptionalHeader; } IMAGE_NT_HEADERS,*PIMAGE_NT_HEADERS;
typedef struct {
 BYTE Name[8]; union { DWORD PhysicalAddress,VirtualSize; } Misc;
 DWORD VirtualAddress,SizeOfRawData,PointerToRawData,PointerToRelocations,PointerToLinenumbers;
 WORD NumberOfRelocations,NumberOfLinenumbers; DWORD Characteristics;
} IMAGE_SECTION_HEADER,*PIMAGE_SECTION_HEADER;
typedef struct {
 union { BYTE ShortName[8]; struct { DWORD Short,Long; } Name; BYTE *LongName[2]; } N;
 DWORD Value; SHORT SectionNumber; WORD Type; BYTE StorageClass,NumberOfAuxSymbols;
} IMAGE_SYMBOL,*PIMAGE_SYMBOL;
typedef union {
 struct { DWORD TagIndex; union { struct { WORD Linenumber,Size; } LnSz; DWORD TotalSize; } Misc;
  union { struct { DWORD PointerToLinenumber,PointerToNextFunction; } Function; struct { WORD Dimension[4]; } Array; } FcnAry; WORD TvIndex;
 } Sym;
 struct { BYTE Name[18]; } File;
 struct { DWORD Length; WORD NumberOfRelocations,NumberOfLinenumbers; DWORD CheckSum; SHORT Number; BYTE Selection; } Section;
} IMAGE_AUX_SYMBOL,*PIMAGE_AUX_SYMBOL;
typedef struct { DWORD VirtualAddress,SymbolTableIndex; WORD Type; } IMAGE_RELOCATION,*PIMAGE_RELOCATION;
typedef struct { union { DWORD SymbolTableIndex,VirtualAddress; } Type; WORD Linenumber; } IMAGE_LINENUMBER,*PIMAGE_LINENUMBER;
typedef struct { BYTE Name[16],Date[12],UserID[6],GroupID[6],Mode[8],Size[10],EndHeader[2]; } IMAGE_ARCHIVE_MEMBER_HEADER,*PIMAGE_ARCHIVE_MEMBER_HEADER;
typedef struct { DWORD VirtualAddress,SizeOfBlock; } IMAGE_BASE_RELOCATION;
typedef struct { DWORD Characteristics,TimeDateStamp; WORD MajorVersion,MinorVersion,NumberOfNamedEntries,NumberOfIdEntries; } IMAGE_RESOURCE_DIRECTORY;
typedef struct { DWORD Name,OffsetToData; } IMAGE_RESOURCE_DIRECTORY_ENTRY;
typedef struct { DWORD OffsetToData,Size,CodePage,Reserved; } IMAGE_RESOURCE_DATA_ENTRY;
typedef struct { DWORD Characteristics,TimeDateStamp; WORD MajorVersion,MinorVersion; DWORD Type,SizeOfData,AddressOfRawData,PointerToRawData; } IMAGE_DEBUG_DIRECTORY;
typedef struct { DWORD Characteristics,TimeDateStamp,ForwarderChain,Name,FirstThunk; } IMAGE_IMPORT_DESCRIPTOR;
typedef struct { DWORD NumberOfSymbols,LvaToFirstSymbol,NumberOfLinenumbers,LvaToFirstLinenumber,RvaToFirstByteOfCode,RvaToLastByteOfCode,RvaToFirstByteOfData,RvaToLastByteOfData; } IMAGE_COFF_SYMBOLS_HEADER;
#pragma pack(pop)

/* NT's frame-pointer-omission debug record (bundled WINNT.H). */
#pragma pack(push,1)
typedef struct _FPO_DATA {
 DWORD ulOffStart,cbProcSize,cdwLocals;
 WORD cdwParams;
 WORD cbProlog:8,cbRegs:3,fHasSEH:1,fUseBP:1,reserved:1,cbFrame:2;
} FPO_DATA,*PFPO_DATA;
#pragma pack(pop)
#define FRAME_FPO 0
#define FRAME_TRAP 1
#define FRAME_TSS 2
#define SIZEOF_RFPO_DATA 16

/* PE/COFF constants referenced by LINK, from the bundled NT headers. */
#define IMAGE_DOS_SIGNATURE 0x5A4D
#define IMAGE_NT_SIGNATURE 0x00004550
#define IMAGE_FILE_RELOCS_STRIPPED 0x0001
#define IMAGE_FILE_EXECUTABLE_IMAGE 0x0002
#define IMAGE_FILE_LINE_NUMS_STRIPPED 0x0004
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED 0x0008
#define IMAGE_FILE_BYTES_REVERSED_LO 0x0080
#define IMAGE_FILE_32BIT_MACHINE 0x0100
#define IMAGE_FILE_SYSTEM 0x1000
#define IMAGE_FILE_DLL 0x2000
#define IMAGE_FILE_BYTES_REVERSED_HI 0x8000
#define IMAGE_FILE_MACHINE_I860 0x14d
#define IMAGE_FILE_MACHINE_I386 0x14c
#define IMAGE_SUBSYSTEM_NATIVE 1
#define IMAGE_SUBSYSTEM_WINDOWS_GUI 2
#define IMAGE_SUBSYSTEM_WINDOWS_CUI 3
#define IMAGE_SUBSYSTEM_POSIX_CUI 7
#define IMAGE_DIRECTORY_ENTRY_EXPORT 0
#define IMAGE_DIRECTORY_ENTRY_IMPORT 1
#define IMAGE_DIRECTORY_ENTRY_RESOURCE 2
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION 3
#define IMAGE_DIRECTORY_ENTRY_BASERELOC 5
#define IMAGE_DIRECTORY_ENTRY_DEBUG 6
#define IMAGE_DIRECTORY_ENTRY_TLS 9
#define IMAGE_DEBUG_TYPE_CODEVIEW 2
#define IMAGE_DEBUG_TYPE_FPO 3
#define IMAGE_SCN_CNT_CODE 0x00000020
#define IMAGE_SCN_CNT_INITIALIZED_DATA 0x00000040
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA 0x00000080
#define IMAGE_SCN_LNK_INFO 0x00000200
#define IMAGE_SCN_LNK_COMDAT 0x00001000
#define IMAGE_SCN_MEM_DISCARDABLE 0x02000000
#define IMAGE_SCN_MEM_EXECUTE 0x20000000
#define IMAGE_SCN_MEM_READ 0x40000000
#define IMAGE_SCN_MEM_WRITE 0x80000000
#define IMAGE_SIZEOF_SYMBOL 18
#define IMAGE_SYM_ABSOLUTE (SHORT)-1
#define IMAGE_SYM_CLASS_EXTERNAL 2
#define IMAGE_SYM_CLASS_STATIC 3
#define IMAGE_SYM_CLASS_LABEL 6
#define IMAGE_SYM_CLASS_UNDEFINED_LABEL 7
#define IMAGE_SYM_CLASS_STRUCT_TAG 10
#define IMAGE_SYM_CLASS_UNION_TAG 12
#define IMAGE_SYM_CLASS_UNDEFINED_STATIC 14
#define IMAGE_SYM_CLASS_ENUM_TAG 15
#define IMAGE_SYM_CLASS_BLOCK 100
#define IMAGE_SYM_CLASS_FUNCTION 101
#define IMAGE_SYM_CLASS_END_OF_STRUCT 102
#define IMAGE_SYM_CLASS_FILE 103
#define IMAGE_SYM_CLASS_SECTION 104
#define IMAGE_SYM_CLASS_WEAK_EXTERNAL 105
#define IMAGE_COMDAT_SELECT_ASSOCIATIVE 5
#define IMAGE_SIZEOF_RELOCATION 10
#define IMAGE_REL_I860_DIR32 06
#define IMAGE_REL_I860_HIGH 036
#define IMAGE_REL_I860_HIGHADJ 047
#define IMAGE_REL_I386_ABSOLUTE 0
#define IMAGE_REL_I386_DIR32 06
#define IMAGE_REL_I386_DIR32NB 07
#define IMAGE_REL_I386_SEG12 011
#define IMAGE_REL_I386_SECTION 012
#define IMAGE_REL_I386_SECREL 013
#define IMAGE_REL_I386_REL32 024
#define IMAGE_REL_BASED_ABSOLUTE 0
#define IMAGE_REL_BASED_HIGHLOW 3
#define IMAGE_REL_BASED_HIGHADJ 4
#define IMAGE_SIZEOF_LINENUMBER 6
#define IMAGE_RESOURCE_NAME_IS_STRING 0x80000000
#define IMAGE_RESOURCE_DATA_IS_DIRECTORY 0x80000000

#ifndef EXCEPTION_ACCESS_VIOLATION
#define EXCEPTION_ACCESS_VIOLATION 0xc0000005UL
#define EXCEPTION_STACK_OVERFLOW 0xc00000fdUL
#endif

#endif
