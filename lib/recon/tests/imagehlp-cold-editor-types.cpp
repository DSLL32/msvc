#include <windows.h>
#include <imagehlp.h>
#include <recon.h>
typedef char FileBits[(IMAGE_FILE_LARGE_ADDRESS_AWARE==0x20 && IMAGE_SCN_MEM_SHARED==0x10000000)?1:-1];
static BOOL (WINAPI *bind_typed)(DWORD,LPSTR,LPSTR,LPSTR,PIMAGEHLP_STATUS_ROUTINE)=BindImageEx;
static BOOL (WINAPI *rebase_typed)(LPSTR,LPSTR,BOOL,BOOL,BOOL,ULONG,ULONG *,ULONG *,ULONG *,ULONG *,ULONG)=ReBaseImage;
static PIMAGE_NT_HEADERS (WINAPI *checksum_typed)(LPVOID,DWORD,LPDWORD,LPDWORD)=CheckSumMappedFile;
static DWORD (WINAPI *map_checksum_typed)(LPSTR,LPDWORD,LPDWORD)=MapFileAndCheckSumA;
int main(void) {return bind_typed==0 || rebase_typed==0 || checksum_typed==0 || map_checksum_typed==0;}
