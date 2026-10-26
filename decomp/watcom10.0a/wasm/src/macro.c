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

/* Adapted bld/wasm/c/macro.c at6b9cb443, actual CF17408..17938.
 *1994 appends T_STRING parameter text directly; the2002 quoting/escaping
 * branch is absent. All other branches follow the actual old body. */
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include "asminit.h"
#include "direct.h"
#include <recon.h>
#ifdef __WATCOMC__
#pragma intrinsic(strlen,strcpy,strcat)
#endif
int ExpandMacro( int tok_count)
/*****************************/
{
    char        buffer[256];
    char        next_char[2];
    dir_node    *dir;
    asm_sym     *sym = NULL;
    macro_info  *info;
    parm_list   *parm;
    asmlines    *lnode;
    char        *line;
    int         count = 0;
    int         macro_name_loc;
    char        expansion_flag = 0;
    int         exp_start;

    if( AsmBuffer[count]->token == T_FINAL ) return( tok_count );

    next_char[1]='\0';

    /* first, find out if it is a macro */
    for( ; count < tok_count; count++ ) {
        if( AsmBuffer[count]->token == T_ID ) {
            sym = AsmGetSymbol( AsmBuffer[count]->string_ptr );
        }
        if( sym != NULL && sym->state == SYM_MACRO ) break;
    }
    if( sym == NULL || sym->state != SYM_MACRO ) {
        return( tok_count );
    }
    macro_name_loc = count;
    if( AsmBuffer[count+1]->token == T_DIRECTIVE &&
        AsmBuffer[count+1]->value == T_MACRO ) {
        /* this is a macro DEFINITION! */
        return( tok_count );
    }

    if( macro_name_loc != 0 ) {
        /* save the rest of the line from before the macro */
        PushLineQueue();
        buffer[0]='\0';
        for( count=0; count < macro_name_loc; count++ ) {
            strcat( buffer, AsmBuffer[count]->string_ptr );
        }
        InputQueueLine( buffer );
    }
    count = macro_name_loc + 1;

    /* now get all the parameters from asmbuffer
     * they should alternate: parm /  comma
     */

    dir = (dir_node *)sym;
    info = dir->e.macroinfo;

    for( parm = info->parmlist; parm != NULL; parm = parm->next ) {
        buffer[0]='\0';
        if( count < tok_count ) {
            if( AsmBuffer[count]->token == T_COMMA ||
                ( AsmBuffer[count]->token == T_STRING &&
                  strlen( AsmBuffer[count]->string_ptr ) == 0 ) ) {
                /* blank parm */

                if( parm->required ) {
                    AsmError( 1113 );
                    return( -1 );
                }
                if( parm->def ) {
                    /* fill in the default value */
                    parm->replace = AsmAlloc( strlen( parm->def )+1 );
                    strcpy( parm->replace, parm->def );
                }
                if( AsmBuffer[count]->token != T_COMMA ) {
                    count++;
                    if( count < tok_count &&
                        AsmBuffer[count]->token != T_COMMA ) {
                        AsmError( 33 );
                        return( -1 );
                    }
                }
                /* go past comma */
                count++;
            } else {
                /* we have a parm! :) */
                for( ; ; ) {
                    if( AsmBuffer[count]->token == T_FINAL ) break;
                    if( *(AsmBuffer[count]->string_ptr) == '%' ) {
                        *(AsmBuffer[count]->string_ptr) = ' ';
                        expansion_flag = 1;
                        exp_start = count;
                    }

                    if( !expansion_flag ) {
                        if( AsmBuffer[count]->token == T_COMMA ||
                            AsmBuffer[count]->string_ptr == NULL ||
                            count == tok_count ) {
                            break;
                        }
                        if( ( AsmBuffer[count]->token > T_ID_IN_BACKQUOTES ) ) {
                            next_char[0] = *( AsmBuffer[count]->string_ptr );
                            strcat( buffer, next_char );
                        } else if( AsmBuffer[count]->token == T_NUM ) {
                            itoa( AsmBuffer[count]->value, buffer+strlen( buffer ), 10 );
                        } else {
                            strcat( buffer, AsmBuffer[count]->string_ptr );
                        }
                    } else {
                        switch( ExpandSymbol( count, 0 ) ) {
                        case -1:
                            free_parmlist( info->parmlist );
                            return( -1 );
                        case 2:
                            continue;
                        }

                        if( AsmBuffer[count]->token == T_COMMA ||
                            AsmBuffer[count]->string_ptr == NULL ||
                            AsmBuffer[count+1]->token == T_FINAL ) {
                            if( AsmBuffer[count+1]->token == T_FINAL ) count++;
                            tok_count = EvalExpr( tok_count, exp_start, count-1 );
                            expansion_flag = 0;
                            Token_Count = tok_count;
                            count = exp_start;
                        }
                    }
                    count++;
                }
                count++; /* go past the comma */
                parm->replace = AsmAlloc( strlen( buffer )+1 );
                strcpy( parm->replace, buffer );
            }
        } else {
            if( parm->required ) {
                AsmError( 1113 );
                return( -1 );
            }
        }
    }

    /* now actually fill in the parms */
    PushLineQueue();
    for( lnode = info->data; lnode != NULL; lnode = lnode->next ) {
        line = fill_in_parms( lnode, info->parmlist );
        if( lineis( line, "local" ) ) {
            AsmScan( line, StringBuf );
            if( macro_local() == -1 ) return( -1 );
            AsmFree( line );
            continue;
        }
        InputQueueLine( line );
        AsmFree( line );
    }

    /* put this macro into the file stack */
    PushMacro( sym->name, info->hidden );

    /* now free the parm replace strings */
    free_parmlist( info->parmlist );

//    tok_count = macro_name_loc - 1;
    return( 0 );
}

