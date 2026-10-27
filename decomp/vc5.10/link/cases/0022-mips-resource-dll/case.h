/* Original RC input; no target executable code is fabricated. */
#include <windows.h>
LANGUAGE LANG_ENGLISH,SUBLANG_ENGLISH_US
STRINGTABLE
BEGIN
    101 "MIPS resource library"
    102 "Independent language and payload records"
END
MIPS_PAYLOAD RCDATA
BEGIN
    0x1234,0xabcd,0x12345678L,"VC5 MIPS resources",0
END
1 VERSIONINFO
 FILEVERSION 1,2,3,4
 PRODUCTVERSION 5,10,0,1
 FILEFLAGSMASK 0x3fL
 FILEFLAGS 0
 FILEOS VOS_NT_WINDOWS32
 FILETYPE VFT_DLL
 FILESUBTYPE 0
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904e4"
        BEGIN
            VALUE "CompanyName","Reconstruction resource fixture\0"
            VALUE "FileDescription","MIPS resource-only DLL\0"
            VALUE "FileVersion","1.2.3.4\0"
            VALUE "OriginalFilename","mips.dll\0"
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation",0x409,1252
    END
END
LANGUAGE LANG_GERMAN,SUBLANG_GERMAN
STRINGTABLE
BEGIN
    101 "MIPS Ressourcenbibliothek"
    102 "Getrennte Sprachdaten"
END

LANGUAGE LANG_FRENCH,SUBLANG_FRENCH
STRINGTABLE
BEGIN
    101 "Bibliotheque de ressources MIPS"
    102 "Donnees et langues independantes"
END
