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

/* Adapted from bld/wasm/c/direct.c at initial commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9, CFfa3c..fce6.
 * The shipped compiler inlines dir_add; retain those operations in this body.
 * Allocation layouts and switch IDs are the 1994 layouts, not newer additions. */
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "direct.h"
#include "asmparse.h"
#include "breakout.h"
#include <recon.h>

#ifdef __WATCOMC__
/* Shipped bodies expand these string operations in place. */
#pragma intrinsic(strlen,strcpy,strchr,strcat,memcpy)
#endif

symbol_queue Tables[TAB_LAST];
unsigned LnamesIdx;
unsigned segdefidx, extdefidx;
static unsigned grpdefidx;
unsigned LineNumber;

dir_node *dir_insert(char *name, int tab)
{
    dir_node *new;
    asm_sym *sym;
    new = AsmAlloc(sizeof(dir_node));
    new->next = NULL;
    new->line = LineNumber;
    new->sym.name = AsmAlloc(strlen(name) + 1);
    strcpy(new->sym.name, name);
    new->next = new->prev = NULL;
    if (tab != TAB_CLASS_LNAME) sym = AsmAdd((asm_sym *)new);
    else sym = (asm_sym *)new;
    if (name == NULL) InternalError("..\\c\\direct.c", 412);
    switch (tab) {
    case TAB_SEG:
        new->e.seginfo = AsmAlloc(sizeof(seg_info));
        new->e.seginfo->lname_idx = 0;
        break;
    case TAB_GRP:
        new->e.grpinfo = AsmAlloc(sizeof(grp_info));
        new->e.grpinfo->idx = ++grpdefidx;
        new->e.grpinfo->seglist = NULL;
        new->e.grpinfo->numseg = 0;
        new->e.grpinfo->lname_idx = 0;
        break;
    case TAB_EXT:
        new->e.extinfo = AsmAlloc(sizeof(ext_info));
        break;
    case TAB_COMM:
        new->e.comminfo = AsmAlloc(sizeof(comm_info));
        new->e.comminfo->size = 1;
        break;
    case TAB_CONST:
        sym->state = SYM_CONST;
        sym->grpidx = sym->segidx = sym->offset = 0;
        new->e.constinfo = AsmAlloc(sizeof(const_info));
        return new;
    case TAB_PROC:
        new->e.procinfo = AsmAlloc(sizeof(proc_info));
        new->e.procinfo->regslist = NULL;
        new->e.procinfo->paralist = NULL;
        new->e.procinfo->locallist = NULL;
        break;
    case TAB_MACRO:
        new->e.macroinfo = AsmAlloc(sizeof(macro_info));
        new->e.macroinfo->parmlist = NULL;
        new->e.macroinfo->data = NULL;
        new->e.macroinfo->filename = NULL;
        new->e.macroinfo->start_line = LineNumber;
        break;
    case TAB_CLASS_LNAME:
    case TAB_LNAME:
        sym->state = tab == TAB_LNAME ? SYM_LNAME : SYM_CLASS_LNAME;
        new->e.lnameinfo = AsmAlloc(sizeof(lname_info));
        new->e.lnameinfo->idx = LnamesIdx;
        return new;
    case TAB_PUB:
        sym->state = SYM_UNDEFINED;
        sym->public = 1;
        return new;
    case TAB_GLOBAL:
        sym->state = SYM_UNDEFINED;
        return new;
    case TAB_STRUCT:
        sym->state = SYM_STRUCT;
        new->e.structinfo = AsmAlloc(sizeof(struct_info));
        new->e.structinfo->size = 0;
        new->e.structinfo->alignment = 0;
        new->e.structinfo->head = NULL;
        new->e.structinfo->tail = NULL;
        sym->total_size = sym->total_length = 0;
        sym->first_size = sym->first_length = 0;
        return new;
    }
    if (Tables[tab].head == NULL) {
        Tables[tab].head = Tables[tab].tail = new;
        new->prev = new->next = NULL;
    } else {
        new->prev = Tables[tab].tail;
        Tables[tab].tail->next = new;
        Tables[tab].tail = new;
        new->next = NULL;
    }
    return new;
}

/* CFf92c..f949; current object directive indices. */
void IdxInit(void)
{
    LnamesIdx = 0;
    segdefidx = 0;
    grpdefidx = 0;
    extdefidx = 0;
}

/* CF2662c: append-only queue of encountered object lname definitions. */
qdesc *LnameQueue = NULL;

/* CFffe8..100bb. Source MAX_LNAME255 and existing group/segment indices. */
int LnameInsert(char *name)
{
    asm_sym *sym;
    dir_node *dir;
    queuenode *node;
    sym = AsmGetSymbol(name);
    if (sym != NULL) {
        dir = (dir_node *)sym;
        if (sym->state == SYM_GRP) {
            if (dir->e.grpinfo->lname_idx == 0)
                dir->e.grpinfo->lname_idx = ++LnamesIdx;
        } else if (sym->state == SYM_SEG) {
            if (dir->e.seginfo->lname_idx == 0)
                dir->e.seginfo->lname_idx = ++LnamesIdx;
        } else return -1;
    } else {
        if (strlen(name) > 255) {
            AsmError(1006); /* LNAME_TOO_LONG */
            return -1;
        }
        ++LnamesIdx;
        dir = dir_insert(name, TAB_LNAME);
    }
    node = AsmAlloc(sizeof(*node));
    node->data = dir;
    if (LnameQueue == NULL) {
        LnameQueue = AsmAlloc(sizeof(*LnameQueue));
        QInit(LnameQueue);
    }
    QEnqueue(LnameQueue, node);
    return LnamesIdx;
}

module_info ModuleInfo;

/* CF11f2c..11f58: only two flag bits exist in this1994 initializer. */
void ModuleInit(void)
{
    ModuleInfo.model = 0;       /* MOD_NONE */
    ModuleInfo.distance = 0;    /* STACK_NONE */
    ModuleInfo.langtype = 0;    /* LANG_NONE */
    ModuleInfo.ostype = 0;      /* OPSYS_DOS */
    ModuleInfo.use32 = 0;
    ModuleInfo.init = 0;
}

assume_info AssumeTable[ASSUME_LAST];

/* CF122a8..122cc, six8-byte register assumptions. */
void AssumeInit(void)
{
    unsigned reg;
    for (reg = ASSUME_DS; reg <= ASSUME_CS; ++reg) {
        AssumeTable[reg].symbol = NULL;
        AssumeTable[reg].error = 0;
        AssumeTable[reg].flat = 0;
    }
}

/* CF139f0..13b0d, four-way table139e0..139f0. Query/finish paths retain
 * their original integer result; delimiter storage is a target byte. */
int Comment(int what_to_do, int index)
{
    static int in_comment; /* CF26730 */
    static unsigned char delim_char; /* CF279df */
    char *text;
    switch (what_to_do) {
    case 0: return in_comment;
    case 1: return delim_char;
    case 2:
        ++index;
        text = AsmBuffer[index]->string_ptr;
        if (text == NULL) {
            AsmError(1073);
            return -1;
        }
        delim_char = *(text + strspn(text, " \t"));
        if (delim_char == 0 || strchr(text, delim_char) == NULL) {
            AsmError(1073);
            return -1;
        }
        if (strchr(text, delim_char) == strrchr(text, delim_char))
            in_comment = 1;
        return 1;
    case 3:
        in_comment = 0;
        return 1;
    }
    return -1;
}

/* FreeInfo, actual CFfd1c..fee1, table CFfce8..fd1c.  List
 * element declarations expose only the prefixes consumed by this owner. */
void FreeInfo( dir_node *dir )
/****************************/
{
    int i;

    switch( dir->sym.state ) {
    case SYM_GRP:
        {
            seg_list    *segcurr;
            seg_list    *segnext;

            segcurr = dir->e.grpinfo->seglist;
            if( segcurr != NULL ) {
                for( ;; ) {
                    segnext = segcurr->next;
                    AsmFree( segcurr );
                    if( segnext == NULL ) break;
                    segcurr = segnext;
                }
            }
            AsmFree( dir->e.grpinfo );
        }
        break;
    case SYM_SEG:
        ObjKillRec( dir->e.seginfo->segrec );
        AsmFree( dir->e.seginfo );
        break;
    case SYM_EXTERNAL:
        AsmFree( dir->e.extinfo );
        break;
    case SYM_LNAME:
        AsmFree( dir->e.lnameinfo );
        break;
    case SYM_CONST:

        for( i=0; i < dir->e.constinfo->count; i++ ) {

            AsmFree( dir->e.constinfo->data[i].string_ptr );
        }
        AsmFree( dir->e.constinfo->data );
        AsmFree( dir->e.constinfo );
        break;
    case SYM_PROC:
        {
            label_list  *labelcurr;
            label_list  *labelnext;
            regs_list   *regcurr;
            regs_list   *regnext;

            labelcurr = dir->e.procinfo->paralist;
            if( labelcurr != NULL ) {
                for( ;; ) {
                    labelnext = labelcurr->next;
                    AsmFree( labelcurr->label );
                    AsmFree( labelcurr->replace );
                    AsmFree( labelcurr );
                    if( labelnext == NULL ) break;
                    labelcurr = labelnext;
                }
            }

            labelcurr = dir->e.procinfo->locallist;
            if( labelcurr != NULL ) {
                for( ;; ) {
                    labelnext = labelcurr->next;
                    AsmFree( labelcurr->label );
                    AsmFree( labelcurr->replace );
                    AsmFree( labelcurr );
                    if( labelnext == NULL ) break;
                    labelcurr = labelnext;
                }
            }

            regcurr = dir->e.procinfo->regslist;
            if( regcurr != NULL ) {
                for( ;; ) {
                    regnext = regcurr->next;
                    AsmFree( regcurr->reg );
                    AsmFree( regcurr );
                    if( regnext == NULL ) break;
                    regcurr = regnext;
                }
            }
            AsmFree( dir->e.procinfo );
        }
        break;
    case SYM_MACRO:
        {
            parm_list       *labelcurr;
            parm_list       *labelnext;
            asmlines        *datacurr;
            asmlines        *datanext;

            /* free the parm list */
            labelcurr = dir->e.macroinfo->parmlist;
            if( labelcurr != NULL ) {
                for( ;; ) {
                    labelnext = labelcurr->next;
                    AsmFree( labelcurr->label );
                    if( labelcurr->replace != NULL ) {
                        AsmFree( labelcurr->replace );
                    }
                    if( labelcurr->def != NULL ) {
                        AsmFree( labelcurr->def );
                    }
                    AsmFree( labelcurr );
                    if( labelnext == NULL ) break;
                    labelcurr = labelnext;
                }
            }

            /* free the lines list */
            datacurr = dir->e.macroinfo->data;
            if( datacurr != NULL ) {
                for( ;; ) {
                    datanext = datacurr->next;
                    AsmFree( datacurr->line );
                    AsmFree( datacurr );
                    if( datanext == NULL ) break;
                    datacurr = datanext;
                }
            }
            if( dir->e.macroinfo->filename != NULL ) {
                AsmFree( dir->e.macroinfo->filename );
            }
            AsmFree( dir->e.macroinfo );
        }
        break;
    case SYM_STRUCT:
        {
            field_list      *ptr;
            field_list      *next;

            for( ptr = dir->e.structinfo->head; ptr != NULL; ptr = next ) {
                AsmFree( ptr->initializer );
                AsmFree( ptr->value );
                next = ptr->next;
                AsmFree( ptr );
            }
            AsmFree( dir->e.structinfo );
        }
    default:
        break;
    }
}