/* CF169ac..16b77; substitute the original #dd parameter marker.
 * Before/count/length are the original unsigned bytes. */
char *replace_parm(parm_list *parms, char *start, unsigned char len, asmlines *line)
{
    char buffer[10];
    parm_list *parm;
    char *new_line, *old_line;
    unsigned char before, count = 0;
    old_line = line->line;
    for (parm = parms; parm != NULL; parm = parm->next) {
        if (parm->label != NULL && strlen(parm->label) == len &&
            strncmp(start, parm->label, len) == 0) {
            new_line = AsmAlloc(strlen(old_line) - len + 4);
            before = start - old_line;
            if (*(start - 1) == '&') --before;
            strncpy(new_line, old_line, before);
            new_line[before] = '\0';
            strcat(new_line, "#");
            if (sprintf(buffer, "%2d", count) != 2)
                InternalError("..\\c\\macro.c", 144);
            if (buffer[0] == ' ') buffer[0] = '0';
            strcat(new_line, buffer);
            if (*(start + len) == '&') ++len;
            strcat(new_line, start + len);
            line->line = new_line;
            ++line->parmcount;
            AsmFree(old_line);
            return new_line + 3;
        }
        ++count;
    }
    return start + len;
}

/* CF16b78..16d06. Macro substitution scans the original identifier domain. */
#define is_valid_id_char(ch) \
    (isalpha((unsigned char)(ch)) || isdigit((unsigned char)(ch)) || \
     (ch) == '_' || (ch) == '@' || (ch) == '$' || (ch) == '?')
void put_parm_placeholders_in_line( asmlines *linestruct, parm_list *parms )
/*********************************************************************************/
{
    char *line;
    char *tmp;
    char *start;
    char quote = 0;
    unsigned char len;

    /* handle the substitution operator ( & ) */
    line = linestruct->line;
    for( tmp = line; *tmp != '\0'; ) {
        /* scan across the string for space, &, " - to start a word */
        for( ; *tmp != '\0'; tmp++ ) {
            if( is_valid_id_char( *tmp ) ) {
                if( tmp == line ) break; /* ok to start at beginning of line */
                continue;
            } else if( isspace( (unsigned char)*tmp ) ) {
                /* find 1st non blank char */
                while( isspace( (unsigned char)*tmp ) ) tmp++;
                break;
            } else if( *tmp == '"' ) {
                /* toggle the quote flag */
                quote = ( quote + 1 ) %2;
                tmp++;
                break;
            } else {
                /* some other garbage */
                tmp++;
                break;
            }
        }
        start = tmp;
        /* scan across the string for space, &, " - to end the word */
        for( ; *tmp != '\0'; tmp++ ) {
            if( is_valid_id_char( *tmp ) ) {
                continue;
            } else if( isspace( (unsigned char)*tmp ) ) {
                break;
            } else if( *tmp == '"' ) {
                /* toggle the quote flag */
                quote = ( quote + 1 ) %2;
                break;
            } else {
                break;
            }
        }
        len = tmp - start;
        /* look for this word in the macro parms, and replace it if it is */
        /* this would change line - it will have to be reallocated */
        if( !quote || *start =='&' || *(start-1)=='&' || *(start+len+1)=='&' ) {
            if( *start != '\0' && len > 0 ) {
                tmp = replace_parm( parms, start, len, linestruct );
            }
        }
    }
}

