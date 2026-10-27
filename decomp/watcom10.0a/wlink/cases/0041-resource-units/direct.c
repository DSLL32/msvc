/* Authored typed1994-format assertions; no original execution/CLI claim. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <unistd.h>
#include <recon-tool.h>
#include "../../src/resource.h"
#include <recon.h>
#define CHECK(x) do { if (!(x)) return 0; } while (0)
int resource_error_unit(void)
{
    WresRecordError(14);
    WresRecordError(8);
    return 1;
}
static int counted_record_unit(void)
{
    struct { unsigned char before; WResResInfo info; char extra[3]; unsigned char after; } owner;
    int fd;
    memset(&owner,0xa5,sizeof(owner));
    fd = open("record.bin",O_BINARY); CHECK(fd >= 0);
    CHECK(WResReadFixedResRecord(&owner.info,fd) == 0);
    CHECK(owner.info.NumResources == 2 && owner.info.ResName.IsName == 1);
    CHECK(owner.info.ResName.ID.Name.NumChars == 4);
    CHECK(WResReadExtraWResID(&owner.info.ResName,fd) == 0);
    CHECK(memcmp(owner.info.ResName.ID.Name.Name,"Name",4) == 0);
    CHECK(owner.before == 0xa5 && owner.after == 0xa5);
    CHECK(lseek(fd,0,SEEK_CUR) == 8 && close(fd) == 0);
    return 1;
}
static int name_order_unit(void)
{
    struct { unsigned char length; char bytes[8]; } first={4,"Name"},same={4,"nAME"},prefix={3,"Nam"},other={4,"Naze"};
    CHECK(WResIDNameCmp((WResIDName *)&first,(WResIDName *)&same) == 0);
    CHECK(WResIDNameCmp((WResIDName *)&prefix,(WResIDName *)&first) == -1);
    CHECK(WResIDNameCmp((WResIDName *)&first,(WResIDName *)&prefix) == 1);
    CHECK(WResIDNameCmp((WResIDName *)&first,(WResIDName *)&other) == 'm'-'z');
    CHECK(memcmp(first.bytes,"Name",4) == 0 && memcmp(same.bytes,"nAME",4) == 0);
    return 1;
}
static int named_directory_unit(void)
{
    struct { WResID id; char extra[7]; } type,name;
    WResDir dir;
    WResDirWindow window;
    WResLangType lang;
    WResLangNode *first,*second;
    int fd,duplicate=9;
    char bytes[7];
    memset(&type,0,sizeof(type)); memset(&name,0,sizeof(name));
    type.id.IsName=1;type.id.ID.Name.NumChars=6;memcpy(type.id.ID.Name.Name,"custom",6);
    name.id.IsName=1;name.id.ID.Name.NumChars=8;memcpy(name.id.ID.Name.Name,"greeting",8);
    fd=open("named-v2.res",O_BINARY);CHECK(fd >= 0);
    dir=WResInitDir();CHECK(dir != NULL);
    CHECK(WResReadDir(fd,dir,&duplicate) == 0 && duplicate == 0);
    CHECK(dir->NumTypes == 1 && dir->NumResources == 1 && dir->TargetOS == 1);
    CHECK(dir->Head == dir->Tail && dir->Head->Info.NumResources == 1);
    CHECK(dir->Head->Head == dir->Head->Tail);
    first=dir->Head->Head->Head;second=first->Next;
    CHECK(second != NULL && first->Prev == NULL && second->Prev == first && second->Next == NULL);
    CHECK(first->data == NULL && second->data == NULL && dir->Head->Head->Tail == second);
    CHECK(lseek(fd,0,SEEK_CUR) == 90);
    lang.lang=0x407;lang.sublang=1;
    window=WResFindResource(&type.id,&name.id,dir,&lang);
    CHECK(window.CurrLang == second && window.CurrRes->Info.NumResources == 2);
    CHECK(second->Info.Offset == 35 && second->Info.Length == 7 && second->Info.MemoryFlags == 0x30);
    CHECK(lseek(fd,35,SEEK_SET) == 35 && read(fd,bytes,7) == 7 && memcmp(bytes,"Deutsch",7) == 0);
    __FreeTypeList(dir);CHECK(dir->Head == NULL && dir->Tail == NULL);
    WResFreeDir(dir);CHECK(close(fd) == 0);
    return 1;
}
static int microsoft_directory_unit(void)
{
    WResDir dir;
    WResID type,name;
    struct { WResID id; char extra[7]; } named_type,named_name;
    WResDirWindow window;
    int fd,duplicate=0;
    char bytes[5];
    fd=open("win16.res",O_BINARY);CHECK(fd >= 0);
    dir=WResInitDir();CHECK(dir != NULL);
    CHECK(WResReadDir(fd,dir,&duplicate) == 0 && duplicate == 1);
    CHECK(dir->NumTypes == 2 && dir->NumResources == 2 && dir->TargetOS == 1);
    CHECK(lseek(fd,0,SEEK_CUR) == 65);
    WResInitIDFromNum(6,&type);WResInitIDFromNum(1,&name);
    window=WResFindResource(&type,&name,dir,NULL);
    CHECK(window.CurrLang != NULL && window.CurrLang->Info.Offset == 12 && window.CurrLang->Info.Length == 5);
    CHECK(lseek(fd,12,SEEK_SET) == 12 && read(fd,bytes,5) == 5 && memcmp(bytes,"first",5) == 0);
    memset(&named_type,0,sizeof(named_type)); memset(&named_name,0,sizeof(named_name));
    named_type.id.IsName=1; named_type.id.ID.Name.NumChars=6;
    memcpy(named_type.id.ID.Name.Name,"custom",6);
    named_name.id.IsName=1; named_name.id.ID.Name.NumChars=8;
    memcpy(named_name.id.ID.Name.Name,"greeting",8);
    window=WResFindResource(&named_type.id,&named_name.id,dir,NULL);
    CHECK(window.CurrLang != NULL && window.CurrLang->data == NULL);
    CHECK(window.CurrLang->Info.Offset == 60 && window.CurrLang->Info.Length == 5);
    CHECK(window.CurrLang->Info.MemoryFlags == 0x10);
    CHECK(lseek(fd,60,SEEK_SET) == 60 && read(fd,bytes,5) == 5 && memcmp(bytes,"named",5) == 0);
    __FreeTypeList(dir);CHECK(dir->Head == NULL && dir->Tail == NULL);
    WResFreeDir(dir);CHECK(close(fd) == 0);
    return 1;
}
const ReconTest resource_tests[] = {
 { "valid-fixed-counted-record",1,counted_record_unit },
 { "case-folded-counted-name-order",1,name_order_unit },
 { "named-v2-directory-language-links-and-free",1,named_directory_unit },
 { "Win16-numeric-named-duplicate-directory",1,microsoft_directory_unit }
};
