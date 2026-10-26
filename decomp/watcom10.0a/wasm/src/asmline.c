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
* Description: Original-source attribution retained for the adapted entry.
*
****************************************************************************/

/* Encountered StringBuf owner from bld/wasm/c/asmline.c and MAX_LINE_LEN256
 * in h/asmglob.h, commit6b9cb4438959d845a2b4dd4bf19f7908ecd608e9.
 * Actual CF pointer27110 is passed to AsmScan by StoreConstant. */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include "asminit.h"
#include "asmparse.h"
#include "direct.h"
#include <recon.h>

char StringBuf[256];

/* Parse phase owner normally resides in write.c; initial BSS value0. */
char Parse_Pass;

#ifdef __WATCOMC__
#pragma intrinsic(strlen,strcpy,strcat)
#endif

/* CF25c34; encountered include search-path owner. */
char *IncludePath;

/* CF85a8..86d5; the1994 delimiter is a single semicolon. */
void AddStringToIncludePath(char *string)
{
    char *tmp;
    while (isspace((unsigned char)*string)) ++string;
    if (IncludePath == NULL) {
        IncludePath = AsmAlloc(strlen(string) + 1);
        strcpy(IncludePath, string);
    } else {
        tmp = IncludePath;
        IncludePath = AsmAlloc(strlen(tmp) + strlen(";") + strlen(string) + 1);
        strcpy(IncludePath, tmp);
        strcat(IncludePath, ";");
        strcat(IncludePath, string);
        AsmFree(tmp);
    }
}

/* CF27218: stack of caller-owned macro/input line queues. */
input_queue *line_queue;

/* CF7c00..7c27: head/tail/next offsets0/4/8 in the actual12B owner. */
input_queue *PushLineQueue(void)
{
    input_queue *node;
    node = AsmAlloc(sizeof(*node));
    node->next = line_queue;
    node->head = node->tail = NULL;
    line_queue = node;
    return node;
}

/* Encountered CodeBuffer/BufSize live owners atCF27210/27214. */
char *CodeBuffer;
unsigned BufSize;

/* CF835c..83e3;1994 has a separate comment-preparation wrapper before
 * conditional preprocessing. Preserve the original returned line identity. */
char *ScanLine(char *string, int length)
{
    char *line;
    char buffer[256];
    line = length < 256 ? buffer : string;
    line = ReadTextLine(line);
    if (line != NULL) {
        PrepareTextLine(line);
        prep_line_for_conditional_assembly(line);
        if (line != string) {
            if (strlen(line) <= (unsigned)length) strcpy(string, line);
            else {
                AsmError(100); /* ASM_CODE_TOO_LONG */
                return NULL;
            }
        }
        join_multiline_cmds(string, length);
    }
    return line;
}

/* CF82a4..82d3: this older reader calls fgets directly; the released
 * get_asmline helper belongs to a later implementation. */
char *ReadTextLine(char *string)
{
    char *line;
    *string = '\0';
    line = input_get(string);
    if (line == NULL) {
        line = fgets(string, 256, AsmFiles.file[ASM]);
        if (line != NULL) ++LineNumber;
    }
    return line;
}

/* CF8340..8359; the separate1994 prepared-line reader used by macro_exam.
 * Preserve the original raw reader's result identity, including NULL. */
char *ReadPreparedLine(char *string)
{
    char *line = ReadTextLine(string);
    if (line != NULL) PrepareTextLine(line);
    return line;
}

/* Shipped18-byte file-stack owner: next0, stream/queue4, name8,
 * parent line12, is_a_file16 and hidden17. */
#pragma pack(1)
typedef struct file_list {
    struct file_list *next;
    union { FILE *file; input_queue *lines; } input;
    char *name;
    unsigned long line;
    char is_a_file;
    char hidden;
} file_list;
#pragma pack()
typedef char file_list_size_check[sizeof(file_list) == 18 ? 1 : -1];
static file_list *file_stack; /* CF25c30 */

/* CF7d64..7d7b; keep the original borrowed filename pointer. */
char *get_curr_filename(void)
{
    return file_stack == NULL ? AsmFiles.fname[ASM] : file_stack->name;
}

/* CF7d7c..7e12; actual1994 note IDs1146 and1147. */
void AsmNote(int message, ...);
void print_include_file_nesting_structure(void)
{
    file_list *node = file_stack;
    if (node == NULL) return;
    while (node->next != NULL) {
        if (!node->hidden)
            AsmNote(node->is_a_file ? 1146 : 1147,
                    node->next->name, node->line);
        node = node->next;
    }
    if (!node->hidden)
        AsmNote(node->is_a_file ? 1146 : 1147,
                AsmFiles.fname[ASM], node->line);
}

/* CF7c94..7d64; retained old initializer writes only. The hidden byte
 * belongs to PushMacro, not this allocator, in the shipped body. */