/* Encountered segment/model/external/assume/language options. Other
 * directive domains remain unrecovered; their consumers must trap first. */
#define UNRECOVERED_OPTION {NULL,0,0}
typeinfo TypeInfo[63] = {
    {"READONLY",0,0},{"BYTE",1,1},{"WORD",2,1},{"DWORD",5,1},
    {"PARA",3,1},{"PAGE",4,1},{"PRIVATE",0,2},{"PUBLIC",2,2},
    {"STACK",5,2},{"COMMON",6,2},{"MEMORY",2,2},
    {"USE16",0,4},{"USE32",1,4},{"IGNORE",0,0},{"AT",0,0},
    UNRECOVERED_OPTION, /* TOK_CLASS has no string; not a lookup row. */
    {"TINY",1,INIT_MEMORY},{"SMALL",2,INIT_MEMORY},{"COMPACT",3,INIT_MEMORY},
    {"MEDIUM",5,INIT_MEMORY},{"LARGE",6,INIT_MEMORY},{"HUGE",7,INIT_MEMORY},
    {"FLAT",4,INIT_MEMORY},{"NEARSTACK",1,0x20},{"FARSTACK",2,0x20},
    {"NEAR",T_NEAR,0},{"FAR",T_FAR,0},{"PROC",T_PROC,0},
    {"BYTE",T_BYTE,0},{"SBYTE",T_BYTE,0},{"WORD",T_WORD,0},
    {"SWORD",T_WORD,0},{"DWORD",T_DWORD,0},{"SDWORD",T_DWORD,0},
    {"PWORD",T_FWORD,0},{"FWORD",T_FWORD,0},{"QWORD",T_QWORD,0},
    {"TBYTE",T_TBYTE,0},{"ABS",T_ABS2,0},
    {"DS",ASSUME_DS,0},{"ES",ASSUME_ES,0},{"SS",ASSUME_SS,0},
    {"FS",ASSUME_FS,0},{"GS",ASSUME_GS,0},{"CS",ASSUME_CS,0},
    {"ERROR",ASSUME_ERROR,0},{"NOTHING",ASSUME_NOTHING,0},
    {"FAR",T_FAR,0},{"NEAR",T_NEAR,0},
    {"BASIC",1,0},{"FORTRAN",2,0},{"PASCAL",3,0},{"C",4,0},
    {"WATCOM_C",5,0},{"STDCALL",6,0},{"SYSCALL",7,0},
    {"PRIVATE",0,0},{"PUBLIC",1,0},{"EXPORT",2,0},
    {"USES",0,0},{"VARARG",0,0},
    {"OS_OS2",1,0},{"OS_DOS",0,0}
};
#undef UNRECOVERED_OPTION
#pragma pack(1)
static struct last_seg_info {
    unsigned char seg;
    char close[1024];
    short stack_size;
} lastseg;
#pragma pack()

/* bld/wasm/c/direct.c Model, adapted against CF1206c..122a5.
 * This version has no cmdline flag.  FLAT creates its group index inline,
 * rather than the later DefFlatGroup call. */
int Model(int index)
{
    char *token;
    unsigned type;
    unsigned initstate = 0;
    if (ModuleInfo.init) {
        AsmError(1070);
        return -1;
    }
    get_module_name();
    for (++index; index < Token_Count; ++index) {
        token = AsmBuffer[index]->string_ptr;
        wipe_space(token);
        type = token_cmp(&token,TOK_TINY,TOK_FARSTACK);
        if (type == (unsigned)-1) {
            type = token_cmp(&token,TOK_PROC_BASIC,TOK_PROC_SYSCALL);
            if (type == (unsigned)-1) {
                type = token_cmp(&token,TOK_OS_OS2,TOK_OS_DOS);
                if (type == (unsigned)-1) {
                    AsmError(1016);
                    return -1;
                }
            }
        }
        MakeConstant(AsmBuffer[index]->value);
        /* Other TypeInfo domains are still unrecovered in both builds. */
        if (type >= 63 || TypeInfo[type].string == NULL) WASM_PENDING();
        if (initstate & TypeInfo[type].init) {
            AsmError(1002);
            return -1;
        }
        initstate |= TypeInfo[type].init;
        switch (type) {
        case TOK_FLAT:
            ModuleInfo.flat_idx = ++grpdefidx;
            LnameInsert("FLAT");
        case TOK_TINY: case TOK_SMALL: case TOK_COMPACT:
        case TOK_MEDIUM: case TOK_LARGE: case TOK_HUGE:
            ModuleInfo.model = TypeInfo[type].value;
            set_text_seg_name();
            module_prologue(ModuleInfo.model);
            break;
        case TOK_NEARSTACK: case TOK_FARSTACK:
            ModuleInfo.distance = TypeInfo[type].value;
            break;
        case TOK_PROC_BASIC: case TOK_PROC_FORTRAN: case TOK_PROC_PASCAL:
        case TOK_PROC_C: case TOK_PROC_WATCOM_C:
        case TOK_PROC_STDCALL: case TOK_PROC_SYSCALL:
            ModuleInfo.langtype = TypeInfo[type].value;
            break;
        case TOK_OS_OS2: case TOK_OS_DOS:
            ModuleInfo.ostype = TypeInfo[type].value;
            break;
        }
        ++index;
        if (index < Token_Count && AsmBuffer[index]->token != ',') {
            AsmError(33);
            return -1;
        }
    }
    if (!(initstate & INIT_MEMORY)) {
        AsmError(1032);
        return -1;
    }
    lastseg.seg = SIM_NONE;
    lastseg.stack_size = 0;
    ModuleInfo.init = 1;
    return 1;
}

/* CF11f58..11f99.  Unlike the released2002 source, the shipped
 * version only splits the filename; it does not sanitize identifier chars. */
void get_module_name(void)
{
    char dummy[256];
    if (AsmFiles.fname[ASM] == NULL)
        InternalError("..\\c\\direct.c",1961);
    _splitpath(AsmFiles.fname[ASM],NULL,NULL,ModuleInfo.name,dummy);
}

/* bld/wasm/c/direct.c wipe_space, actual CF10154..1019e. */
void wipe_space(char *token)
{
    char *start;
    if (token == NULL) return;
    if (strlen(token) == 0) return;
    for (start = token; ; ++start) {
        if (*start != ' ' && *start != '\t') break;
    }
    if (start == token) return;
    memmove(token,start,strlen(start) + 1);
}

/* Initial bld/wasm/c/direct.c token_cmp, CF10bd4..10c66. */
int token_cmp(char **token, int start, int end)
{
    int i;
    char *str = *token;
    if (*str == '\'' || *str == '"') {
        ++*token;
        for (str = *token; ; ++str) {
            if (*str == '\'' || *str == '"') {
                if (str[1] == '\0') break;
                *str = '\0';
            }
            if (*str == '\0') {
                AsmError(1003);
                return -1;
            }
        }
        *str = '\0';
        return 15; /* TOK_CLASS */
    }
    for (i = start; i <= end; ++i) {
        /* Unencountered option strings have no reconstructed data yet. */
        if (i < 0 || i >= 63 || TypeInfo[i].string == NULL) WASM_PENDING();
        if (stricmp(TypeInfo[i].string,str) == 0) return i;
    }
    return -1;
}

/* Encountered CODE/DATA spellings plus16-bit STACK/BSS generated by SimSeg.
 * Unencountered template rows stay NULL and are guarded before use. */
char *SimCodeBegin[2][7] = {
    {"_TEXT SEGMENT WORD PUBLIC 'CODE' IGNORE",
     "STACK SEGMENT PARA STACK 'STACK' IGNORE",
     "_DATA SEGMENT WORD PUBLIC 'DATA' IGNORE",
     "_BSS SEGMENT WORD PUBLIC 'BSS' IGNORE",NULL,NULL,NULL},
    {"_TEXT SEGMENT DWORD USE32 PUBLIC 'CODE' IGNORE",NULL,
     "_DATA SEGMENT DWORD USE32 PUBLIC 'DATA' IGNORE",NULL,NULL,NULL,NULL}
};

/* Initial direct.c set_text_seg_name, actual CF11f9c..12069. */
void set_text_seg_name(void)
{
    int length;
    if (Options.text_seg == NULL) {
        if (Use32 < 0 || Use32 >= 2) WASM_PENDING();
        if (ModuleInfo.model >= MOD_MEDIUM) {
            length = strlen(ModuleInfo.name) + 6;
            Options.text_seg = AsmAlloc(length);
            Options.text_seg[length - 1] = '\0';
            strcpy(Options.text_seg,ModuleInfo.name);
            strncat(Options.text_seg,SimCodeBegin[Use32][0],5);
        } else {
            Options.text_seg = AsmAlloc(6);
            Options.text_seg[5] = '\0';
            strncpy(Options.text_seg,SimCodeBegin[Use32][0],5);
        }
    }
}

char *SimCodeEnd[7] = {"_TEXT ENDS","STACK ENDS","_DATA ENDS","_BSS ENDS",NULL,NULL,NULL};

/* Initial direct.c module_prologue, actual CF11dac..11f2c and
 * its two branch tables at11d74..11dac. Shipped local is256B. */
void module_prologue(int type)
{
    int bit;
    char buffer[256];
    bit = find_bit();
    strcpy(buffer,Options.text_seg);
    strcat(buffer,SimCodeBegin[bit][0] + 5);
    InputQueueLine(buffer);
    strcpy(buffer + strlen(Options.text_seg),SimCodeEnd[0] + 5);
    InputQueueLine(buffer);
    if (type != MOD_TINY) {
        InputQueueLine(SimCodeBegin[bit][2]);
        InputQueueLine(SimCodeEnd[2]);
    }
    switch (type) {
    case MOD_TINY:
        InputQueueLine("DGROUP GROUP _TEXT");
        break;
    case MOD_SMALL: case MOD_COMPACT: case MOD_MEDIUM:
    case MOD_LARGE: case MOD_HUGE:
        InputQueueLine("DGROUP GROUP _DATA");
        break;
    }
    if (type == MOD_FLAT) {
        InputQueueLine("ASSUME CS:FLAT,DS:FLAT,SS:FLAT,ES:FLAT,FS:ERROR,GS:ERROR");
    } else {
        InputQueueLine("ASSUME DS:DGROUP, SS:DGROUP");
        switch (type) {
        case MOD_TINY:
            InputQueueLine("ASSUME CS:DGROUP, ES:DGROUP");
            break;
        case MOD_SMALL: case MOD_COMPACT: case MOD_MEDIUM:
        case MOD_LARGE: case MOD_HUGE:
            strcpy(buffer,"ASSUME CS:");
            strcat(buffer,Options.text_seg);
            InputQueueLine(buffer);
            break;
        }
    }
}

/* Initial direct.c find_bit; actual CF11424..1143a. */
int find_bit(void)
{
    return ModuleInfo.use32 ? 1 : 0;
}

/* Actual CF275d8: segment stack. Globals.code_seg atCF26320 is
 * already part of the shared packed state declared in wasm.h. */
seg_list *CurrSeg;

