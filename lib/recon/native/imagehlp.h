#ifndef RECON_NATIVE_IMAGEHLP_H
#define RECON_NATIVE_IMAGEHLP_H
/* Native subset of the VC5 IMAGEHLP SDK, preserving ordinary declarations. */
#include <win32.h>
#ifndef IMAGEAPI
#define IMAGEAPI WINAPI
#endif
#ifdef __cplusplus
extern "C" {
#endif
typedef enum _IMAGEHLP_STATUS_REASON {
    BindOutOfMemory,
    BindRvaToVaFailed,
    BindNoRoomInImage,
    BindImportModuleFailed,
    BindImportProcedureFailed,
    BindImportModule,
    BindImportProcedure,
    BindForwarder,
    BindForwarderNOT,
    BindImageModified,
    BindExpandFileHeaders,
    BindImageComplete,
    BindMismatchedSymbols,
    BindSymbolsNotUpdated
} IMAGEHLP_STATUS_REASON;
typedef BOOL (WINAPI *PIMAGEHLP_STATUS_ROUTINE)(IMAGEHLP_STATUS_REASON,LPSTR,LPSTR,ULONG,ULONG);
BOOL IMAGEAPI BindImageEx(DWORD,LPSTR,LPSTR,LPSTR,PIMAGEHLP_STATUS_ROUTINE);
BOOL IMAGEAPI ReBaseImage(LPSTR,LPSTR,BOOL,BOOL,BOOL,ULONG,ULONG *,ULONG *,ULONG *,ULONG *,ULONG);
PIMAGE_NT_HEADERS IMAGEAPI CheckSumMappedFile(LPVOID,DWORD,LPDWORD,LPDWORD);
/* Declaration only for the later checksum path; no native binding yet. */
DWORD IMAGEAPI MapFileAndCheckSumA(LPSTR,LPDWORD,LPDWORD);
#define CHECKSUM_SUCCESS 0
#define CHECKSUM_OPEN_FAILURE 1
#define CHECKSUM_MAP_FAILURE 2
#define CHECKSUM_MAPVIEW_FAILURE 3
#define CHECKSUM_UNICODE_FAILURE 4
#define BIND_NO_BOUND_IMPORTS 0x00000001
#define BIND_NO_UPDATE 0x00000002
#define BIND_ALL_IMAGES 0x00000004
#define IMAGE_SEPARATION (64*1024)
#ifdef __cplusplus
}
#endif
#endif
