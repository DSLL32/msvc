/* Standalone diagnostic catalog/ABI probe, never part of RCPP's graph. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef char *(__stdcall *MessageLookup)(unsigned);
int main(int argc, char **argv)
{
    HMODULE module;
    MessageLookup lookup;
    FILE *output;
    unsigned code, length, alias;
    char *text;
    if (argc != 4) return 2;
    module = LoadLibraryExA(argv[1], NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (!module) return 3;
    /* LoadLibrary maps EXEs without rebasing their absolute references.
     * Apply the image's own HIGHLOW relocations in this analysis-only mapping
     * when its preferred address is occupied; no executable file is changed. */
    {
        IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)module;
        IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)((char *)module+dos->e_lfanew);
        DWORD delta = (DWORD)module-nt->OptionalHeader.ImageBase;
        if (delta) {
            DWORD old_protection, position, end;
            IMAGE_BASE_RELOCATION *block;
            unsigned count, i, type;
            WORD *entries;
            position = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
            end = position+nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
            if (!position || !VirtualProtect(module,nt->OptionalHeader.SizeOfImage,
                                             PAGE_EXECUTE_READWRITE,&old_protection)) return 6;
            while (position < end) {
                block = (IMAGE_BASE_RELOCATION *)((char *)module+position);
                if (block->SizeOfBlock < sizeof(*block)) return 7;
                count = (block->SizeOfBlock-sizeof(*block))/sizeof(WORD);
                entries = (WORD *)(block+1);
                for (i=0; i<count; ++i) {
                    type = entries[i] >> 12;
                    if (type == IMAGE_REL_BASED_HIGHLOW)
                        *(DWORD *)((char *)module+block->VirtualAddress+(entries[i]&4095)) += delta;
                    else if (type != IMAGE_REL_BASED_ABSOLUTE) return 8;
                }
                position += block->SizeOfBlock;
            }
        }
    }
    lookup = (MessageLookup)((char *)module + strtoul(argv[2],NULL,16));
    output = fopen(argv[3],"wb");
    if (!output) return 4;
    for (code = 0; code < 65538; ++code) {
        unsigned argument = code == 65537 ? (unsigned)-1 : code;
        text = lookup(argument);
        if (text != lookup(argument)) return 5;
        length = strlen(text);
        fwrite(&argument,sizeof(argument),1,output);
        fwrite(&length,sizeof(length),1,output);
        fwrite(text,1,length,output);
    }
    alias = lookup(2045) == lookup(2412);
    fwrite(&alias,sizeof(alias),1,output);
    alias = lookup(0) == lookup((unsigned)-1);
    fwrite(&alias,sizeof(alias),1,output);
    fclose(output);
    FreeLibrary(module);
    return 0;
}