#undef is_valid_id_char

/* CF16940..169ac; append a nine-byte owned macro-line node. */
asmlines *asmline_insert(asmlines **head, void *data)
{
    asmlines **tail, *entry;
    for (tail = head; *tail != NULL; tail = &(*tail)->next) ;
    entry = AsmAlloc(sizeof(*entry));
    entry->next = NULL;
    entry->line = AsmAlloc(strlen(data) + 1);
    entry->parmcount = 0;
    strcpy(entry->line, data);
    *tail = entry;
    return entry;
}

/* CF16d08..16d5d; case-insensitive keyword with a whitespace/end boundary.
 * wipe_space mutates the caller's line before every comparison. */
signed char lineis(char *line, char *prefix)
{
    int length = strlen(prefix);
    wipe_space(line);
    if (strnicmp(line, prefix, length)) return 0;
    if (line[length] != '\0' && !isspace((unsigned char)line[length])) return 0;
    return 1;
}

/* CF16e9c..171ef; parameter list and nested macro body capture. */
int macro_exam( int i )
/****************************/
{
    macro_info          *info;
    char                *string;
    char                *token;
    char                *name;
    parm_list           *paranode;
    parm_list           *paracurr;
    asmlines            *linestruct;
    char                buffer[ 256 ];
    dir_node            *dir;
    unsigned            nesting_depth = 0;
    unsigned char       store_data;

    name = AsmBuffer[i++]->string_ptr;
    dir = (dir_node *)AsmGetSymbol( name );
    info = dir->e.macroinfo;

    store_data = Parse_Pass == PASS_1 || info->data == NULL;

    /* go past "MACRO" */
    i++;

    if( store_data ) {
        for( ; i < Token_Count ; ) {
            token = AsmBuffer[i]->string_ptr;
            paranode = AsmAlloc( sizeof( parm_list ) );
            paranode->def = NULL;
            paranode->replace = NULL;
            paranode->required = 0;

            /* first get the parm. name */
            paranode->label = AsmAlloc( strlen( token ) + 1 );
            strcpy( paranode->label, token );
            i++;

            /* now see if it has a default value or is required */
            if( AsmBuffer[i]->token == T_COLON ) {
                i++;
                if( *AsmBuffer[i]->string_ptr == '=' ) {
                    i++;
                    if( AsmBuffer[i]->token != T_STRING ) {
                        AsmError( 51 );
                        return( -1 );
                    }
                    token = AsmBuffer[i]->string_ptr;
                    paranode->def = AsmAlloc( strlen( token ) + 1 );
                    strcpy( paranode->def, token );
                    i++;
                } else if( strcmp( AsmBuffer[i]->string_ptr, "REQ" ) == 0 ) {
                    /* required parameter */
                    paranode->required = 1;
                    i++;
                }
            }
            if( i< Token_Count && AsmBuffer[i]->token != T_COMMA ) {
                AsmError( 33 );
                return( -1 );
            }
            /* go past comma */
            i++;

            /* add this parm node to the list */
            // fixme
            paranode->next = NULL;
            if( info->parmlist == NULL ) {
                info->parmlist = paranode;
            } else {
                for( paracurr = info->parmlist;; paracurr = paracurr->next ) {
                    if( paracurr->next == NULL ) break;
                }
                paracurr->next = paranode;
            }

        } /* looping through parameters */
    }

    /* now read in all the contents of the macro, and store them */
    for( ; ; ) {
        char *ptr;

        string = ReadPreparedLine( buffer );
        if( string == NULL ) {
            AsmError( 135 );
            return( -1 );
        } else if( lineis( string, "endm" ) ) {
            if( nesting_depth ) {
                nesting_depth--;
            } else {
                return( 1 );
            }
        }
        ptr = string;
        while( isspace( (unsigned char)*ptr ) ) ptr++;
        if( lineis( ptr, "for" )
         || lineis( ptr, "forc" )
         || lineis( ptr, "irp" ) ) {
            nesting_depth++;
        }
        /* The shipped loop tests ptr, not *ptr, before scanning a token.
         * Preserve that older behavior; ordinary lines have trailing space. */
        while( ptr != NULL && !isspace( (unsigned char)*ptr ) ) ptr++; // skip 1st token
        while( isspace( (unsigned char)*ptr ) ) ptr++;
        if( lineis( ptr, "macro" )
         || lineis( ptr, "for" )
         || lineis( ptr, "forc" )
         || lineis( ptr, "irp" ) ) {
            nesting_depth++;
        }

        if( store_data ) {
            linestruct = asmline_insert( &(info->data), string );
            /* make info->data point at the LAST line in the struct */
            put_parm_placeholders_in_line( linestruct, info->parmlist );
        }
    }
}