/* Initial direct.c SegDef, independently checked against CF10cd0..1139d. */
int SegDef( int i )
/*****************/
{
    char                defined = FALSE;
    char                *token;
    obj_rec             *seg;
    obj_rec             *oldobj;
    int          classidx;
    unsigned            type;           // type of option
    unsigned            initstate = 0;  // to show if a field is initialized
    char                oldreadonly;    // readonly value of a defined segment
    unsigned char       ignore;
    dir_node            *dirnode;
    char                *name;
    struct asm_sym      *sym;

    if( i < 0 ) {
        AsmError( 1036 );
        return( ERROR );
    }

    name = AsmBuffer[i]->string_ptr;

    switch( AsmBuffer[i+1]->value ) {

        case T_SEGMENT:
            seg = ObjNewRec( CMD_SEGDEF );

            /* Check to see if the segment is already defined */
            sym = AsmGetSymbol( name );
            if( sym != NULL && ( sym->state == SYM_SEG || sym->grpidx != 0 ) ) {
                // segment already defined
                dirnode = (dir_node *)sym;
                defined = TRUE;
                oldreadonly = dirnode->e.seginfo->readonly;
                ignore = dirnode->e.seginfo->ignore;
                if( dirnode->e.seginfo->lname_idx == 0 ) {
                    // segment was mentioned in a group statement, but not really set up
                    defined = FALSE;
                    LnameInsert( name );
                    seg->d.segdef.idx = ++segdefidx;
                }
            } else {
                if( sym != NULL && sym->state != SYM_SEG ) {
                    AsmTakeOut( name );
                }
                dirnode = dir_insert( name, TAB_SEG );
                seg->d.segdef.idx = ++segdefidx;
            }

            /* Setting up default values */
            if( !defined ) {
                seg->d.segdef.align = ALIGN_PARA;
                seg->d.segdef.combine = COMB_INVALID;
                seg->d.segdef.use_32 = ( (Code->info.cpu&P_CPU_MASK) >= P_386 ) ? TRUE : FALSE;
                seg->d.segdef.class_name_idx = 1;
                /* null class name, in case none is mentioned */
                dirnode->e.seginfo->readonly = FALSE;
            } else {
                oldobj = dirnode->e.seginfo->segrec;
                dirnode->e.seginfo->readonly = oldreadonly;
                seg->d.segdef.align = oldobj->d.segdef.align;
                seg->d.segdef.combine = oldobj->d.segdef.combine;
                if( !ignore ) {
                    seg->d.segdef.use_32 = oldobj->d.segdef.use_32;
                } else {
                    seg->d.segdef.use_32 = ( (Code->info.cpu&P_CPU_MASK) >= P_386 ) ? TRUE : FALSE;
                }
                seg->d.segdef.class_name_idx = oldobj->d.segdef.class_name_idx;
            }
            dirnode->e.seginfo->ignore = FALSE; // always false unless set explicitly
            seg->d.segdef.access_valid = FALSE;
            seg->d.segdef.seg_length = 0;

            if( lastseg.stack_size > 0 ) {
                seg->d.segdef.seg_length = lastseg.stack_size;
                lastseg.stack_size = 0;
            }

            i+=2; /* go past segment name and "SEGMENT " */

            for( ; i < Token_Count; i ++ ) {
                if( AsmBuffer[i]->token == T_STRING ) {
                    /* the class name - the only token which is of type STRING */
                    token = AsmBuffer[i]->string_ptr;

                    classidx = FindClassLnameIdx( token );
                    if( classidx == LNAME_NULL ) {
                        classidx = InsertClassLname( token );
                        if( classidx == LNAME_NULL ) return( ERROR );
                    }
                    seg->d.segdef.class_name_idx = classidx;
                    Globals.code_seg = ( stricmp( token, "code" ) == 0 );
                    continue;
                }

                /* go through all tokens EXCEPT the class name */
                token = AsmBuffer[i]->string_ptr;

                // look up the type of token
                type = token_cmp( &token, TOK_READONLY, TOK_AT );
                if( type == ERROR ) {
                    AsmError( 1009 );
                    return( ERROR );
                }

                /* initstate is used to check if any field is already
                   initialized */

                if( ( initstate & TypeInfo[type].init ) ) {
                    AsmError( 1001 ); // initialized already
                    return( ERROR );
                } else {
                    initstate |= TypeInfo[type].init; // mark it initialized
                }
                switch( type ) {
                    case TOK_READONLY:
                        dirnode->e.seginfo->readonly = TRUE;
                        break;
                    case TOK_BYTE:
                    case TOK_WORD:
                    case TOK_DWORD:
                    case TOK_PARA:
                    case TOK_PAGE:
                        seg->d.segdef.align = TypeInfo[type].value;
                        break;
                    case TOK_PRIVATE:
                    case TOK_PUBLIC:
                    case TOK_STACK:
                    case TOK_COMMON:
                    case TOK_MEMORY:
                        seg->d.segdef.combine = TypeInfo[type].value;
                        break;
                    case TOK_USE16:
                    case TOK_USE32:
                        seg->d.segdef.use_32 = TypeInfo[type].value;
                        break;
                    case TOK_IGNORE:
                        dirnode->e.seginfo->ignore = TRUE;
                        break;
                    case TOK_AT:
                        AsmError( 1004 );
                        return( ERROR );
                        break;
                    default:
                        AsmError( 1009 );
                        return( ERROR );
                }
            }

            if( defined && !ignore && !dirnode->e.seginfo->ignore ) {
                /* Check if new definition is different from previous one */

                oldobj = dirnode->e.seginfo->segrec;
                if( oldreadonly != dirnode->e.seginfo->readonly ||
                    oldobj->d.segdef.align != seg->d.segdef.align ||
                    oldobj->d.segdef.combine != seg->d.segdef.combine ||
                    oldobj->d.segdef.use_32 != seg->d.segdef.use_32 ||
                    oldobj->d.segdef.class_name_idx !=  seg->d.segdef.class_name_idx
                  ) {
                    ObjKillRec( seg );
                    AsmError( 1005 );
                    return( ERROR );
                } else {
                    // definition is the same as before
                    ObjKillRec( seg );
                    if( push_seg( dirnode ) == ERROR ) {
                        return( ERROR );
                    }
                }
            } else if( defined && ignore ) {
                /* reset to the new values */
                oldobj = dirnode->e.seginfo->segrec;
                oldobj->d.segdef.align = seg->d.segdef.align;
                oldobj->d.segdef.combine = seg->d.segdef.combine;
                oldobj->d.segdef.use_32 = seg->d.segdef.use_32;
                oldobj->d.segdef.class_name_idx = seg->d.segdef.class_name_idx;
                dirnode->sym.state = SYM_SEG;

                ObjKillRec( seg );
                if( push_seg( dirnode ) == ERROR ) {
                    return( ERROR );
                }
            } else if( defined && dirnode->e.seginfo->ignore ) {
                /* keep the old values */
                dirnode->e.seginfo->readonly = oldreadonly;
                dirnode->e.seginfo->ignore = ignore;
                ObjKillRec( seg );
                if( push_seg( dirnode ) == ERROR ) {
                    return( ERROR );
                }
            } else {
                /* A new definition */
                dirnode->e.seginfo->segrec = seg;
                dirnode->e.seginfo->start_loc = 0;
                dirnode->e.seginfo->current_loc = 0;
                if( push_seg( dirnode ) == ERROR ) {
                    return( ERROR );
                }
                sym = (struct asm_sym *)dirnode;
                if (sym->state != SYM_UNDEFINED) InternalError("..\\c\\direct.c",1565);
                sym->state = SYM_SEG;
                GetSymInfo( sym );
                sym->offset = 0;
                LnameInsert( name );

            }
            break;
        case T_ENDS:
            if( CurrSeg == NULL ) {
                AsmError( 1008 );
                return( ERROR );
            }

            sym = AsmGetSymbol( name );
            if( sym == NULL ) {
                AsmError( 1019 );
                return( ERROR );
            }
            if( (struct dir_node *)sym != CurrSeg->seg ) {
                AsmError( 1007 );
                return( ERROR );
            }
            pop_seg();
            if( CurrSeg == NULL ) break;
            if( CurrSeg->seg->e.seginfo->segrec->d.segdef.class_name_idx
                == FindClassLnameIdx( "code" ) ) {
                Globals.code_seg = TRUE;
            } else {
                Globals.code_seg = FALSE;
            }
            break;
        default:
            return( ERROR );
    }
    find_use32();
    return( NOT_ERROR );
}

/* Initial direct.c FindClassLnameIdx; CFffa4..ffe8. */
int FindClassLnameIdx(char *name)
{
    void **ptr;
    queuenode *node;
    dir_node *dir;
    if (LnameQueue == NULL) return 0;
    for (ptr = LnameQueue->head; ptr != NULL; ptr = *(void **)ptr) {
        node = (queuenode *)ptr;
        dir = (dir_node *)node->data;
        if (dir->sym.state != SYM_CLASS_LNAME) continue;
        if (stricmp(dir->sym.name,name) == 0) return dir->e.lnameinfo->idx;
    }
    return 0;
}

/* Initial direct.c InsertClassLname; CFff1c..ffa3. */
int InsertClassLname(char *name)
{
    dir_node *dir;
    queuenode *node;
    if (strlen(name) > 255) {
        AsmError(1006);
        return 0;
    }
    ++LnamesIdx;
    dir = dir_insert(name,TAB_CLASS_LNAME);
    node = AsmAlloc(sizeof(*node));
    node->data = (void *)dir;
    if (LnameQueue == NULL) {
        LnameQueue = AsmAlloc(sizeof(*LnameQueue));
        QInit(LnameQueue);
    }
    QEnqueue(LnameQueue,node);
    return LnamesIdx;
}

/* Initial direct.c push_seg; CFf968..f9a1. */
int push_seg(dir_node *segment)
{
    seg_list *curr;
    for (curr = CurrSeg; curr != NULL; curr = curr->next) {
        if (curr->seg == segment) {
            AsmError(1007);
            return -1;
        }
    }
    push((void **)&CurrSeg,segment);
    return 1;
}

/* Initial direct.c push; CFf94c..f966. */
void push(void **stack, void *element)
{
    stacknode *new;
    new = AsmAlloc(sizeof(*new));
    new->next = *stack;
    new->elt = element;
    *stack = new;
}

/* Initial direct.c GetSymInfo; CF139c0..139dd. */
void GetSymInfo(asm_sym *symbol)
{
    symbol->segidx = GetCurrSeg();
    symbol->grpidx = GetCurrGrp();
    symbol->offset = GetCurrAddr();
}

/* Initial direct.c GetCurrSeg; CF102dc..102fe. */
unsigned GetCurrSeg(void)
{
    if (CurrSeg == NULL) return 0;
    return CurrSeg->seg->e.seginfo->segrec->d.segdef.idx;
}

/* Initial direct.c GetCurrGrp; CF103cc..10413. */
unsigned GetCurrGrp(void)
{
    dir_node *grp;
    seg_list *segment, *curr;
    if (CurrSeg == NULL) return 0;
    curr = CurrSeg;
    for (grp = Tables[TAB_GRP].head; grp != NULL; grp = grp->next) {
        for (segment = grp->e.grpinfo->seglist; segment != NULL;
             segment = segment->next) {
            if (curr->seg == segment->seg) return grp->e.grpinfo->idx;
        }
    }
    return 0;
}

/* Initial direct.c GetCurrAddr; CF102c0..102da. */
unsigned long GetCurrAddr(void)
{
    if (CurrSeg == NULL) return 0;
    return CurrSeg->seg->e.seginfo->current_loc;
}

