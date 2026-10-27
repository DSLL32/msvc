#include <windows.h>
#include <imagehlp.h>
#include <recon.h>
static BOOL (WINAPI *bind_typed)(DWORD,LPSTR,LPSTR,LPSTR,PIMAGEHLP_STATUS_ROUTINE)=BindImageEx;
static BOOL (WINAPI *rebase_typed)(LPSTR,LPSTR,BOOL,BOOL,BOOL,ULONG,ULONG *,ULONG *,ULONG *,ULONG *,ULONG)=ReBaseImage;
static PIMAGE_NT_HEADERS (WINAPI *checksum_typed)(LPVOID,DWORD,LPDWORD,LPDWORD)=CheckSumMappedFile;
int main(void) {return bind_typed==0 || rebase_typed==0 || checksum_typed==0;}
