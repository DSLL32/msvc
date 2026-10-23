#ifndef LINK_PASS_ONE_H
#define LINK_PASS_ONE_H
#include <windows.h>
#include "link-driver.h"
#include <recon.h>

typedef struct LibraryEnumerator {
    BYTE (__stdcall *next)(struct LibraryEnumerator *);
    void (__stdcall *end)(struct LibraryEnumerator *);
    OutputLibrary *first,*current;
} LibraryEnumerator;
typedef char LibraryEnumeratorSize[(sizeof(LibraryEnumerator)==16)?1:-1];

#ifdef __cplusplus
extern "C" {
#endif
void __cdecl link_message(unsigned message,...);
OutputLibrary *__stdcall create_library(const char *,const char *,LinkPointerList *);
void __stdcall check_output_input_collision(const char *,InputArgument *);
void __stdcall check_input_modules(LinkContext *,OutputLibrary *);
void __stdcall resolve_weak_externals(LinkContext *,DWORD);
void __stdcall search_libraries(LinkContext *);
BYTE __stdcall resolve_initial_entry(LinkContext *);
BYTE __stdcall infer_subsystem(LinkContext *);
BYTE __stdcall infer_entry_point(LinkContext *);
BYTE __stdcall process_definition_file(LinkContext *,const char *);
void __stdcall finalize_mac_entry(StringNode **,LinkContext *);
void __stdcall set_subsystem_version(LinkContext *);
void __stdcall warn_unused_libraries(OutputLibrary *);
void __stdcall warn_missing_default_libraries(LinkPointerList *);
void __stdcall initialize_library_enumerator(LibraryEnumerator *,OutputLibrary *);
BYTE __stdcall next_library(LibraryEnumerator *);
void __stdcall warn_unused_delay_libraries(void);
#ifdef __cplusplus
}
#endif
#endif