/* Initial direct.c find_use32; CF10c68..10c94. The old ModuleInfo
 * bitfield is signed, whereas SEGDEF.use_32 is unsigned. */
void find_use32(void)
{
    if (CurrSeg == NULL) Use32 = ModuleInfo.use32;
    else Use32 = CurrSeg->seg->e.seginfo->segrec->d.segdef.use_32;
}

/* Initial direct.c pop_seg; CFf9a4..f9c9. */
dir_node *pop_seg(void)
{
    if (CurrSeg == NULL) InternalError("..\\c\\direct.c",373);
    return (dir_node *)pop((void **)&CurrSeg);
}

/* Initial direct.c pop; CFf9f0..fa07. */
void *pop(void **stack)
{
    stacknode *node;
    void *element;
    node = (stacknode *)*stack;
    *stack = node->next;
    element = node->elt;
    AsmFree(node);
    return element;
}

/* Initial direct.c GrpDef adapted against CF10990..10b14.  The older
 * body inlines group creation, lacks the later symbol-kind rejections,
 * prepends new member links and uses current grpdefidx on segment entries. */
int GrpDef(int index)
{
    char *name;
    dir_node *grp;
    seg_list *new, *curr;
    if (index < 0) {
        AsmError(1037);
        return -1;
    }
    name = AsmBuffer[index]->string_ptr;
    grp = (dir_node *)AsmGetSymbol(name);
    if (grp == NULL) {
        grp = dir_insert(name,TAB_GRP);
        if (grp->sym.state != SYM_UNDEFINED)
            InternalError("..\\c\\direct.c",1213);
        grp->sym.segidx = 0;
        grp->sym.grpidx = grpdefidx;
        grp->sym.state = SYM_GRP;
        grp->sym.offset = 0;
        LnameInsert(name);
    }
    for (index += 2; index < Token_Count; index += 2) {
        name = AsmBuffer[index]->string_ptr;
        if (checkword(&name) == -1) {
            AsmError(33);
            return -1;
        }
        new = AsmAlloc(sizeof(*new));
        new->seg = (dir_node *)AsmGetSymbol(name);
        if (new->seg == NULL) {
            new->seg = dir_insert(name,TAB_SEG);
            new->seg->e.seginfo->lname_idx = 0;
        } else {
            new->seg->e.seginfo->idx = grpdefidx;
        }
        new->seg->sym.grpidx = grpdefidx;
        curr = grp->e.grpinfo->seglist;
        if (curr == NULL) {
            grp->e.grpinfo->seglist = new;
            new->next = NULL;
        } else {
            for (; curr != NULL; curr = curr->next) {
                if (curr->seg == new->seg) {
                    AsmFree(new);
                    new = NULL;
                    break;
                }
            }
            if (new == NULL) continue;
            new->next = grp->e.grpinfo->seglist;
            grp->e.grpinfo->seglist = new;
        }
        ++grp->e.grpinfo->numseg;
    }
    return 1;
}

/* Initial direct.c checkword; CF101a0..1025d. */
unsigned checkword(char **token)
{
    char *head, *end;
    for (head = *token; ; ++head) {
        if (*head != ' ' && *head != '\t') break;
    }
    end = strchr(head,' ');
    if (end == NULL) end = strchr(head,'\t');
    if (end == NULL) end = strchr(head,'\n');
    if (end != NULL) {
        *end++ = '\0';
        while (*end != '\0') {
            if (*end != ' ' && *end != '\t' && *end != '\n') return -1;
            ++end;
        }
    }
    *token = head;
    return 1;
}

/* Initial direct.c SetAssume, actual CF122cc..12447. This older body
 * does not skip a SEG unary token or create a group for FLAT. */
int SetAssume(int index)
{
    char *token, *segloc;
    unsigned reg;
    assume_info *info;
    asm_sym *sym;
    for (++index; index < Token_Count; ++index) {
        if (AsmBuffer[index]->token == T_RES_ID && AsmBuffer[index]->value == T_NOTHING) {
            AssumeInit();
            continue;
        }
        token = AsmBuffer[index]->string_ptr;
        wipe_space(token);
        ++index;
        if (AsmBuffer[index]->token != T_COLON) {
            AsmError(1021);
            return -1;
        }
        ++index;
        segloc = AsmBuffer[index]->string_ptr;
        ++index;
        if (*segloc == '\0') {
            AsmError(1039);
            return -1;
        }
        reg = token_cmp(&token,TOK_DS,TOK_CS);
        if (reg == (unsigned)-1) {
            AsmError(1040);
            return -1;
        }
        if (reg >= 63 || TypeInfo[reg].string == NULL) WASM_PENDING();
        info = &AssumeTable[TypeInfo[reg].value];
        if (reg <= TOK_CS) {
            if (token_cmp(&segloc,TOK_ERROR,TOK_ERROR) != -1) {
                info->error = 1;
            } else if (token_cmp(&segloc,TOK_FLAT,TOK_FLAT) != -1) {
                info->flat = 1;
            } else {
                sym = AsmGetSymbol(segloc);
                info->symbol = sym;
                info->error = 0;
            }
        } else {
            info->symbol = NULL;
            info->error = 0;
            info->flat = 0;
        }
        if (index < Token_Count && AsmBuffer[index]->token != ',') {
            AsmError(33);
            return -1;
        }
    }
    return 1;
}

/* Initial direct.c Include; CF113a0..113e0. */
int Include(int index)
{
    switch (AsmBuffer[index]->token) {
    case T_ID: case T_STRING: case T_PATH:
        return InputQueueFile(AsmBuffer[index]->string_ptr);
    default:
        AsmError(1115);
        return -1;
    }
}

/* CF26630; public-symbol queue has the same qdesc/node ownership as Lnames. */
qdesc *PubQueue;

/* CF1084c..1098f. The shipped body inlines MakePublic and always sets a
 * procedure's public visibility; there is no later export-preservation test. */
int PubDef(int i)
{
    char *mangle_type, *token;
    asm_sym *sym;
    dir_node *node;
    queuenode *item;
    mangle_type = Check4Mangler(&i);
    for (; i < Token_Count; i += 2) {
        token = AsmBuffer[i]->string_ptr;
        if (checkword(&token) == -1) { AsmError(33); return -1; }
        sym = AsmGetSymbol(token);
        if (sym != NULL) {
            node = (dir_node *)sym;
            if (sym->state == SYM_CONST && node->e.constinfo->data[0].token == T_ID) {
                ExpandTheWorld(i, 0);
                return PubDef(i);
            }
            switch (sym->state) {
            case SYM_UNDEFINED: case 1: /* SYM_INTERNAL */
            case SYM_EXTERNAL: case SYM_STACK: case SYM_CONST:
                if (sym->public) return 1;
                sym->public = 1;
                break;
            case SYM_PROC:
                sym->public = 1;
                node->e.procinfo->visibility = 1;
                break;
            }
        } else {
            node = dir_insert(token, TAB_PUB);
            if (node == (dir_node *)-1) return -1;
        }
        SetMangler(&node->sym, mangle_type);
        item = AsmAlloc(sizeof(*item));
        item->data = node;
        if (PubQueue == NULL) {
            PubQueue = AsmAlloc(sizeof(*PubQueue));
            QInit(PubQueue);
        }
        QEnqueue(PubQueue, item);
    }
    return 1;
}

/* CF107e4..10825; an optional quoted mangler precedes the public names. */
char *Check4Mangler(int *i)
{
    char *type = NULL;
    if (AsmBuffer[*i]->token == T_STRING) {
        type = AsmBuffer[*i]->string_ptr;
        ++*i;
        if (AsmBuffer[*i]->token != T_COMMA) AsmWarn(2, 33);
        else ++*i;
    }
    return type;
}

/* CF1066c..107e4. Unlike the later source there is no structure-qualified
 * fallback; the external type must match the original TypeInfo domain. */
int ExtDef(int i)
{
    char *token, *type_token, *mangle_type;
    int type, mem_type;
    asm_sym *sym;
    mangle_type = Check4Mangler(&i);
    for (; i < Token_Count; ++i) {
        token = AsmBuffer[i++]->string_ptr;
        if (AsmBuffer[i]->token != T_COLON) { AsmError(1021); return -1; }
        ++i;
        type_token = AsmBuffer[i]->string_ptr;
        type = token_cmp(&type_token, TOK_EXT_NEAR, TOK_EXT_ABS);
        if (type == -1) { AsmError(1023); return -1; }
        mem_type = TypeInfo[type].value;
        for (; i < Token_Count && AsmBuffer[i]->token != T_COMMA; ++i) ;
        sym = AsmGetSymbol(token);
        if (sym != NULL) {
            if (sym->mem_type == SYM_UNDEFINED) {
                if (MakeExtern(token, mem_type, 1) == NULL) return -1;
            } else if (sym->mem_type != mem_type) { AsmError(1025); return -1; }
        } else if (MakeExtern(token, mem_type, 0) == NULL) return -1;
    }
    sym = AsmGetSymbol(token);
    SetMangler(sym, mangle_type);
    return 1;
}

/* CF105b0..1066c. A redefined symbol's name has stack lifetime across
 * AsmTakeOut; the original malloc.h expands alloca with stackavail. */
asm_sym *MakeExtern(char *name, int type, unsigned char already_defined)
{
    dir_node *ext;
    asm_sym *sym;
    if (already_defined) {
        char *copy = alloca(strlen(name) + 1);
        strcpy(copy, name);
        AsmTakeOut(name);
        name = copy;
    }
    ext = dir_insert(name, TAB_EXT);
    if (ext == NULL) return NULL;
    sym = &ext->sym;
    ext->e.extinfo->idx = ++extdefidx;
    GetSymInfo(sym);
    sym->state = SYM_EXTERNAL;
    sym->offset = 0;
    sym->mem_type = type;
    return sym;
}

/* CF131c8..132ef;1994 does not call the later dir_add helper when
 * promoting an existing undefined symbol to a procedure. */
int ProcDef(int index)
{
    asm_sym *symbol;
    dir_node *node;
    char *name;
    if (CurrProc != NULL) push_proc(CurrProc);
    if (Parse_Pass == PASS_1) {
        if (index < 0) { AsmError(1054); return -1; }
        name = AsmBuffer[index]->string_ptr;
        symbol = AsmGetSymbol(name);
        if (symbol != NULL && symbol->state != SYM_UNDEFINED) {
            AsmError(1055);
            return -1;
        }
        if (symbol == NULL) {
            node = dir_insert(name, TAB_PROC);
            symbol = &node->sym;
        } else {
            node = (dir_node *)symbol;
            node->e.procinfo = AsmAlloc(sizeof(proc_info));
            node->e.procinfo->regslist = NULL;
            node->e.procinfo->paralist = NULL;
            node->e.procinfo->locallist = NULL;
            symbol->public = 1;
        }
        symbol->state = SYM_PROC;
        GetSymInfo(symbol);
        symbol->mem_type = proc_exam(index);
        if (symbol->mem_type == -1) return -1;
        return 1;
    }
    if (index < 0) InternalError("..\\c\\direct.c", 2860);
    symbol = AsmGetSymbol(AsmBuffer[index]->string_ptr);
    CurrProc = (dir_node *)symbol;
    if (CurrProc == NULL) InternalError("..\\c\\direct.c", 2863);
    GetSymInfo(symbol);
    DefineProc = 1;
    return 1;
}

