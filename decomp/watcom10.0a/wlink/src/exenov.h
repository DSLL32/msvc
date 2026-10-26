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

#ifndef WLINK_EXENOV_H
#define WLINK_EXENOV_H
#pragma pack(1)
#define MAX_DESCRIPTION_LENGTH 127
#define MAX_SCREEN_NAME_LENGTH 71
#define MAX_THREAD_NAME_LENGTH 17
#define OLD_THREAD_NAME_LENGTH  5
#define NLM_SIGNATURE "NetWare Loadable Module\x01a"
#define NLM_VERSION 4
#define VERSION_SIGNATURE "VeRsIoN#"
#define VERSION_SIGNATURE_LENGTH 8
#define COPYRIGHT_SIGNATURE "CoPyRiGhT="
#define COPYRIGHT_SIGNATURE_LENGTH 10
#define MAX_COPYRIGHT_LENGTH 255
#define EXTENDED_NLM_SIGNATURE "MeSsAgEs"
#define EXTENDED_NLM_SIGNATURE_LENGTH (sizeof(EXTENDED_NLM_SIGNATURE)-1)
#define MSG_FILE_SIGNATURE "NetWare Message File: "
#define MSG_FILE_SIGNATURE_LENGTH 22
#define LANGUAGE_ID_OFFSET 106
#define NUM_MESSAGES_OFFSET 110

/* the fields marked vbl are variable length. */

/* the first "fixed chunk" of the header */

typedef struct nlm_header {
    char            signature[ sizeof( NLM_SIGNATURE ) - 1 ];
    unsigned long     version;
    char            moduleName[14];     /* 1st character is a length byte.*/
    unsigned long     codeImageOffset;
    unsigned long     codeImageSize;
    unsigned long     dataImageOffset;
    unsigned long     dataImageSize;
    unsigned long     uninitializedDataSize;
    unsigned long     customDataOffset;
    unsigned long     customDataSize;
    unsigned long     moduleDependencyOffset;
    unsigned long     numberOfModuleDependencies;
    unsigned long     relocationFixupOffset;
    unsigned long     numberOfRelocationFixups;
    unsigned long     externalReferencesOffset;
    unsigned long     numberOfExternalReferences;
    unsigned long     publicsOffset;
    unsigned long     numberOfPublics;
    unsigned long     debugInfoOffset;
    unsigned long     numberOfDebugRecords;
    unsigned long     codeStartOffset;
    unsigned long     exitProcedureOffset;
    unsigned long     checkUnloadProcedureOffset;
    unsigned long     moduleType;
    unsigned long     flags;
    unsigned char      descriptionLength;
} nlm_header;

/* The second fixed chunk follows the separately emitted variable strings. */

typedef struct nlm_header_3 {
    char            versionSignature[ VERSION_SIGNATURE_LENGTH ];
    unsigned long     majorVersion;
    unsigned long     minorVersion;
    unsigned long     revision;
    unsigned long     year;
    unsigned long     month;      /* starts at 1 */
    unsigned long     day;
} nlm_header_3;

/* remaining header fields */

typedef struct fixed_hdr_3 {
    char copyrightSignature[COPYRIGHT_SIGNATURE_LENGTH];
} fixed_hdr_3;

typedef struct {
    unsigned char  stamp[ EXTENDED_NLM_SIGNATURE_LENGTH ];
    unsigned long languageID;
    unsigned long messageFileOffset;
    unsigned long messageFileLength;
    unsigned long messageCount;
    unsigned long helpFileOffset;
    unsigned long helpFileLength;
    unsigned long RPCDataOffset;
    unsigned long RPCDataLength;
    unsigned long sharedCodeOffset;
    unsigned long sharedCodeLength;
    unsigned long sharedDataOffset;
    unsigned long sharedDataLength;
    unsigned long sharedRelocationFixupOffset;
    unsigned long sharedRelocationFixupCount;
    unsigned long sharedExternalReferenceOffset;
    unsigned long sharedExternalReferenceCount;
    unsigned long sharedPublicsOffset;
    unsigned long sharedPublicsCount;
    unsigned long sharedDebugRecordOffset;
    unsigned long sharedDebugRecordCount;
    unsigned long sharedInitializationOffset;
    unsigned long sharedExitProcedureOffset;
    unsigned long productID;
    unsigned long reserved0;
    unsigned long reserved1;
    unsigned long reserved2;
    unsigned long reserved3;
    unsigned long reserved4;
    unsigned long reserved5;
} extended_nlm_header;


typedef nlm_header fixed_header;
typedef nlm_header_3 fixed_hdr_2;
typedef char nov_copyright_header_is_10[(sizeof(fixed_hdr_3) == 10) ? 1 : -1];
typedef char nov_header_is_131[(sizeof(fixed_header) == 131) ? 1 : -1];
typedef char nov_extension_is_124[(sizeof(extended_nlm_header) == 124) ? 1 : -1];
extern unsigned long WriteNovData( unsigned long, fixed_header * );
extern unsigned long WriteNovImage( unsigned long, unsigned char );
extern unsigned long WriteNovRelocs( fixed_header * );
extern unsigned long WriteNovImports( fixed_header * );
extern unsigned long WriteNovExports( fixed_header * );
extern unsigned long WriteNovModules( fixed_header * );
extern unsigned long WriteNovDBI( fixed_header * );
extern unsigned long AppendToLoadFile( char * );
extern unsigned long WriteMessages( extended_nlm_header * );
extern unsigned long WriteSharedNLM( extended_nlm_header *, unsigned long );
extern void GetProcOffsets( fixed_header * );
extern void NovNameWrite( char * );

#pragma pack()
#include <recon.h>
#endif
