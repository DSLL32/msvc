#ifndef LINK_INPUT_H
#define LINK_INPUT_H
#include <windows.h>
#include "link-driver.h"
#include <recon.h>

#ifdef __cplusplus
extern "C" {
#endif
/* Diagnostic input filename. This32B native prefix models only observed input;
 * it is not a claim about the original full buffer capacity. Capture guards hold
 * a longer filename before this body executes. */
extern char current_phase[32];
extern int current_input_file;
/* Distinct from the BYTE incremental_failed flag used by run_linker. */
extern DWORD incremental_status_code;
OutputModule *__stdcall read_input_argument(InputArgument *,OutputLibrary *,LinkContext *);
void __stdcall load_object_module(LinkContext *,void *,OutputModule *,WORD);
void __stdcall infer_output_file_name(LinkContext *,InputArgument *);
#ifdef __cplusplus
}
#endif
#endif
