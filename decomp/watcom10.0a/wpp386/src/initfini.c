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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/

/* INITFINI/EXIT_POINT names from bld/fe_misc/{c/initfini.c,h/initfini.h}
 * at initial commit6b9cb4438959d845a2b4dd4bf19f7908ecd608e9.
 * Its initfini.c is incomplete; actual1994 control flow/layout is authoritative.
 * This version has no subsequent bit or acquisition callback. */
#include "compiler.h"
#include "initstate.h"
#include <recon.h>

extern void cmemInit(INITFINI *);
extern void cmemFini(INITFINI *);
extern void cppNamesInit(INITFINI *);
extern void cppNamesFini(INITFINI *);
extern void hfileInit(INITFINI *);
extern void hfileFini(INITFINI *);
extern void fnameInit(INITFINI *);
extern void fnameFini(INITFINI *);
extern void ioSuppInit(INITFINI *);
extern void ioSuppFini(INITFINI *);
extern void errFileInit(INITFINI *);
extern void errFileFini(INITFINI *);

static INITFINI program_registration_0 = { cmemInit, cmemFini };
static INITFINI program_registration_1 = { cppNamesInit, cppNamesFini };
static INITFINI program_registration_2 = { hfileInit, hfileFini };
static INITFINI program_registration_3 = { fnameInit, fnameFini };
static INITFINI program_registration_4 = { ioSuppInit, ioSuppFini };
static INITFINI program_registration_5 = { errFileInit, errFileFini };
EXIT_POINT ExitPoint_program = {
    0, { &program_registration_0, &program_registration_1, &program_registration_2, &program_registration_3, &program_registration_4, &program_registration_5, 0 }
};

EXIT_POINT *exit_stack;

void ExitPointAcquireRtn(EXIT_POINT *est)
{
    INITFINI **iptr;
    INITFINI *entry;

    est->previous = exit_stack;
    exit_stack = est;
    for (iptr = est->registered; *iptr; ++iptr) {
        entry = *iptr;
        entry->init_rtn(entry);
    }
}

/* Actual1994 doCCompile operands: semantic callback records. Unreached
 * callbacks with provisional names remain registered traps, not empty bodies.
 * Split records follow released fe_misc/initfini.c with no subsequent flag. */
typedef struct split_fini SPLIT_FINI;
struct split_fini {
    SPLIT_FINI *next;
    INITFINI *orig;
    EXIT_POINT *init, *fini;
};
typedef struct split_init {
    INITFINI defn;
    SPLIT_FINI fini;
} SPLIT_INIT_RECORD;
static INITFINI statistics_registration = { statsInit, statsPrint };
static INITFINI macros_registration = { macroInit, macroFini };
static SPLIT_INIT_RECORD split_statistics = {
    { splitInit, InitFiniStub },
    { 0, &statistics_registration, 0, &ExitPoint_cpp_analysis }
};
static SPLIT_INIT_RECORD split_macros = {
    { splitInit, InitFiniStub },
    { 0, &macros_registration, 0, &ExitPoint_cpp_analysis }
};
static INITFINI front_registration_0 = { cgioInit, cgioFini };
static INITFINI front_registration_1 = { rtnGenInit, rtnGenFini };
static INITFINI front_registration_2 = { srcFileInit, srcFileFini };
static INITFINI front_registration_3 = { typesInit, typesFini };
static INITFINI front_registration_4 = { typeCompareInit, typeCompareFini };
static INITFINI preproc_registration_0 = { preprocInit0, preprocFini0 };
static INITFINI preproc_registration_1 = { preprocInit1, preprocFini1 };
static INITFINI preproc_registration_2 = { preprocInit2, InitFiniStub };
static INITFINI analysis_registration_0 = { analysisInit0, analysisFini0 };
static INITFINI analysis_registration_1 = { analysisInit1, analysisFini1 };
static INITFINI analysis_registration_2 = { analysisInit2, analysisFini2 };
static INITFINI object_registration_0 = { objectInit0, objectFini0 };
static INITFINI object_registration_1 = { objectInit1, objectFini1 };
static INITFINI object_registration_2 = { objectInit2, objectFini2 };
static INITFINI object_registration_3 = { objectInit3, objectFini3 };
static INITFINI object_registration_4 = { objectInit4, objectFini4 };
static INITFINI object_registration_5 = { objectInit5, objectFini5 };
static INITFINI object_registration_6 = { objectInit6, objectFini6 };
static INITFINI object_registration_7 = { objectInit7, objectFini7 };
static INITFINI object_registration_8 = { objectInit8, objectFini8 };
static INITFINI object_registration_9 = { objectInit9, objectFini9 };
static INITFINI object_registration_10 = { objectInit10, objectFini10 };
static INITFINI object_registration_11 = { objectInit11, objectFini11 };
static INITFINI object_registration_12 = { objectInit12, InitFiniStub };
static INITFINI object_registration_13 = { objectInit13, objectFini13 };
static INITFINI object_registration_14 = { objectInit14, objectFini14 };
static INITFINI object_registration_15 = { objectInit15, objectFini15 };
static INITFINI object_registration_16 = { objectInit16, objectFini16 };
static INITFINI object_registration_17 = { objectInit17, objectFini17 };
EXIT_POINT ExitPoint_cpp_front_end = { 0, { &front_registration_0, &split_statistics.defn, &split_macros.defn, &front_registration_1, &front_registration_2, &front_registration_3, &front_registration_4, 0 } };
EXIT_POINT ExitPoint_cpp_preproc = { 0, { &preproc_registration_0, &preproc_registration_1, &preproc_registration_2, 0 } };
EXIT_POINT ExitPoint_cpp_preproc_only = { 0, { 0 } };
EXIT_POINT ExitPoint_cpp_analysis = { 0, { &analysis_registration_0, &analysis_registration_1, &analysis_registration_2, 0 } };
EXIT_POINT ExitPoint_cpp_object = { 0, { &object_registration_0, &object_registration_1, &object_registration_2, &object_registration_3, &object_registration_4, &object_registration_5, &object_registration_6, &object_registration_7, &object_registration_8, &object_registration_9, &object_registration_10, &object_registration_11, &object_registration_12, &object_registration_13, &object_registration_14, &object_registration_15, &object_registration_16, &object_registration_17, 0 } };

static SPLIT_FINI *split_hdr;

void splitInit(INITFINI *defn)
{
    SPLIT_INIT_RECORD *init;
    SPLIT_FINI *fptr;

    init = (SPLIT_INIT_RECORD *)defn;
    fptr = &init->fini;
    fptr->orig->init_rtn(fptr->orig);
    fptr->init = exit_stack;
    RingInsert((void **)&split_hdr, fptr, 0);
}
