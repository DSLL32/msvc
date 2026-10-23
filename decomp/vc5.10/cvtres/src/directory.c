#include "cvtres.h"
#include <recon.h>

IMAGE_RESOURCE_DIRECTORY *__stdcall init_directory(
    IMAGE_RESOURCE_DIRECTORY *directory, DWORD characteristics, DWORD version,
    DWORD timestamp, WORD names, WORD ids)
{
    directory->Characteristics = characteristics;
    directory->TimeDateStamp = timestamp;
    directory->MajorVersion = (WORD)(version >> 16);
    directory->MinorVersion = (WORD)version;
    directory->NumberOfNamedEntries = names;
    directory->NumberOfIdEntries = ids;
    return directory;
}

IMAGE_RESOURCE_DATA_ENTRY *__stdcall init_data_entry(
    IMAGE_RESOURCE_DATA_ENTRY *entry, DWORD offset, DWORD size,
    DWORD codepage, DWORD reserved)
{
    entry->OffsetToData = offset;
    entry->Size = size;
    entry->CodePage = codepage;
    entry->Reserved = reserved;
    return entry;
}
