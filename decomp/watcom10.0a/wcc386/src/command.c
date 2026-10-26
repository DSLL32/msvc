/****************************************************************************
*
*                            Open Watcom Project
*
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  Basic type initialization and type-node allocation.
*
****************************************************************************/


#include <stdlib.h>
#include <process.h>
#include <string.h>
#include <stddef.h>
#include <sys/stat.h>
#include "command.h"
#include "options.h"
#include "scanner.h"
#include "files.h"
#include "macros.h"
#include <recon.h>

FCB *MainSrcFile;
void ParseCmdLine(char **cmdline)
{
    char buffer[1024];
    char *args[2];
    char *cmd;
    if (cmdline==NULL) {
        args[0]=buffer;
        args[1]=NULL;
        cmdline=args;
        getcmd(buffer);
    }
    cmd=(cmdline==NULL) ? NULL : cmdline[0];
    if (cmd==NULL) cmd="";
    while (*cmd==' ') ++cmd;
    if (*cmd=='?' || *cmd=='\0') {
        CBanner();
        CCusage();
    }
    GenCOptions(cmdline);
    if (WholeFName!=NULL) {
        MakePgmName(WholeFName);
        if (!CompFlags.command_line_processed) {
            OpenPgmFile();
            MainSrcFile=SrcFile;
        }
    }
}

char *SrcFName;
void MakePgmName(char *name)
{
    char *p,*root,*end,*last_dot,*extension;
    unsigned length;
    unsigned char c;
    last_dot=NULL;
    p=root=name;
    end=NULL;
    for (;;) {
        c=(unsigned char)*p;
        if (c=='\0' || c==' ' || c=='\t') {
            if (end==NULL) end=p;
            break;
        }
        if (c=='.') {
            last_dot=p;
            if (end==NULL) end=p;
        } else if (c==':' || c=='\\' || c==']') {
            last_dot=NULL;
            root=++p;
            end=NULL;
            continue;
        } else {
            if (c==SwitchChar) {
                if (end==NULL) end=p;
                break;
            }
            if (c=='/') {
                last_dot=NULL;
                root=++p;
                end=NULL;
                continue;
            }
        }
        ++p;
    }
    if (name[0]=='.' && p-name==1) {
        IsStdIn=1;
        name=root="stdin";
        p=end=name+5;
    }
    length=end-root;
    SrcFName=(char *)CMemAlloc(length+1);
    memcpy(SrcFName,root,length);
    SrcFName[length]='\0';
    if (ModuleName==NULL) ModuleName=SrcFName;
    length=p-name;
    extension="";
    if (!IsStdIn && last_dot==NULL) extension=".c";
    WholeFName=(char *)CMemAlloc(length+(extension[0]?3:1));
    memcpy(WholeFName,name,length);
    strcpy(WholeFName+length,extension);
}

int OpenPgmFile(void)
{
    if (IsStdIn) return OpenFCB(stdin,"stdin");
    if (TryOpen("","",WholeFName,"")==0) {
        if (TryOpen("..\\c","\\",WholeFName,"")==0) {
            CantOpenFile(WholeFName);
            if (!CompFlags.command_line_processed) CSuicide();
            return 0;
        }
    }
    return 1;
}

int TryOpen(char *prefix,char *separator,char *filename,char *suffix)
{
    int i;
    char buffer[132];
    FILE *stream;
    i=0;
    while ((buffer[i]=*prefix++)!=0) ++i;
    while ((buffer[i]=*separator++)!=0) ++i;
    while ((buffer[i]=*filename++)!=0) ++i;
    while ((buffer[i]=*suffix++)!=0) ++i;
    stream=fopen(buffer,"rb");
    if (stream==NULL) return 0;
    if (CompFlags.use_precompiled_header) {
        CompFlags.use_precompiled_header=0;
        if (UsePreCompiledHeader(buffer)==0) {
            fclose(stream);
            return 1;
        }
    }
    if (OpenFCB(stream,buffer)) return 1;
    fclose(stream);
    return 0;
}

int OpenFCB(FILE *stream,char *name)
{
    if (!FCB_Alloc(stream,name)) {
        CErr1(0x428);
        return 0;
    }
    return 1;
}