/* proc_exam CF12cd8..131c6, table12ca4..12cd8. Compared to2002:
 * no unknown-token break, visibility replaces rather than widens,
 * and parameter entry asserts the comma. */
int proc_exam( int i )
{
    char        *name;
    char        *token;
    char        *typetoken;
    signed char       minimum;        // Minimum value of the type of token to be read

    dir_node    *dir;
    proc_info   *info;
    regs_list   *regist;
    regs_list   *temp_regist;
    label_list  *paranode;
    label_list  *paracurr;
    int         type;
    int         vis;
    struct asm_sym      *sym;

    name = AsmBuffer[i++]->string_ptr;
    dir = (dir_node *)AsmGetSymbol( name );
    info = dir->e.procinfo;

    minimum = TOK_PROC_FAR;

    info->langtype = ModuleInfo.langtype;

    // fixme ... we need error checking here --- for nested procs

    /* Obtain all the default value */

    if( ModuleInfo.model <= 4 ) {
        info->mem_type = T_NEAR;
    } else {
        info->mem_type = T_FAR;
    }
    info->visibility = dir->sym.public ? 1 : 0;
    info->parasize = 0;
    info->localsize = 0;
    info->is_vararg = 0;
    SetMangler( &dir->sym, NULL );

    /* Parse the definition line, except the parameters */
    for( i++; i < Token_Count && AsmBuffer[i]->token != T_COMMA; i++ ) {
        token = AsmBuffer[i]->string_ptr;
        if( AsmBuffer[i]->token == T_STRING ) {
            /* name mangling */
            SetMangler( &dir->sym, token );
            continue;
        }

        type = token_cmp( &token, TOK_PROC_FAR, TOK_PROC_USES );

        if( type < minimum ) {
            AsmError( 51 );
            return( -1 );
        }
        switch( type ) {
        case TOK_PROC_FAR:
        case TOK_PROC_NEAR:
            info->mem_type = TypeInfo[type].value;;
            minimum = TOK_PROC_BASIC;
            break;
        case TOK_PROC_BASIC:
        case TOK_PROC_FORTRAN:
        case TOK_PROC_PASCAL:
        case TOK_PROC_C:
        case TOK_PROC_WATCOM_C:
        case TOK_PROC_STDCALL:
        case TOK_PROC_SYSCALL:
            info->langtype = TypeInfo[type].value;
            minimum = TOK_PROC_PRIVATE;
            break;
        case TOK_PROC_PRIVATE:
        case TOK_PROC_PUBLIC:
        case TOK_PROC_EXPORT:
            vis = TypeInfo[type].value;
            info->visibility = vis;
            minimum = TOK_PROC_USES;
            break;
        case TOK_PROC_USES:
            for( i++; i < Token_Count && AsmBuffer[i]->token != T_COMMA; i++ ) {
                token = AsmBuffer[i]->string_ptr;
                regist = AsmAlloc( sizeof( regs_list ));
                regist->next = NULL;
                regist->reg = AsmAlloc( strlen(token) + 1 );
                strcpy( regist->reg, token );
                if( info->regslist == NULL ) {
                    info->regslist = regist;
                } else {
                    for( temp_regist = info->regslist;;
                            temp_regist = temp_regist->next ) {
                        if( temp_regist->next == NULL ) break;
                    }
                    temp_regist->next = regist;
                }
            }
            if( AsmBuffer[i]->token == T_COMMA ) i--;
            break;
        default:
            AsmError(51);
            return -1;
        }
    }

    CurrProc = dir;
    DefineProc = 1;

    if( i >= Token_Count ) {
        return( info->mem_type );
    } else if( info->langtype == 0 ) {
        AsmError( 1056 );
        return( -1 );
    } else {
        if (AsmBuffer[i]->token != T_COMMA)
            InternalError("..\\c\\direct.c", 2725);
        i++;
    }

    /* now parse parms */

    for( ; i < Token_Count; i++ ) {
        /* read symbol */
        token = AsmBuffer[i++]->string_ptr;

        /* read colon */
        if( AsmBuffer[i]->token != T_COLON ) {
            AsmError( 1021 );
            return( -1 );
        }
        i++;

        /* now read qualified type */
        typetoken = AsmBuffer[i]->string_ptr;

        type = token_cmp( &typetoken, TOK_EXT_BYTE, TOK_EXT_TBYTE );

        if( type == -1 ) {
            type = token_cmp( &typetoken, TOK_PROC_VARARG, TOK_PROC_VARARG );
            if( type == -1 ) {
                AsmError( 1023 );
                return( -1 );
            } else {
                if( info->langtype <= 3 ) {
                    AsmError( 1069 );
                    return( -1 );
                }
            }
        }

        sym = AsmLookup( token );
        if( sym == NULL ) return( -1 );

        if( sym->state != SYM_UNDEFINED ) {
            AsmError( 71 );
            return( -1 );
        } else {
            sym->state = SYM_INTERNAL;
        }

        paranode = AsmAlloc( 28 );
        paranode->is_vararg = type == TOK_PROC_VARARG ? 1 : 0;
        paranode->size = find_size( type );
        paranode->label = AsmAlloc( strlen( token ) + 1 );
        paranode->replace = NULL;
        strcpy( paranode->label, token );

        if( Use32 ) {
            info->parasize += ((paranode->size + 3) & ~3);
        } else {
            info->parasize += ((paranode->size + 1) & ~1);
        }
        info->is_vararg |= paranode->is_vararg;

        if( info->langtype >= 1 && info->langtype <= 3 ) {

            /* Parameters are stored in reverse order */
            paranode->next = info->paralist;
            info->paralist = paranode;

        } else {

            paranode->next = NULL;
            if( info->paralist == NULL ) {
                info->paralist = paranode;
            } else {
                for( paracurr = info->paralist;; paracurr = paracurr->next ) {
                    if( paracurr->next == NULL ) break;
                }
                paracurr->next = paranode;
            }
        }
        /* go past comma */
        i++;
        if( i < Token_Count && AsmBuffer[i]->token != T_COMMA ) {
            AsmError( 33 );
            return( -1 );
        }
    }
    return( info->mem_type );
}


/* WritePrologue CF13398..137e3;1994 locals already store their total
 * size, and concatenates the addressing fragments around sprintf(%d). */
int WritePrologue( void )
/***********************/
{
    char                buffer[80];
    regs_list           *regist;
    int                 len;
    proc_info           *info;
    label_list          *curr;
    long                offset;
    int                 retcode;
    unsigned char              parm_count = 0;
    int                 align = Use32 ? 4 : 2;

    if (CurrProc == NULL) InternalError("..\\c\\direct.c", 2922);
    info = CurrProc->e.procinfo;

    if( Parse_Pass == PASS_1 ) {

        /* Figure out the replacing string for local variables */
        offset = 0;
        for( curr = info->locallist; curr != NULL; curr = curr->next ) {
            offset += (curr->size + align - 1) & ~(align - 1);
            size_override( buffer, curr->size );
            strcat(buffer, Use32 ? " [ebp- " : " [bp- ");
            sprintf(buffer + strlen(buffer), "%d", offset);
            strcat(buffer, "] ");
            curr->replace = AsmAlloc( strlen( buffer ) + 1 );
            strcpy( curr->replace, buffer );

        }

        /* Figure out the replacing string for the parameters */

        if( info->mem_type == T_NEAR ) {
            offset = 4;         // offset from BP : return addr + old BP
        } else {
            offset = 6;
        }

        if( Use32 ) offset *= 2;

        for( curr = info->paralist; curr; curr = curr->next, parm_count++ ) {
            if( info->langtype == 5 ) {
                retcode = get_watcom_argument_string( buffer, (unsigned char)curr->size, &parm_count );
                if( retcode == -1 ) return( -1 );
            }
            if( info->langtype != 5 || retcode == 0 ) {
                size_override( buffer, curr->size );
                if( Use32 ) {
                    strcat( buffer, " [ebp+ " );
                } else {
                    strcat( buffer, " [bp+ " );
                }
                sprintf( buffer + strlen( buffer ), "%d", offset );
                strcat( buffer, "] " );

                offset += ((curr->size + align - 1) & ~(align - 1));
            }
            if( *buffer == '\0' ) {
                len = strlen( buffer+1 ) + 2;
                curr->replace = AsmAlloc( len );
                memcpy( curr->replace, buffer, len );
            } else {
                curr->replace = AsmAlloc( strlen( buffer ) + 1 );
                strcpy( curr->replace, buffer );
            }
        }
    }

    if( info->localsize != 0 || info->parasize != 0 || info->is_vararg ) {

        /* Push BP or EBP */
        if( Use32 ) {
            strcpy( buffer, "push ebp" );
        } else {
            strcpy( buffer, "push bp" );
        }
        InputQueueLine( buffer );

        /* Move SP/ESP to BP/EBP */
        if( Use32 ) {
            strcpy( buffer, "mov ebp, esp" );
        } else {
            strcpy( buffer, "mov bp, sp" );
        }
        InputQueueLine( buffer );

        /* Substract SP/ESP by the number of localbytes */
        if( info->localsize != 0 ) {
            if( Use32 ) {
                strcpy( buffer, "sub esp, " );
            } else {
                strcpy( buffer, "sub sp, " );
            }
            sprintf( buffer+strlen(buffer), "%d", info->localsize );
            InputQueueLine( buffer );
        }
    }

    /* Push the registers */
    strcpy( buffer, "push " );
    len = strlen( buffer );
    for( regist = info->regslist; regist; regist = regist->next ) {
        strcpy( buffer + len, regist->reg );
        InputQueueLine( buffer );
    }
    return( 1 );
}


/* CF12448..1247b, lookup by the owned segment record's16-bit index. */
dir_node *GetSeg(asm_sym *sym)
{
    dir_node *curr;
    unsigned idx = sym->segidx;
    for (curr = Tables[TAB_SEG].head; curr != NULL; curr = curr->next) {
        if (curr->sym.state == SYM_SEG &&
            idx == curr->e.seginfo->segrec->d.segdef.idx) return curr;
    }
    return NULL;
}

