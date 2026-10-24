#ifndef LINK_INPUT_H
#define LINK_INPUT_H
#include <windows.h>
#include "link-driver.h"
#include <recon.h>

#ifdef __cplusplus
extern "C" {
#endif
/* Diagnostic input filename. This 32-byte native prefix models observed input;
 * it is not a claim about the original full buffer capacity. Capture guards hold
 * a longer filename before this body executes. */
extern char current_phase[32];
extern int current_input_file;
/* Distinct from the BYTE incremental_failed flag used by run_linker. */
extern DWORD incremental_status_code;
extern DWORD archive_member_offset;
extern BYTE object_read_mode;
OutputModule *__stdcall read_input_argument(InputArgument *,OutputLibrary *,LinkContext *);
BYTE __stdcall is_archive(const char *,int);
void __stdcall read_library_index(OutputLibrary *,LinkContext *);
BYTE __stdcall is_mac_resource_file(int);
DWORD __stdcall read_big_endian_dword(int);
DWORD __stdcall byte_swap_dword(DWORD);
OutputModule *__stdcall read_object_file(InputArgument *,LinkContext *,OutputLibrary *);
void __stdcall load_object_module(LinkContext *,void *,OutputModule *,WORD);
void __stdcall infer_output_file_name(LinkContext *,InputArgument *);
typedef char InputArgumentTagAt0c[(offsetof(InputArgument,input_tag)==12)?1:-1];
typedef char OutputLibraryTagAt34[(offsetof(OutputLibrary,input_tag)==52)?1:-1];
#ifdef __cplusplus
}
#endif
#endif
