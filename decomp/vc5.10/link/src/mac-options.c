/* Mac import versions retain per-library bounds and incremental history. */
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "options.h"
#include "memory.h"
#include <recon.h>

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

/* Distinct original owners: 494558,494518,494580,494530,4944c8. */
MacImportVersions mac_library_current_versions,mac_library_old_code_versions;
MacImportVersions mac_current_version_history,mac_old_code_version_history;
MacImportVersions mac_old_api_version_history;
BYTE mac_import_versions_changed; /* Original47f0c8; initially zero. */
extern DWORD mac_current_import_version;
/* Original4670b4 is old CODE; 4670b0 is old API, both initially -1. */
extern LONG mac_old_code_version,mac_old_api_version;
extern BYTE incremental_reuse;
void __cdecl link_warning(const char *,unsigned,...);
MacImportVersion *__stdcall append_mac_version(MacImportVersions *list,
    const char *library,void *owner,DWORD version)
{ __asm { int 3 } return NULL; }

void __stdcall set_mac_import_versions(const char *library,const char *current,
    const char *old_code,const char *old_api)
{
    MacImportVersion *entry;
    DWORD requested,effective=0;
    unsigned index;
    BYTE found=0;
    if(library) {
        /* The per-library route has current/code bounds only. The original
         * reuses its fourth formal slot for the effective bound. */
        if(current) {
            requested=(DWORD)atoi(current);
            entry=mac_library_current_versions.first;
            for(index=0;index<(WORD)mac_library_current_versions.count && !found;
                ++index,entry=entry->next) {
                if(!_stricmp(entry->library,library)) {
                    found=1;
                    if(entry->version<requested) {
                        entry->version=requested;
                        mac_import_versions_changed=1;
                    }
                    effective=entry->version;
                }
            }
            if(found)found=0;
            else {
                append_mac_version(&mac_library_current_versions,
                    duplicate_argument(library),NULL,requested);
                effective=requested;
            }
            if(incremental_reuse) {
                entry=mac_current_version_history.first;
                for(index=0;index<(WORD)mac_current_version_history.count && !found;
                    ++index,entry=entry->next) {
                    if(entry->library && !_stricmp(entry->library,library) &&
                       entry->version==requested)found=1;
                }
                if(found)found=0;
                else {
                    append_mac_version(&mac_current_version_history,
                        duplicate_argument(library),NULL,requested);
                    link_warning(library,effective==requested?4547:4548,
                        "CURRENTVER",requested);
                }
            }
        }
        if(old_code) {
            requested=(DWORD)atoi(old_code);
            entry=mac_library_old_code_versions.first;
            for(index=0;index<(WORD)mac_library_old_code_versions.count && !found;
                ++index,entry=entry->next) {
                if(!_stricmp(entry->library,library)) {
                    found=1;
                    if(entry->version>requested) {
                        entry->version=requested;
                        mac_import_versions_changed=1;
                    }
                    effective=entry->version;
                }
            }
            if(found)found=0;
            else {
                append_mac_version(&mac_library_old_code_versions,
                    duplicate_argument(library),NULL,requested);
                effective=requested;
            }
            if(incremental_reuse) {
                entry=mac_old_code_version_history.first;
                for(index=0;index<(WORD)mac_old_code_version_history.count && !found;
                    ++index,entry=entry->next) {
                    if(entry->library && !_stricmp(entry->library,library) &&
                       entry->version==requested)found=1;
                }
                if(!found) {
                    append_mac_version(&mac_old_code_version_history,
                        duplicate_argument(library),NULL,requested);
                    link_warning(library,effective==requested?4547:4548,
                        "OLDCODEVER",requested);
                }
            }
        }
        return;
    }
    if(current) {
        requested=(DWORD)atoi(current);
        if(requested>mac_current_import_version) {
            if(mac_current_import_version)mac_import_versions_changed=1;
            mac_current_import_version=requested;
        }
        if(incremental_reuse) {
            entry=mac_current_version_history.first;
            for(index=0;index<(WORD)mac_current_version_history.count && !found;
                ++index,entry=entry->next) {
                if(!entry->library && entry->version==requested)found=1;
            }
            if(found)found=0;
            else {
                append_mac_version(&mac_current_version_history,NULL,NULL,requested);
                link_warning(NULL,mac_current_import_version==requested?4547:4548,
                    "CURRENTVER",requested);
            }
        }
    }
    if(old_code) {
        requested=(DWORD)atoi(old_code);
        if(requested<(DWORD)mac_old_code_version) {
            if(mac_old_code_version!=-1)mac_import_versions_changed=1;
            mac_old_code_version=(LONG)requested;
        }
        if(incremental_reuse) {
            entry=mac_old_code_version_history.first;
            for(index=0;index<(WORD)mac_old_code_version_history.count && !found;
                ++index,entry=entry->next) {
                if(!entry->library && entry->version==requested)found=1;
            }
            if(found)found=0;
            else {
                append_mac_version(&mac_old_code_version_history,NULL,NULL,requested);
                link_warning(NULL,(DWORD)mac_old_code_version==requested?4547:4548,
                    "OLDCODEVER",requested);
            }
        }
    }
    if(old_api) {
        requested=(DWORD)atoi(old_api);
        if(requested<(DWORD)mac_old_api_version) {
            if(mac_old_api_version!=-1)mac_import_versions_changed=1;
            mac_old_api_version=(LONG)requested;
        }
        if(incremental_reuse) {
            entry=mac_old_api_version_history.first;
            for(index=0;index<(WORD)mac_old_api_version_history.count && !found;
                ++index,entry=entry->next) {
                if(!entry->library && entry->version==requested)found=1;
            }
            if(!found) {
                append_mac_version(&mac_old_api_version_history,NULL,NULL,requested);
                link_warning(NULL,(DWORD)mac_old_api_version==requested?4547:4548,
                    "OLDAPIVER",requested);
            }
        }
    }
}

#pragma intrinsic(strlen,strcpy,strcat)
void __stdcall set_mac_initializer(LinkContext *context,const char *name)
{
    char *decorated=(char *)allocate_incremental(strlen(name)+3);
    context->mac_initializer_name=(const unsigned char *)decorated;
    if(*name!='?') {
        strcpy(decorated,"_");
        strcat(decorated,name);
    } else strcpy(decorated,name);
}

void __stdcall set_mac_terminator(LinkContext *context,const char *name)
{
    char *decorated=(char *)allocate_incremental(strlen(name)+3);
    context->mac_terminator_name=(const unsigned char *)decorated;
    if(*name!='?') {
        strcpy(decorated,"_");
        strcat(decorated,name);
    } else strcpy(decorated,name);
}