/* CF17938..17a2d; named macro owner and its definition-file identity. */
int MacroDef(int i, unsigned char hidden)
{
    asm_sym *sym;
    char *name;
    dir_node *dir;
    if (Parse_Pass != PASS_1) {
        if (i < 0) InternalError("..\\c\\macro.c", 637);
        dir = (dir_node *)AsmGetSymbol(AsmBuffer[i]->string_ptr);
        if (dir != NULL) return macro_exam(i);
    }
    if (i < 0) { AsmError(1054); return -1; }
    name = AsmBuffer[i]->string_ptr;
    sym = AsmGetSymbol(name);
    if (sym != NULL) { AsmError(1055); return -1; }
    dir = dir_insert(name, TAB_MACRO);
    name = get_curr_filename();
    dir->e.macroinfo->filename = AsmAlloc(strlen(name) + 1);
    strcpy(dir->e.macroinfo->filename, name);
    dir->e.macroinfo->hidden = hidden;
    sym = &dir->sym;
    sym->state = SYM_MACRO;
    return macro_exam(i);
}

/* CF17320..173e8. Actual1994 uses checked stack alloca, not the later
 * AsmTmpAlloc pool, and counts parameters in an unsigned byte. */
char *fill_in_parms(asmlines *line, parm_list *parameters)
{
    char buffer[256];
    parm_list *parm;
    char *new_line;
    char **parm_array;
    unsigned char count = 0;
    for (parm = parameters; parm != NULL; parm = parm->next) ++count;
    parm_array = alloca(count * sizeof(char *));
    count = 0;
    for (parm = parameters; parm != NULL; parm = parm->next) {
        parm_array[count] = parm->replace;
        ++count;
    }
    my_sprintf(buffer,line->line,count - 1,parm_array);
    new_line = AsmAlloc(strlen(buffer) + 1);
    strcpy(new_line,buffer);
    return new_line;
}

/* CF171f0..17320, expand the original #dd placeholder format. */
int my_sprintf(char *dest, char *format, int argc, char *argv[])
{
    char buffer[3];
    char *start, *end;
    unsigned char parmno = 0;
    *dest = '\0';
    start = format;
    for (end = start; *end != '\0'; start = end + 3) {
        for (end = start;
             !(*end == '#' && isdigit(end[1]) && isdigit(end[2])) &&
             *end != '\0'; ++end) ;
        if (*end == '\0') {
            strncat(dest,start,end - start);
            return strlen(dest);
        }
        buffer[0] = end[1];
        buffer[1] = end[2];
        buffer[2] = '\0';
        parmno = atoi(buffer);
        strncat(dest,start,end - start);
        if (parmno > argc) InternalError("..\\c\\macro.c",420);
        if (argv[parmno] != NULL) strcat(dest,argv[parmno++]);
    }
    return strlen(dest);
}

/* CF173e8..17406, release each replacement without freeing the formal list. */
void free_parmlist(parm_list *head)
{
    parm_list *parm;
    for (parm = head; parm != NULL; parm = parm->next) {
        AsmFree(parm->replace);
        parm->replace = NULL;
    }
}