file_list *push_flist(char *name, unsigned char is_a_file)
{
    file_list *node;
    dir_node *dir;
    node = AsmAlloc(sizeof(*node));
    node->next = file_stack;
    node->is_a_file = is_a_file;
    node->line = LineNumber;
    file_stack = node;
    if (!is_a_file) {
        dir = (dir_node *)AsmGetSymbol(name);
        LineNumber = dir->e.macroinfo->start_line;
        node->name = AsmAlloc(strlen(dir->e.macroinfo->filename) + 1);
        strcpy(node->name, dir->e.macroinfo->filename);
    } else {
        node->name = AsmAlloc(strlen(name) + 1);
        strcpy(node->name, name);
        LineNumber = 0;
    }
    return node;
}

/* CF800c..80cb. The shipped caller reserves the NT260-byte path and
 *263-byte split buffer (rounded to264), despite other DOS CRT providers.
 * Adapted from bld/wasm/c/asmline.c at6b9cb443. */
int InputQueueFile(char *path)
{
    FILE *file;
    file_list *node;
    char fullpath[260];
    char buffer[263];
    char *drive, *dir, *fname, *ext;
    char *name;
    _splitpath2(path, buffer, &drive, &dir, &fname, &ext);
    _makepath(fullpath, drive, dir, fname, ext);
    file = fopen(fullpath, "r");
    name = path;
    if (file == NULL && IncludePath != NULL) {
        name = buffer;
        file = open_file_in_include_path(path, name);
    }
    if (file == NULL) {
        AsmError(1033); /* CANNOT_OPEN_INCLUDE_FILE */
        return -1;
    }
    node = push_flist(name, 1);
    node->input.file = file;
    return 1;
}


/* CF80cc..822b plus shared terminal7e0b..7e12. Adapted from the released
 * asmline.c, retaining the old direct-fgets and pre-result line increment. */
char *input_get(char *string)
{
    line_list *inputline;
    file_list *inputfile;
    Globals.expand_count = 0;
    while (line_queue != NULL) {
        if (line_queue->head != NULL) {
            inputline = line_queue->head;
            strcpy(string, inputline->line);
            line_queue->head = inputline->next;
            if (line_queue->head == NULL) line_queue->tail = NULL;
            AsmFree(inputline->line);
            AsmFree(inputline);
            return string;
        }
        PopLineQueue();
    }
    while (file_stack != NULL) {
        inputfile = file_stack;
        if (inputfile->is_a_file) {
            char *line;
            line = fgets(string, 256, inputfile->input.file);
            ++LineNumber;
            if (line != NULL) return line;
            file_stack = inputfile->next;
            fclose(inputfile->input.file);
            AsmFree(inputfile->name);
            LineNumber = inputfile->line;
            AsmFree(inputfile);
        } else {
            inputline = inputfile->input.lines->head;
            ++LineNumber;
            if (inputline != NULL) {
                strcpy(string, inputline->line);
                inputfile->input.lines->head = inputline->next;
                AsmFree(inputline->line);
                AsmFree(inputline);
                return string;
            }
            file_stack = inputfile->next;
            AsmFree(inputfile->name);
            AsmFree(inputfile->input.lines);
            LineNumber = inputfile->line;
            AsmFree(inputfile);
        }
    }
    return NULL;
}

/* CF7c28..7c3e: callers establish a nonempty stack; unlike the later
 * released version this body has no NULL guard or boolean return. */
void PopLineQueue(void)
{
    input_queue *old = line_queue;
    line_queue = old->next;
    AsmFree(old);
}

/* CF82d4..82e0: retain the physical old wrapper and its returned identity. */
char *PrepareTextLine(char *string)
{
    StripLineComment(string);
    return string;
}

/* CF822c..82a3: quote/angle-literal aware semicolon removal. A physical
 * newline terminates the line even inside a quoted or angle literal. */
void StripLineComment(char *string)
{
    char quote = 0;
    for (;; ++string) {
        switch (*string) {
        case '\0': return;
        case '\n': *string = '\0'; return;
        case ';':
            if (quote == 0) { *string = '\0'; return; }
            break;
        case '\'': case '"': case '`':
            if (quote == 0) quote = *string;
            else if (quote == *string) quote = 0;
            break;
        case '<': if (quote == 0) quote = '>'; break;
        case '>': if (quote == '>') quote = 0; break;
        }
    }
}

/* CF82e0..833e. Original defect: an empty line reads line[-1], initially
 * unwritten CodeBuffer[1011] in WriteObjModule, while checking whitespace
 * and comma continuation. Keep that bug in the Win32 source. Two ordinary
 * current-case original runs both took the no-join branch; native empty
 * lines explicitly use that defined no-join result, without reading or
 * reproducing the uninitialized predecessor byte. */
