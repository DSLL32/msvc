#ifndef LINK_MAC_OPTIONS_H
#define LINK_MAC_OPTIONS_H
#include <windows.h>
#include <stddef.h>
#include <recon.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct MacImportVersion {
    const char *library;
    void *owner;
    DWORD version, reserved;
    struct MacImportVersion *next;
} MacImportVersion;
typedef struct MacImportVersions {
    MacImportVersion *first,*last;
    WORD count,reserved;
} MacImportVersions;
typedef char MacVersionLayout[(sizeof(MacImportVersion)==20 &&
    offsetof(MacImportVersion,version)==8 &&
    offsetof(MacImportVersion,next)==16 && sizeof(MacImportVersions)==12)?1:-1];

extern MacImportVersions mac_library_current_versions,mac_library_old_code_versions;
extern MacImportVersions mac_current_version_history,mac_old_code_version_history;
extern MacImportVersions mac_old_api_version_history;
#ifdef __cplusplus
}
#endif
#endif