int TokenFno;
char *ErrFName;
int FCB_Alloc(FILE *stream,char *filename)
{
    FCB *file;
    char *buffer;
    struct fname_list *flist;
    file=(FCB *)CMemAlloc(sizeof(FCB));
    buffer=(char *)FEmalloc(4098);
    if (file) {
        file->src_buf=file->src_ptr=buffer;
        buffer[0]='\0';
        if (SrcFile!=NULL) SrcFile->src_ptr=ScanCharPtr;
        ScanCharPtr=buffer;
        flist=AddFlist(filename);
        file->src_line=SrcFileLineNum=1;
        file->src_name=CStrSave(filename);
        file->src_fno=flist->index;
        TokenFno=flist->index;
        file->src_flist=flist;
        ErrFName=flist->name;
        file->src_fp=stream;
        file->prev_file=SrcFile;
        file->src_cnt=0;
        file->prev_currchar=CurrChar;
        file->src_bufsize=4096;
        file->peeking=0;
        if (SrcFile!=NULL && SrcFile==MainSrcFile) AddIncFileList(filename);
        CurrChar='\n';
        SrcFile=file;
        if (CompFlags.cpp_output) {
            if (CppFile==NULL) OpenCppFile();
            EmitPoundLine(1,filename);
        }
        return 1;
    }
    return 0;
}

struct fname_list *AddFlist(char *name)
{
    struct fname_list *entry,**owner;
    int index;
    owner=&FNames;
    index=0;
    while ((entry=*owner)!=NULL) {
        if (strcmp(name,entry->name)==0) break;
        ++index;
        owner=&entry->next;
    }
    if (entry==NULL) {
        entry=(struct fname_list *)CMemAlloc(offsetof(struct fname_list,name)+strlen(name)+1);
        entry->next=NULL;
        entry->index=index;
        strcpy(entry->name,name);
        *owner=entry;
        SrcFileTime(name,&entry->mtime);
    }
    return entry;
}

int SrcFileTime(char *name,unsigned *mtime)
{
    struct stat info;
    if (stat(name,&info)==0) {
        *mtime=info.st_mtime;
        return 0;
    }
    *mtime=0;
    return -1;
}

void DelErrFile(void)
{
    char name[80];
    if (!CompFlags.no_conmsg) {
        sprintf(name,"%s.err",SrcFName);
        remove(name);
    }
}

int OpenSrcFile(char *filename,int delimiter)
{
    int i,save;
    char *p,*drive,*dir,*name,*ext;
    char buff[_MAX_PATH+3],tryname[_MAX_PATH];
    FCB *curr;
    if (delimiter!='<') {
        /* This earlier search always tries cwd first, before later -I-
         * curdir control and the absolute-path special case were added. */
        if (TryOpen("","",filename,"")) return 1;
        _splitpath2(filename,buff,&drive,&dir,&name,&ext);
        if (drive[0]=='\0' && dir[0]!='/' && dir[0]!='\\') {
            for (curr=SrcFile;curr!=NULL;curr=curr->prev_file) {
                _splitpath2(curr->src_name,buff,&drive,&dir,&name,&ext);
                _makepath(tryname,drive,dir,filename,NULL);
                if (TryOpen("","",tryname,"")) return 1;
            }
        }
    }
    if (HFileList!=NULL) {
        p=HFileList;
        do {
            i=0;
            while (*p!=';' && *p!='\0') {
                if (i<sizeof(buff)-2) buff[i++]=*p;
                ++p;
            }
            buff[i]='\0';
            if (i>=1 && strcmp(buff+i-1,"\\")==0) buff[i-1]='\0';
            if (TryOpen(buff,"\\",filename,"")) return 1;
            if (*p==';') ++p;
        } while (*p!='\0');
    }
    if (delimiter!='<') {
        if (TryOpen("..\\h","\\",filename,"")) return 1;
    }
    save=CompFlags.cpp_output;
    if (CompFlags.cpp_output) {
        if (delimiter=='<') CppPrtf("#include <%s>",filename);
        else CppPrtf("#include \"%s\"",filename);
        CompFlags.cpp_output=0;
    }
    CErr2p(0x41f,filename);
    CompFlags.cpp_output=save;
    return 0;
}

INCFILE *IncFileList;
void AddIncFileList(char *filename)
{
    INCFILE *file,*previous;
    int len;
    len=strlen(filename);
    file=(INCFILE *)CMemAlloc(sizeof(INCFILE)+len);
    file->len=len;
    strcpy(file->filename,filename);
    file->nextfile=NULL;
    if (IncFileList==NULL) {
        IncFileList=file;
    } else {
        previous=IncFileList;
        while (previous->nextfile!=NULL) previous=previous->nextfile;
        previous->nextfile=file;
    }
}

/* In this version CreateFileName is inlined into ObjFileName. The SDK's
 * Win32 _MAX_PATH2 is263; its temporary occupies264 rounded stack bytes. */
char *ObjFileName(char *extension)
{
    char buffer[263];
    char *drive,*directory,*name,*ext,*path;
    path=ObjectFileName==NULL ? WholeFName:ObjectFileName;
    _splitpath2(path,buffer,&drive,&directory,&name,&ext);
    if (ObjectFileName==NULL || ext[0]=='\0') ext=extension;
    if (name[0]=='\0' || name[0]=='*') name=ModuleName;
    if (ObjectFileName==NULL) { drive="";directory=""; }
    _makepath(Buffer,drive,directory,name,ext);
    return Buffer;
}