char *join_multiline_cmds(char *line, int length)
{
    char *ptr;
#ifdef __linux__
    if (*line == '\0') return line;
#endif
    ptr = line + strlen(line) - 1;
    while (isspace((unsigned char)*ptr)) --ptr;
    if (*ptr == ',')
        ScanLine(line + strlen(line), length - strlen(line));
    return line;
}

/* CF84f0..85a6: procedure substitution uses DefineProc at CF27287.
 * Use32 is the distinct byte at CF27285. */
void AsmLine(char *string)
{
    int count;
    Token_Count = AsmScan(string, StringBuf);
    Token_Count = ExpandMacro(Token_Count);
    if (ExpandTheWorld(1, 1) == -1) {
        write_to_file = 0;
        return;
    }
    if (CurrProc != NULL && !DefineProc) {
        for (count = 0; count < Token_Count; ++count) {
            switch (ExpandProcString(count)) {
            case -1: write_to_file = 0; return;
            case 2: return;
            }
        }
    }
    if (Token_Count > 0) {
        if (AsmParse() == -1) write_to_file = 0;
    } else if (Token_Count < 0) write_to_file = 0;
}

/* Initial asmline.c InputQueueLine; CF7e14..7e5d. */
void InputQueueLine(char *line)
{
    line_list *new;
    new = enqueue();
    new->line = AsmAlloc(strlen(line) + 1);
    strcpy(new->line,line);
}

/* Initial asmline.c enqueue, actual CF7c40..7c91. The empty-queue
 * creation is inline here in the shipped body, not a PushLineQueue call. */
line_list *enqueue(void)
{
    line_list *new;
    new = AsmAlloc(sizeof(*new));
    new->next = NULL;
    if (line_queue == NULL) {
        line_queue = AsmAlloc(sizeof(*line_queue));
        line_queue->next = NULL;
        line_queue->head = line_queue->tail = NULL;
    }
    if (line_queue->tail == NULL) {
        line_queue->head = line_queue->tail = new;
    } else {
        line_queue->tail->next = new;
        line_queue->tail = new;
    }
    return new;
}

/* CF8410..8443; data records are selected only on the code-segment path. */
void AsmDataByte(unsigned char value)
{
    if (Options.output_data_in_code_records && Globals.code_seg &&
        !Globals.data_in_code) OutSelect(1);
    AsmByte(value);
}

/* CF8444..84ee:1994 inlines the segment check and counts pass0 separately. */
void AsmByte(unsigned char value)
{
    if (CurrSeg != NULL) {
        CurrSeg->seg->e.seginfo->current_loc++;
        if (CurrSeg->seg->e.seginfo->current_loc >=
            CurrSeg->seg->e.seginfo->segrec->d.segdef.seg_length)
            CurrSeg->seg->e.seginfo->segrec->d.segdef.seg_length =
                CurrSeg->seg->e.seginfo->current_loc;
    }
    if (Parse_Pass != PASS_1) {
        if (CurrSeg == NULL) {
            if (CheckSeg) {
                AsmError(1038);
                write_to_file = 0;
                CheckSeg = 0;
            }
        } else if (write_to_file) {
            CodeBuffer[BufSize] = value;
            ++BufSize;
            if (BufSize >= 1012) FlushCurrSeg();
        }
    }
    PassTotal[Parse_Pass != PASS_1]++;
}

/* CF83e4..840f, same original byte-output selection as asmline.c. */
void AsmCodeByte(unsigned char value)
{
    if (Options.output_data_in_code_records && Globals.data_in_code == 1)
        OutSelect(0);
    AsmByte(value);
}

/* CF86e4..8704, queue ownership moves into the newly pushed file frame. */
void PushMacro(char *name, unsigned char hidden)
{
    file_list *new = push_flist(name,0);
    new->input.lines = line_queue;
    new->hidden = hidden;
    line_queue = line_queue->next;
}

/* CF86d8..86e2, same IncludePath owner used by path construction. */
void FreeIncludePath(void)
{
    AsmFree(IncludePath);
}

/* CF7e60..800c. The1994 search copy is stack-owned, not the later
 * AsmTmpAlloc pool. Its Windows separator is retained for shared path I/O. */
FILE *open_file_in_include_path(char *name, char *fullpath)
{
    char *inc_path_list;
    char *next_path;
    char buffer[256];
    FILE *file = NULL;

    while (isspace(*name)) name++;
    inc_path_list = alloca(strlen(IncludePath) + 1);
    strcpy(inc_path_list, IncludePath);
    next_path = strtok(inc_path_list, ";");
    while (file == NULL && next_path != NULL) {
        strcpy(buffer, next_path);
        if (buffer[strlen(buffer) - 1] != '\\') strcat(buffer, "\\");
        strcat(buffer, name);
        file = fopen(buffer, "r");
        if (file) break;
        next_path = strtok(NULL, ";");
    }
    strcpy(fullpath, buffer);
    return file;
}