/* CF126dc..1279f, preserve the actual byte-sized Frame_Datum stores. */
unsigned GetAssume( struct asm_sym* sym, unsigned def )
/**********************************************/
{
    unsigned        reg;

#ifdef __linux__
    /* In the original layout, def8's flag DWORD is274cc, exactly the
     * existing ModuleInfo.flat_idx owner. Native globals are laid out
     * differently: project only the actually consumed bit1 of that field.
     * Other unchecked selectors remain unsupported; period access stays below. */
    if( def >= ASSUME_LAST && def != ASSUME_NOTHING ) WASM_PENDING();
    if( def == ASSUME_NOTHING ? (ModuleInfo.flat_idx & 2) != 0 :
        AssumeTable[def].flat ) {
#else
    if( AssumeTable[def].flat ) {
#endif
        Frame = 1;
        Frame_Datum = ModuleInfo.flat_idx;
        return( def );
    }

    reg = search_assume( get_grp( sym ), def );

    if( reg != ASSUME_NOTHING ) {

        Frame = 1;
        Frame_Datum = sym->grpidx;

    } else if( reg == ASSUME_NOTHING ) {

        reg = search_assume( GetSeg( sym ), def );
        if( reg != ASSUME_NOTHING ) {
            if( Frame == EMPTY ) {
                Frame = 0;
                Frame_Datum = sym->segidx;
            }
        } else {
            if( sym->state == SYM_EXTERNAL ) {
                if( Frame == EMPTY ) {
                    Frame = 2;
                    Frame_Datum = GetDirIdx( sym->name, TAB_EXT );
                }
            }
        }

    }

    return( reg );
}

/* CF1247c..124b0, borrow group index from the segment only when absent. */
dir_node *get_grp(asm_sym *sym)
{
    dir_node *curr;
    unsigned idx = sym->grpidx;
    if (idx == 0) {
        curr = GetSeg(sym);
        if (curr != NULL) idx = curr->e.seginfo->idx;
    }
    for (curr = Tables[TAB_GRP].head; curr != NULL; curr = curr->next) {
        if (idx == curr->e.grpinfo->idx) return curr;
    }
    return NULL;
}

/* CF1259c..125f4, prefer the default assumed register before the search. */
unsigned search_assume(dir_node *segment, unsigned def)
{
    unsigned reg;
    unsigned char found = 0;
    if (segment == NULL) return ASSUME_NOTHING;
    if (def != ASSUME_NOTHING && AssumeTable[def].symbol == (asm_sym *)segment) {
        reg = def;
        found = 1;
    }
    if (!found) {
        for (reg = ASSUME_DS; reg <= ASSUME_CS; ++reg) {
            if (AssumeTable[reg].symbol == (asm_sym *)segment) {
                found = 1;
                break;
            }
        }
    }
    return found ? reg : ASSUME_NOTHING;
}

/* CF138e8..139bd. The original calls write_epilogue before examining count. */
int Ret(int i, int count)
{
    char buffer[20];
    proc_info *info = CurrProc->e.procinfo;
    if (info->mem_type == T_NEAR) strcpy(buffer,"retn ");
    else strcpy(buffer,"retf ");
    write_epilogue();
    if (count == i + 1) {
        if ((info->langtype >= 1 && info->langtype <= 3) ||
            (info->langtype == 6 && !info->is_vararg)) {
            if (info->parasize != 0)
                sprintf(buffer + strlen(buffer),"%d",info->parasize);
        }
    } else {
        if (AsmBuffer[i+1]->token != T_NUM) {
            AsmError(117);
            return ERROR;
        }
        sprintf(buffer + strlen(buffer),"%d",AsmBuffer[i+1]->value);
    }
    InputQueueLine(buffer);
    return NOT_ERROR;
}

/* CF13844..138e7. */
void write_epilogue(void)
{
    char buffer[80];
    proc_info *info;
    if (CurrProc == NULL) InternalError("..\\c\\direct.c",3040);
    info = CurrProc->e.procinfo;
    pop_register(info->regslist);
    if (info->localsize == 0 && info->parasize == 0 && !info->is_vararg)
        return;
    if (info->localsize != 0) {
        if (Use32) strcpy(buffer,"mov esp, ebp");
        else strcpy(buffer,"mov sp, bp");
        InputQueueLine(buffer);
    }
    if (Use32) strcpy(buffer,"pop ebp");
    else strcpy(buffer,"pop bp");
    InputQueueLine(buffer);
}

/* CF137e4..13841. Recursion preserves reverse register restoration order. */
void pop_register(struct regs_list *regist)
{
    char buffer[20];
    if (regist == NULL) return;
    pop_register(regist->next);
    strcpy(buffer,"pop ");
    strcpy(buffer + strlen(buffer),regist->reg);
    InputQueueLine(buffer);
}

/* CF132f0..13395, 1994 procedure close. */
int ProcEnd(int i)
{
    proc_info *info;
    label_list *curr;
    asm_sym *sym;
    if (CurrProc == NULL) { AsmError(1067); return ERROR; }
    if (i < 0) { AsmError(1054); return ERROR; }
    info = CurrProc->e.procinfo;
    sym = AsmGetSymbol(AsmBuffer[i]->string_ptr);
    if ((dir_node *)sym != CurrProc) { AsmError(1068); return ERROR; }
    if (Parse_Pass == PASS_1) {
        for (curr = info->paralist; curr != NULL; curr = curr->next)
            AsmTakeOut(curr->label);
        for (curr = info->locallist; curr != NULL; curr = curr->next)
            AsmTakeOut(curr->label);
    }
    CurrProc = pop_proc();
    return NOT_ERROR;
}

/* CF2663c, shared nested-procedure stack owner. */
static void *ProcStack;

/* CFf9cc..f9db. */
void push_proc(dir_node *procedure)
{
    push(&ProcStack, procedure);
}
/* CFf9dc..f9f0 falls into the existing pop entry in the original. */
dir_node *pop_proc(void)
{
    if (ProcStack == NULL) return NULL;
    return (dir_node *)pop(&ProcStack);
}

/* CFfee4..ff1a, original direct_idx is a signed32-bit int. */
int GetLnameIdx(char *name)
{
    asm_sym *sym = AsmGetSymbol(name);
    dir_node *dir;
    if (sym == NULL) return LNAME_NULL;
    dir = (dir_node *)sym;
    if (sym->state == SYM_UNDEFINED) return LNAME_NULL;
    if (sym->state == SYM_GRP) return dir->e.grpinfo->lname_idx;
    if (sym->state == SYM_SEG) return dir->e.seginfo->lname_idx;
    return dir->e.lnameinfo->idx;
}

/* CF10b14..10bd1, second-pass segment selection. */
int SetCurrSeg(int i)
{
    char *name = AsmBuffer[i]->string_ptr;
    asm_sym *sym;
    Use32 = ModuleInfo.use32;
    if (write_to_file) {
        switch (AsmBuffer[i+1]->value) {
        case T_SEGMENT:
            FlushCurrSeg();
            sym = AsmGetSymbol(name);
            if (sym == NULL) InternalError("..\\c\\direct.c",1291);
            push_seg((dir_node *)sym);
            break;
        case T_ENDS:
            FlushCurrSeg();
            pop_seg();
            break;
        }
    }
    if (CurrSeg != NULL) {
        Globals.code_seg = CurrSeg->seg->e.seginfo->segrec->d.segdef.class_name_idx
                           == FindClassLnameIdx("code");
    }
    find_use32();
    return NOT_ERROR;
}

/* CF10374..103c9; current buffer must start at the owned segment offset. */
unsigned long GetCurrSegStart(void)
{
    if (CurrSeg == NULL) return 0;
    if (write_to_file && CurrSeg->seg->e.seginfo->current_loc - BufSize !=
            CurrSeg->seg->e.seginfo->start_loc)
        InternalError("..\\c\\direct.c",934);
    return CurrSeg->seg->e.seginfo->start_loc;
}

/* CF10260..102bf; indices use their original owning directory records. */
unsigned GetDirIdx(char *name, int table)
{
    dir_node *dir = (dir_node *)AsmGetSymbol(name);
    if (dir == NULL) InternalError("..\\c\\direct.c",872);
    switch (table) {
    case TAB_SEG: return dir->e.seginfo->segrec->d.segdef.idx;
    case TAB_EXT: return dir->e.extinfo->idx;
    case TAB_COMM: return dir->e.comminfo->idx;
    case TAB_GRP: return dir->e.grpinfo->idx;
    default: InternalError("..\\c\\direct.c",884);
    }
    return ERROR;
}

/* CF266a4/266a8 shared startup directive owners. Later startup producers
 * must use these same objects rather than duplicate state. */
static char *StartAddr = "`symbol_reserved_for_start_address`";
char StartupDirectiveFound;
/* CF127a0..12917, original module END directive. */
int ModuleEnd( int count )
/*************************/
{
    struct fixup        *fixup;

    if( lastseg.seg != SIM_NONE ) {
        close_lastseg();
        InputQueueLine( CurrString );
        return( NOT_ERROR );
    }
    EndDirectiveFound = TRUE;

    if( Parse_Pass == PASS_1 ) return( NOT_ERROR );

    if( StartupDirectiveFound ) {
        if( count == 2 ) {
            AsmError( 51 );
            return( ERROR );
        }
        strcat( CurrString, " " );
        strcat( CurrString, StartAddr );
        InputQueueLine( CurrString );
        StartupDirectiveFound = FALSE;
        return( NOT_ERROR );
    }

    ModendRec = ObjNewRec( CMD_MODEND );
    ModendRec->d.modend.main_module = FALSE;

    if( count == 1 ) {
        ModendRec->d.modend.start_addrs = FALSE;
        return( NOT_ERROR );
    }

    ModendRec->d.modend.start_addrs = TRUE;
    ModendRec->d.modend.is_logical = TRUE;

    if( AsmBuffer[1]->token != T_ID ) {
        AsmError( 1048 );
        return( ERROR );
    }
    ModendRec->d.modend.main_module = TRUE;

    fixup = CreateFixupRec( 0 );
    if( fixup == NULL ) {
        return( ERROR );
    }
    ModendRec->d.modend.ref.log = fixup->lr;
    return( NOT_ERROR );
}

/* CF100bc..100df, shared terminal RET100ba with LnameInsert. */
void FreePubQueue(void)
{
    if (PubQueue != NULL) {
        while (PubQueue->head != NULL) AsmFree(QDequeue(PubQueue));
        AsmFree(PubQueue);
    }
}

/* CF100e0..10103, sharing RET100ba with LnameInsert/FreePubQueue. */
void FreeAliasQueue(void)
{
    if (AliasQueue != NULL) {
        while (AliasQueue->head != NULL) AsmFree(QDequeue(AliasQueue));
        AsmFree(AliasQueue);
    }
}

/* CF10104..10151, owns class-only LNAME symbols outside sym_table. */
void FreeLnameQueue(void)
{
    dir_node *dir;
    queuenode *node;
    if (LnameQueue != NULL) {
        while (LnameQueue->head != NULL) {
            node = QDequeue(LnameQueue);
            dir = node->data;
            if (dir->sym.state == SYM_CLASS_LNAME) {
                AsmFree(dir->e.lnameinfo);
                AsmFree(dir->sym.name);
                AsmFree(dir);
            }
            AsmFree(node);
        }
        AsmFree(LnameQueue);
    }
}

/* CF13c3c..13ca3. AsmBuffer base26bb0 and operand26bb4+4*index
 * select the following module-name token, as in the released source. */
int NameDirective(int index)
{
    if (Options.module_name == NULL) {
        Options.module_name = AsmAlloc(strlen(AsmBuffer[index + 1]->string_ptr) + 1);
        strcpy(Options.module_name,AsmBuffer[index + 1]->string_ptr);
    }
    return NOT_ERROR;
}

/* CF113e0..11423, encountered library directive.1994 compares the
 * insertion result to ERROR(-1), rather than the later NULL test. */
int IncludeLib(int index)
{
    char *name = AsmBuffer[index]->string_ptr;
    if (name == NULL) {
        AsmError(1035);
        return ERROR;
    }
    if (AsmGetSymbol(name) == NULL) {
        if (dir_insert(name,TAB_LIB) == (dir_node *)ERROR) return ERROR;
    }
    return NOT_ERROR;
}

/* CF13d80..13f84, source-backed COMMON symbol parsing. */
int CommDef( int i )
/******************/
{
    char        *token;
    char        *mangle_type = NULL;
    char        *typetoken;
    int         type;
    int         distance;
    int         count;
    struct asm_sym *sym;

    mangle_type = Check4Mangler( &i );
    for( ; i < Token_Count; i++ ) {
        count = 1;
        /* get the distance ( near or far ) */

        typetoken = AsmBuffer[i]->string_ptr;
        distance = token_cmp( &typetoken, TOK_EXT_NEAR, TOK_EXT_FAR );

        if( distance != ERROR ) {
            /* otherwise, there is no distance specified */
            i++;
        } else {
            distance = TOK_EXT_NEAR;
        }

        /* get the symbol name */
        token = AsmBuffer[i++]->string_ptr;

        /* go past the colon */
        if( AsmBuffer[i]->token != T_COLON ) {
            AsmError( 1021 );
            return( ERROR );
        }
        i++;
        typetoken = AsmBuffer[i]->string_ptr;
        type = token_cmp( &typetoken, TOK_EXT_BYTE, TOK_EXT_TBYTE );

        if( type == ERROR ) {
            AsmError( 1023 );
            return( ERROR );
        }
        for( ; i< Token_Count && AsmBuffer[i]->token != T_COMMA; i++ ) {
            if( AsmBuffer[i]->token == T_COLON ) {
                i++;
                /* count */
                if( AsmBuffer[i]->token != T_NUM ) {
                    AsmError( 34 );
                    return( ERROR );
                }
                count = AsmBuffer[i]->value;
            }
        }

        sym = AsmGetSymbol( token );

        if( sym != NULL ) {
            SetMangler( sym, mangle_type );
            if( sym->mem_type == SYM_UNDEFINED ) {
                if( MakeComm( token, TypeInfo[type].value, TRUE, count,
                    TypeInfo[distance].value ) == ERROR ) {
                    return( ERROR );
                }
            } else if( sym->mem_type != TypeInfo[type].value ) {
                AsmError( 1025 );
                return( ERROR );
            }
        } else {
            if( MakeComm( token, TypeInfo[type].value, FALSE, count,
                    TypeInfo[distance].value ) == ERROR ) {
                return( ERROR );
            }
            sym = AsmGetSymbol( token );
            SetMangler( sym, mangle_type );
        }
    }
    return( NOT_ERROR );
}

/* CF13ca4..13d7e, five-argument Watcom register/stack ABI (RET4).
 * The1994 temporary is checked stack alloca, not later AsmTmpAlloc. */
int MakeComm(char *name, int type, unsigned char already_defd, int number, int distance)
{
    dir_node *dir;
    asm_sym *sym;
    if (already_defd) {
        char *tmp = alloca(strlen(name) + 1);
        strcpy(tmp,name);
        AsmTakeOut(name);
        name = tmp;
    }
    dir = dir_insert(name,TAB_COMM);
    if (dir == NULL) return ERROR;
    sym = &dir->sym;
    dir->e.comminfo->idx = ++extdefidx;
    dir->e.comminfo->size = number;
    dir->e.comminfo->distance = distance;
    GetSymInfo(sym);
    sym->state = SYM_EXTERNAL;
    sym->offset = 0;
    sym->mem_type = type;
    return NOT_ERROR;
}

/* CF10414..105b0; use the sole existing GlobalQueue owner. */
int GlobalDef( int i )
/********************/
{
    char        *token;
    char        *mangle_type = NULL;
    char        *typetoken;
    int         type;
    struct asm_sym *sym;
    dir_node    *dir;
    struct queuenode    *qnode;

    mangle_type = Check4Mangler( &i );
    for( ; i < Token_Count; i++ ) {
        /* get the symbol name */
        token = AsmBuffer[i++]->string_ptr;

        /* go past the colon */
        if( AsmBuffer[i]->token != T_COLON ) {
            AsmError( 1021 );
            return( ERROR );
        }
        i++;

        typetoken = AsmBuffer[i]->string_ptr;
        type = token_cmp( &typetoken, TOK_EXT_NEAR, TOK_EXT_ABS );

        if( type == ERROR ) {
            AsmError( 1023 );
            return( ERROR );
        }
        for( ; i< Token_Count && AsmBuffer[i]->token != T_COMMA; i++ );

        sym = AsmGetSymbol( token );

        if( sym != NULL && sym->mem_type != SYM_UNDEFINED ) {
            return( PubDef( 0 ) ); // it is defined here, so make a pubdef
        }

        if( sym == NULL ) {
            dir = dir_insert( token, TAB_GLOBAL );
            sym = (struct asm_sym*)dir;
        } else {
            dir = (dir_node*)sym;
        }

        if( dir == NULL ) return( ERROR );

        GetSymInfo( sym );
        sym->state = SYM_UNDEFINED;
        sym->offset = 0;
        sym->mem_type = TypeInfo[type].value;

        SetMangler( sym, mangle_type );

        /* put the symbol in the globaldef table */
        qnode = AsmAlloc( sizeof( queuenode ) );
        qnode->data = (void *)dir;

        if( GlobalQueue == NULL ) {
            GlobalQueue = AsmAlloc( sizeof( qdesc ) );
            QInit( GlobalQueue );
        }
        QEnqueue( GlobalQueue, qnode );
    }
    return( NOT_ERROR );
}

/* CF116e8..11d74 with model table116cc..116e8. The1994 structure has
 * no latercmdline field; only actual classified template rows may be used. */
int SimSeg( int i )
/*****************/
/* Handles simplified segment, based on optasm pg. 142-146 */
{
    char        buffer[256];
    int         bit;
    char        *string;
    int         type;
    int         seg;

    if( !ModuleInfo.init ) {
        AsmError( 1071 );
        return( ERROR );
    }

    if( AsmBuffer[i]->value != T_STACK ) {
        close_lastseg();
    }
    buffer[0] = '\0';
    bit = find_bit();
    type = AsmBuffer[i]->value;
    i++; /* get past the directive token */
    if( i < Token_Count ) {
        string = AsmBuffer[i]->string_ptr;
    } else {
        switch( type ) {
        case T_CODE:
            string = Options.text_seg;
            break;
        case T_DATA:
            string = Options.data_seg;
            break;
        default:
            string = NULL;
        }
    }

    switch( type ) {
    case T_CODE:
        strcpy( buffer, string );
        /* we already have a name */
        strcat( buffer, SimCodeBegin[bit][SIM_CODE] + SIM_CODE_OFFSET  );
        InputQueueLine( buffer );
        strcpy( lastseg.close, string );
        strcat( lastseg.close, SimCodeEnd[SIM_CODE] + SIM_CODE_OFFSET );
        lastseg.seg = SIM_CODE;

        switch( ModuleInfo.model ) {
            case MOD_TINY:
                InputQueueLine( "ASSUME CS:DGROUP" );
                break;
            case MOD_SMALL:
            case MOD_COMPACT:
            case MOD_FLAT:                              // may not be correct
                strcpy( buffer, "ASSUME CS: " );
                if( string != NULL ) {
                    strcat( buffer, string );
                } else {
                    strcat( buffer, "_TEXT" );
                }
                InputQueueLine( buffer );
                break;
            case MOD_MEDIUM:
            case MOD_LARGE:
            case MOD_HUGE:
                strcpy( buffer, "ASSUME CS:" );
                if( string != NULL ) {
                    strcat( buffer, string );
                } else {
                    strcat( buffer, ModuleInfo.name );
                    strcat( buffer, "_TEXT" );
                }
                InputQueueLine( buffer );
                break;
            default:
                break;
        }
        break;
    case T_STACK:
        if (SimCodeBegin[bit][SIM_STACK] == NULL) WASM_PENDING();
        InputQueueLine( SimCodeBegin[bit][SIM_STACK] );
        input_group( type );
        InputQueueLine( SimCodeEnd[SIM_STACK] );
        if( i < Token_Count ) {
            if( AsmBuffer[i]->token != T_NUM ) {
                AsmError( 117 );
                return( ERROR );
            } else {
                lastseg.stack_size = AsmBuffer[i]->value;
            }
        } else {
            lastseg.stack_size = 1024;
        }
        break;
    case T_DATA:
    case T_DATA_UN:             // .data?
    case T_CONST:
        if( type == T_DATA ) {
            seg = SIM_DATA;
        } else if( type == T_DATA_UN ) {
            seg = SIM_DATA_UN;
        } else {
            seg = SIM_CONST;
        }

        if (SimCodeBegin[bit][seg] == NULL || SimCodeEnd[seg] == NULL) WASM_PENDING();
        if( string != NULL && type == T_DATA ) {
            strcpy( buffer, string );
            /* we already have a name */
            strcat( buffer, SimCodeBegin[bit][seg] + SIM_DATA_OFFSET  );
        } else {
            strcpy( buffer, SimCodeBegin[bit][seg] );
        }
        InputQueueLine( buffer );
        InputQueueLine( "ASSUME CS:ERROR" );
        input_group( type );
        lastseg.seg = seg;
        if( string != NULL ) {
            strcpy( lastseg.close, string );
            strcat( lastseg.close, SimCodeEnd[seg] + SIM_DATA_OFFSET );
        } else {
            strcpy( lastseg.close, SimCodeEnd[seg] );
        }
        break;
    case T_FARDATA:
    case T_FARDATA_UN:  // .fardata?
        seg = type == T_FARDATA ? SIM_FARDATA : SIM_FARDATA_UN;
        if (SimCodeBegin[bit][seg] == NULL || SimCodeEnd[seg] == NULL) WASM_PENDING();
        if( string != NULL ) {
            strcpy( buffer, string );
            strcat( buffer, SimCodeBegin[bit][seg] + FAROFFSET(seg) );
        } else {
            strcpy( buffer, SimCodeBegin[bit][seg] );
        }
        InputQueueLine( buffer );
        InputQueueLine( "ASSUME CS:ERROR" );
        if( string != NULL ) {
            strcpy( lastseg.close, string );
            strcat( lastseg.close, SimCodeEnd[seg] + FAROFFSET(seg) );
        } else {
            strcpy( lastseg.close, SimCodeEnd[seg] );
        }
        lastseg.seg = seg;
        break;
    default:
        InternalError("..\\c\\direct.c",1884);
        break;
    }
    return( NOT_ERROR );
}

/* CF1143c..114d1; emit the group for a simplified16-bit segment. */
void input_group( int type )
{
    char buffer[256];

    if( ModuleInfo.model == MOD_FLAT ) return;
    strcpy( buffer, "DGROUP GROUP " );
    switch( type ) {
    case T_DATA:
        strcat( buffer, "_DATA" );
        break;
    case T_DATA_UN:
        strcat( buffer, "_BSS" );
        break;
    case T_CONST:
        strcat( buffer, "CONST" );
        break;
    case T_STACK:
        strcat( buffer, "STACK" );
        break;
    }
    InputQueueLine( buffer );
}

/* CF114d4..1150a; the no-open-segment path shares input_group's RET. */
void close_lastseg( void )
{
    if( lastseg.seg != SIM_NONE ) {
        lastseg.seg = SIM_NONE;
        if( CurrSeg == NULL && *lastseg.close != '\0' ) {
            AsmError( 1121 );
            return;
        }
        InputQueueLine( lastseg.close );
    }
}

/* CF1031c..10373; seven-entry switch table at10300..1031c. */
unsigned long GetCurrSegAlign( void )
{
    if( CurrSeg == NULL ) return 0;
    switch( CurrSeg->seg->e.seginfo->segrec->d.segdef.align ) {
    case ALIGN_ABS:
    case ALIGN_BYTE: return 1;
    case ALIGN_WORD: return 2;
    case ALIGN_DWORD: return 4;
    case ALIGN_PARA: return 16;
    case ALIGN_PAGE: return 256;
    case ALIGN_4KPAGE: return 4096;
    default: return 0;
    }
}

/* CF12918..1296e, the procedure/LOCAL type-width query. */
int find_size( int type )
{
    switch( type ) {
    case TOK_EXT_BYTE: case TOK_EXT_SBYTE: return 1;
    case TOK_EXT_WORD: case TOK_EXT_SWORD: return 2;
    case TOK_EXT_DWORD: case TOK_EXT_SDWORD: return 4;
    case TOK_EXT_FWORD: case TOK_EXT_PWORD: return 6;
    case TOK_EXT_QWORD: return 8;
    case TOK_EXT_TBYTE: return 10;
    case TOK_PROC_VARARG: return 0;
    default: return ERROR;
    }
}

/* CF129f0..12ca4. The shared label-list allocation is28B, with only
 * the reached prefix interpreted. */
int LocalDef( int i )
/*******************/
{
    char        *string;
    int         type;
    label_list  *local;
    label_list  *curr;
    proc_info   *info;
    struct asm_sym      *sym;

    if( DefineProc == FALSE ) {
        AsmError( 1064 );
        return( ERROR );
    }

    if( CurrProc == NULL ) InternalError("..\\c\\direct.c",2531);

    info = CurrProc->e.procinfo;

    for( i++; i< Token_Count; i++ ) {
        string = AsmBuffer[i]->string_ptr;

        if( AsmBuffer[i]->token != T_ID ) {
            AsmError( 132 );
            return( ERROR );
        }

        sym = AsmLookup( AsmBuffer[i]->string_ptr );
        if( sym == NULL ) return( ERROR );

        if( sym->state != SYM_UNDEFINED ) {
            AsmError( 71 );
            return( ERROR );
        } else {
            sym->state = SYM_INTERNAL;
        }

        local = AsmAlloc( 28 );
        local->label = AsmAlloc( strlen( AsmBuffer[i]->string_ptr ) + 1 );
        strcpy( local->label, AsmBuffer[i++]->string_ptr );
        local->size = 2;
        local->replace = NULL;
        local->factor = 1;
        local->next = NULL;

        if( i < Token_Count ) {
            if( AsmBuffer[i]->token == T_OP_SQ_BRACKET ) {
                i++;
                if( AsmBuffer[i]->token != T_NUM || i >= Token_Count ) {
                    AsmError( 51 );
                    return( ERROR );
                }
                local->factor = AsmBuffer[i++]->value;
                if( AsmBuffer[i]->token != T_CL_SQ_BRACKET || i >= Token_Count ) {
                    AsmError( 1114 );
                    return( ERROR );
                }
                i++;
            }
        }

        if( i < Token_Count ) {
            if( AsmBuffer[i]->token != T_COLON ) {
                AsmError( 1021 );
                return( ERROR );
            }
            i++;

            type = token_cmp( &(AsmBuffer[i]->string_ptr), TOK_EXT_BYTE,
                                TOK_EXT_TBYTE );
            if( type == ERROR ) {
                AsmError( 1023 );
                return( ERROR );
            }
            local->size = find_size( type );
        }

        info->localsize += ( local->size * local->factor );

        if( info->locallist == NULL ) {
            info->locallist = local;
        } else {
            for( curr = info->locallist;; curr = curr->next ) {
                if( curr->next == NULL ) break;
            }
            curr->next = local;
        }

        /* go past comma */
        i++;
        if( i < Token_Count && AsmBuffer[i]->token != T_COMMA ) {
            AsmError( 33 );
            return( ERROR );
        }

    }

    return( NOT_ERROR );
}


/* CF1299c..129ed, table12970..1299c. Unlike2002, unrecognized
 * widths leave the caller's buffer untouched; only zero clears it. */
void size_override( char *buffer, int size )
{
    switch( size ) {
    case 0: buffer[0] = '\0'; break;
    case 1: strcpy( buffer, " byte ptr " ); break;
    case 2: strcpy( buffer, " word ptr " ); break;
    case 4: strcpy( buffer, " dword ptr " ); break;
    case 6: strcpy( buffer, " fword ptr " ); break;
    case 8: strcpy( buffer, " qword ptr " ); break;
    case 10: strcpy( buffer, " tbyte ptr " ); break;
    }
}

/* CF13b10..13c3b, NUL-separated alias/substitute queue ownership. */
int AddAlias( int i )
/*******************/
{
    /* takes directive of form: <alias_name> alias <substitute_name> */
    queuenode    *qnode;
    char *tmp;

    if( i < 0 ) {
        AsmError( 51 );
        return( ERROR );
    }
    if( AsmBuffer[i]->token != T_ID ||
        AsmBuffer[i+2]->token != T_ID ) {
        AsmError( 1120 );
        return( ERROR );
    }

    /* add this alias to the alias queue */

    /* aliases are stored as:  <len><alias_name><len><substitute_name> */

    qnode = AsmAlloc( sizeof( queuenode ) );
    tmp = AsmAlloc( strlen( AsmBuffer[i]->string_ptr ) +
                    strlen( AsmBuffer[i+2]->string_ptr ) + 2 );
    strcpy( tmp, AsmBuffer[i]->string_ptr );
    qnode->data = (void *)tmp;
    tmp += strlen( tmp ) + 1 ;
    strcpy( tmp, AsmBuffer[i+2]->string_ptr );
    if( AliasQueue == NULL ) {
        AliasQueue = AsmAlloc( sizeof( qdesc ) );
        QInit( AliasQueue );
    }
    QEnqueue( AliasQueue, qnode );
    return( NOT_ERROR );
}


/* CF26700, encountered register-parameter spellings; four columns A,D,B,C. */
char *parm_reg[3][4]= {
    { " AL ", " DL ", " BL ", " CL " },
    { " AX ", " DX ", " BX ", " CX " },
    { " EAX ", " EDX ", " EBX ", " ECX " },
};

enum regsize {
    A_BYTE = 0,
    A_WORD,
    A_DWORD,
};



/* CFf788..f92c, tablef760..f788. */
int get_watcom_argument_string( char *buffer, unsigned char size, unsigned char *parm_number )
/******************************************************************************/
/* get the register for parms 0 to 3,
 * using the watcom register parm passing conventions ( A D B C ) */
{
    int parm = *parm_number;

    if( parm > 3 ) return( FALSE );
    switch( size ) {
    case 1:
        sprintf( buffer, parm_reg[A_BYTE][parm] );
        break;
    case 2:
        sprintf( buffer, parm_reg[A_WORD][parm] );
        break;
    case 4:
        if( Use32 ) {
            sprintf( buffer, parm_reg[A_DWORD][parm] );
            break;
        } else {
            switch( parm ) {
            case 0:
                sprintf( buffer, " [DX AX]" );
                buffer[0] = 0;
                *parm_number = 1; // take up 0 and 1
                break;
            case 1:
            case 2:
                sprintf( buffer, " [CX BX]" );
                buffer[0] = 0;
                *parm_number = 3; // take up 2 and 3
                break;
            default:
                // passed on stack ... it's ok
                return( FALSE );
            }
            return( TRUE );
        }
    case 10:
        AsmErr( 1129 );
        return( ERROR );
    case 6:
        if( Use32 ) {
            switch( parm ) {
            case 0:
                sprintf( buffer, " [DX EAX]" );
                buffer[0]=0;
                *parm_number = 1; // take up 0 and 1
                break;
            case 1:
            case 2:
                sprintf( buffer, " [CX EBX]" );
                buffer[0]=0;
                *parm_number = 3; // take up 2 and 3
                break;
            default:
                // passed on stack ... it's ok
                return( FALSE );
            }
            return( TRUE );
        }
        // fall through for 16 bit to default
    case 8:
        if( Use32 ) {
            switch( parm ) {
            case 0:
                sprintf( buffer, " [EDX EAX]" );
                buffer[0]=0;
                *parm_number = 1; // take up 0 and 1
                break;
            case 1:
            case 2:
                sprintf( buffer, " [ECX EBX]" );
                buffer[0]=0;
                *parm_number = 3; // take up 2 and 3
                break;
            default:
                // passed on stack ... it's ok
                return( FALSE );
            }
            return( TRUE );
        }
        // fall through for 16 bit to default
    default:
        // something wierd
        AsmError( 1130 );
        return( ERROR );
    }
    return( TRUE );
}

/* CF125f4..126d9. The1994 flat test is conditional on a NULL assume
 * pointer and compares the bit field with TRUE; preserve that ordering. */
unsigned GetPrefixAssume( struct asm_sym* sym, unsigned prefix )
/******************************************************/
{
    int         type;
    dir_node    *dir;
    struct asm_sym      *assume;

    if( Parse_Pass == PASS_1 ) return( prefix );

    assume = AssumeTable[prefix].symbol;
    if( assume == NULL && AssumeTable[prefix].flat == TRUE ) {
        Frame = F_GRP;
        Frame_Datum = ModuleInfo.flat_idx;
        return( prefix );
    }
    if( assume == NULL ) {
        if( sym->state == SYM_EXTERNAL ) {

            return( prefix );
        } else {
            return( ASSUME_NOTHING );
        }
    }

    dir = (dir_node *)assume;
    if( assume->state == SYM_SEG ) {
        type = TAB_SEG;
    } else if( assume->state == SYM_GRP ) {
        type = TAB_GRP;
    }

    if( get_grp( sym ) == dir ||    // dir is the group for this symbol
        GetSeg( sym ) == dir ||    // dir is the seg. for this symbol
        sym->state == SYM_EXTERNAL ) {

        if( type == TAB_GRP ) {
            Frame = F_GRP;
            if( sym->state == SYM_EXTERNAL ) {
                Frame_Datum = dir->e.grpinfo->idx;
            } else {
                Frame_Datum = sym->grpidx;
            }
        } else {
            Frame = F_SEG;
            if( sym->state == SYM_EXTERNAL ) {
                Frame_Datum = dir->e.seginfo->segrec->d.segdef.idx;
            } else {
                Frame_Datum = sym->segidx;
            }
        }
        return( prefix );
    } else {
        return( ASSUME_NOTHING );
    }
}


/* CF124cc..12599, explicit segment/group frame override. */
int FixOverride( int index )
/**************************/
/* Fix segment or group override */
{
    struct asm_sym      *sym1;
    struct asm_sym      *sym2;
    dir_node            *tmp;

    sym1 = AsmLookup( AsmBuffer[index-2]->string_ptr );
    if (sym1 == NULL) InternalError("..\\c\\direct.c", 2245);

    tmp = GetSeg( sym1 );
    if( tmp != NULL ) {
        sym2 = AsmLookup( AsmBuffer[index]->string_ptr );
        if (sym2 == NULL) InternalError("..\\c\\direct.c", 2250);
        Frame = F_SEG;
        Frame_Datum = sym1->segidx;
        return( NOT_ERROR );
    }

    tmp = get_grp( sym1 );
    if( tmp != NULL ) {
        sym2 = AsmLookup( AsmBuffer[index]->string_ptr );
        if (sym2 == NULL) InternalError("..\\c\\direct.c", 2259);
        Frame = F_GRP;
        Frame_Datum = sym1->grpidx;
        return( NOT_ERROR );
    }
    AsmError( 51 );
    return( ERROR );
}
