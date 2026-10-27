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


/* Adapted from initial6b9cb443 bld/plusplus/c/yydriver.c.
 * Actual1994 CF6243d..6249a has three stacks, no later RESTART_PARSE
 * pool or reporting counters; its impossible token is3. */
#include <stddef.h>
#include <string.h>
#include "compiler.h"
#include "carve.h"
#include "frontend.h"
#include "scan.h"
#include "macro.h"
#include "scope.h"
#include "symbol.h"
#include "vstk.h"
#include "cgio.h"
#include "pending.h"
#include "ptree.h"
#include "yydriver-tables.h"
#include "gstack.h"
#include "template.h"
#include "rewrite.h"
#include "stringl.h"
#include <recon.h>
DECL_INFO *MakeNewTypeId(PTREE type_id);
DECL_INFO *AddArrayDeclarator(DECL_INFO *info,PTREE size);
PTREE PTreeNonZeroConstantExpr(PTREE expr);
PTREE DoDeclSpec(DECL_SPEC *spec);
DECL_SPEC *PTypeLinkage(void);
void LinkagePush(char *name);
REWRITE *RewritePackageMemInit(PTREE multi);
int DeclNoInit(DECL_INFO *info);
PTREE MakeConstructorId(DECL_SPEC *spec);
void VerifyMemberFunction(DECL_SPEC *spec,DECL_INFO *info);
void VerifyPureFunction(DECL_INFO *info,PTREE init);
DECL_INFO *InsertDeclInfo(SCOPE scope,DECL_INFO *info);
DECL_SPEC *PTypeStgClass(unsigned char);
DECL_SPEC *PTypeScalar(unsigned short);
DECL_SPEC *PTypeSpecifier(unsigned char);
TYPE MakePointerType(unsigned short ref_flag,unsigned char cv_flags);
static carve_t carveVALUE_STACK, carveSTATE_STACK, carveLOCATION_STACK;
static unsigned currToken;
static SUICIDE_CALLBACK parserSuicide;
void parserSuicideHandler(void);
void parseInit(INITFINI *defn)
{
    (void)defn;
    carveVALUE_STACK = CarveCreate(256 * 4, 2);
    carveSTATE_STACK = CarveCreate(256 * 2, 2);
    carveLOCATION_STACK = CarveCreate(256 * 12, 2);
    currToken = 3;
    parserSuicide.call_back = parserSuicideHandler;
    RegisterSuicideCallback(&parserSuicide);
}

/* The1994 stack has no later restart-parse pointer: vsp is at12,
 * gstack32 and look-ahead storage48; sizeof(PARSE_STACK)==92. */
typedef union yyvalue {
    unsigned flags,token;
    struct type *type;
    struct decl_spec *dspec;
    struct decl_info *dinfo;
    PTREE tree;
    REWRITE *rewrite;
    void *pointer;
} YYSTYPE;
typedef struct parse_stack PARSE_STACK;
#pragma pack(4)
struct parse_stack {
    PARSE_STACK *next;
    CGIO_LOCATION *lsp,*lstack;
    YYSTYPE *vsp,*vstack;
    unsigned short *ssp,*sstack,*exhaust;
    GLOBAL_STACK *gstack;
    void *qualifications;
    SCOPE reset_scope;
    struct type *class_colon;
    VSTK_CTL look_ahead_storage;
    unsigned look_ahead_count,look_ahead_index;
    CGIO_LOCATION template_record_locn;
    void *template_record_tokens;
    unsigned char no_super_tokens:1,use_saved_tokens:1,favour_reduce:1,
        favour_shift:1,look_ahead_stack:1,look_ahead_active:1,
        template_decl:1,template_args:1;
    unsigned char template_class_inst_defer:1,special_colon_colon:1,
        reserved_flags:6;
};
#pragma pack()
typedef char ParseStackSize[(sizeof(PARSE_STACK)==92 && offsetof(PARSE_STACK,vsp)==12)?1:-1];
void setWatchColonColon(PARSE_STACK *stack,DECL_SPEC *spec)
{
    stack->class_colon=spec->partial;
    stack->special_colon_colon=1;
}
REWRITE *ParseGetRecordingInProgress(CGIO_LOCATION **location);
static PARSE_STACK *currParseStack;
static CGIO_LOCATION yylocation;
void SrcFileGetTokenLocn(CGIO_LOCATION *location);
void *StackPush(void **head,void *item);
void syncLocation(void)
{
    if (currToken==3) SrcFileGetTokenLocn(&yylocation);
}
void initParseStacks(PARSE_STACK *stack,int look_ahead)
{
    StackPush((void **)&currParseStack,stack);
    if (look_ahead) {
        stack->vstack=NULL;
        stack->vsp=NULL;
        stack->lstack=NULL;
        stack->lsp=NULL;
    } else {
        stack->lstack=(CGIO_LOCATION *)CarveAlloc(carveLOCATION_STACK);
        stack->lsp=stack->lstack;
        stack->vstack=(YYSTYPE *)CarveAlloc(carveVALUE_STACK);
        stack->vsp=stack->vstack;
        stack->vsp->tree=NULL;
    }
    stack->sstack=(unsigned short *)CarveAlloc(carveSTATE_STACK);
    stack->ssp=stack->sstack;
    stack->exhaust=stack->sstack+255;
    *stack->ssp=0;
}
void commonInit(PARSE_STACK *stack)
{
    stack->gstack=NULL;
    stack->reset_scope=CurrScope;
    stack->qualifications=NULL;
    VstkOpen(&stack->look_ahead_storage,20,16);
    stack->look_ahead_count=0;
    stack->look_ahead_index=0;
    stack->template_record_tokens=NULL;
    stack->class_colon=NULL;
    stack->no_super_tokens=0;
    stack->use_saved_tokens=0;
    stack->favour_reduce=0;
    stack->favour_shift=0;
    stack->look_ahead_stack=0;
    stack->look_ahead_active=0;
    stack->template_decl=0;
    stack->template_args=0;
    stack->template_class_inst_defer=0;
    stack->special_colon_colon=0;
    /* Original leaves these six unused stack bits uninitialized. */
    stack->reserved_flags=0;
}
void newDeclStack(PARSE_STACK *stack)
{
    initParseStacks(stack,0);
    commonInit(stack);
}
struct decl_spec *PTypeDefault(void);
void pushDefaultDeclSpec(PARSE_STACK *stack)
{
    GLOBAL_STACK *top=GStackPush(&stack->gstack,1);
    top->u.dspec=PTypeDefault();
}
void pushDeclSpec(PARSE_STACK *stack,DECL_SPEC *spec)
{
    GLOBAL_STACK *top=GStackPush(&stack->gstack,1);
    top->u.dspec=spec;
}
enum yy_token {
    Y_GLOBAL_ID=94,
    Y_GLOBAL_TYPE_NAME=95,
    Y_GLOBAL_OPERATOR=96,
    Y_GLOBAL_TILDE=97,
    Y_GLOBAL_NEW=98,
    Y_GLOBAL_DELETE=99,
    Y_SCOPED_ID=101,
    Y_SCOPED_TYPE_NAME=102,
    Y_SCOPED_OPERATOR=103,
    Y_SCOPED_TILDE=104,
    Y_SCOPED_TIMES=105,
    Y_TEMPLATE_SCOPED_ID=106,
    Y_TEMPLATE_SCOPED_TYPE_NAME=107,
    Y_TEMPLATE_SCOPED_OPERATOR=108,
    Y_TEMPLATE_SCOPED_TILDE=109,
    Y_TEMPLATE_SCOPED_TIMES=110,

    Y_BAD=0,
    Y_IMPOSSIBLE=3,
    Y_ASM=4,
    Y_AUTO=5,
    Y_CHAR=6,
    Y_CLASS=7,
    Y_CONST=8,
    Y_DELETE=9,
    Y_DOUBLE=10,
    Y_ELSE=11,
    Y_ENUM=12,
    Y_EXTERN=13,
    Y_FLOAT=14,
    Y_FOR=15,
    Y_FRIEND=16,
    Y_INLINE=17,
    Y_INT=18,
    Y_LONG=19,
    Y_NEW=20,
    Y_OPERATOR=21,
    Y_PRIVATE=22,
    Y_PROTECTED=23,
    Y_PUBLIC=24,
    Y_REGISTER=25,
    Y_SHORT=26,
    Y_SIGNED=27,
    Y_SIZEOF=28,
    Y_STATIC=29,
    Y_STRUCT=30,
    Y_TEMPLATE=31,
    Y_THIS=32,
    Y_THROW=33,
    Y_TYPEDEF=34,
    Y_UNION=35,
    Y_UNSIGNED=36,
    Y_VIRTUAL=37,
    Y_VOID=38,
    Y_VOLATILE=39,
    Y_COMMA=40,
    Y_QUESTION=41,
    Y_COLON=42,
    Y_SEMI_COLON=43,
    Y_LEFT_PAREN=44,
    Y_RIGHT_PAREN=45,
    Y_LEFT_BRACKET=46,
    Y_RIGHT_BRACKET=47,
    Y_LEFT_BRACE=48,
    Y_RIGHT_BRACE=49,
    Y_TILDE=50,
    Y_EQUAL=51,
    Y_EQ=52,
    Y_EXCLAMATION=53,
    Y_NE=54,
    Y_OR=55,
    Y_OR_EQUAL=56,
    Y_OR_OR=57,
    Y_AND=58,
    Y_AND_EQUAL=59,
    Y_AND_AND=60,
    Y_XOR=61,
    Y_XOR_EQUAL=62,
    Y_GT=63,
    Y_GE=64,
    Y_LT=65,
    Y_LE=66,
    Y_LSHIFT=67,
    Y_LSHIFT_EQUAL=68,
    Y_RSHIFT=69,
    Y_RSHIFT_EQUAL=70,
    Y_PLUS=71,
    Y_PLUS_EQUAL=72,
    Y_PLUS_PLUS=73,
    Y_MINUS=74,
    Y_MINUS_EQUAL=75,
    Y_MINUS_MINUS=76,
    Y_TIMES=77,
    Y_TIMES_EQUAL=78,
    Y_DIVIDE=79,
    Y_DIVIDE_EQUAL=80,
    Y_PERCENT=81,
    Y_PERCENT_EQUAL=82,
    Y_DOT=83,
    Y_DOT_DOT_DOT=84,
    Y_DOT_STAR=85,
    Y_ARROW=86,
    Y_ARROW_STAR=87,
    Y_SEG_OP=88,
    Y_TYPE_NAME=89,
    Y_TEMPLATE_NAME=90,
    Y_CONSTANT=91,
    Y_STRING=92,
    Y_ID=93,
    Y___OFFSETOF=118,
    Y___TYPEOF=119,
    Y___BASED=120,
    Y___CDECL=121,
    Y___EXPORT=122,
    Y___FAR=123,
    Y___FAR16=124,
    Y___FORTRAN=125,
    Y___HUGE=126,
    Y___INTERRUPT=127,
    Y___LOADDS=128,
    Y___NEAR=129,
    Y__PACKED=130,
    Y___PASCAL=131,
    Y___PRAGMA=132,
    Y___SAVEREGS=133,
    Y__SEG16=134,
    Y___SEGMENT=135,
    Y___SEGNAME=136,
    Y___SELF=137,
    Y___STDCALL=138,
    Y__SYSCALL=139,
};
/* Recovered grammar translation, named from the released grammar/token
 * declarations and checked against the1994 ordinary scanner slots. The
 * synthetic slots are named from the actual original Tokens table; the
 * reserved170..256 slots are verified zero. */
const unsigned char yytranslate[257]={
    Y_BAD, /* T_NULL */
    Y_BAD, /* T_EOF */
    Y_COMMA, /* T_COMMA */
    Y_QUESTION, /* T_QUESTION */
    Y_COLON, /* T_COLON */
    Y_SEMI_COLON, /* T_SEMI_COLON */
    Y_BAD, /* T_COLON_COLON */
    Y_LEFT_PAREN, /* T_LEFT_PAREN */
    Y_RIGHT_PAREN, /* T_RIGHT_PAREN */
    Y_LEFT_BRACKET, /* T_LEFT_BRACKET */
    Y_RIGHT_BRACKET, /* T_RIGHT_BRACKET */
    Y_LEFT_BRACE, /* T_LEFT_BRACE */
    Y_RIGHT_BRACE, /* T_RIGHT_BRACE */
    Y_DOT, /* T_DOT */
    Y_TILDE, /* T_TILDE */
    Y_EQUAL, /* T_EQUAL */
    Y_EQ, /* T_EQ */
    Y_EXCLAMATION, /* T_EXCLAMATION */
    Y_NE, /* T_NE */
    Y_OR, /* T_OR */
    Y_OR_EQUAL, /* T_OR_EQUAL */
    Y_OR_OR, /* T_OR_OR */
    Y_XOR, /* T_XOR */
    Y_XOR_EQUAL, /* T_XOR_EQUAL */
    Y_AND, /* T_AND */
    Y_AND_EQUAL, /* T_AND_EQUAL */
    Y_AND_AND, /* T_AND_AND */
    Y_GT, /* T_GT */
    Y_GE, /* T_GE */
    Y_RSHIFT, /* T_RSHIFT */
    Y_RSHIFT_EQUAL, /* T_RSHIFT_EQUAL */
    Y_LT, /* T_LT */
    Y_LE, /* T_LE */
    Y_LSHIFT, /* T_LSHIFT */
    Y_LSHIFT_EQUAL, /* T_LSHIFT_EQUAL */
    Y_PLUS, /* T_PLUS */
    Y_PLUS_EQUAL, /* T_PLUS_EQUAL */
    Y_PLUS_PLUS, /* T_PLUS_PLUS */
    Y_MINUS, /* T_MINUS */
    Y_MINUS_EQUAL, /* T_MINUS_EQUAL */
    Y_MINUS_MINUS, /* T_MINUS_MINUS */
    Y_TIMES, /* T_TIMES */
    Y_TIMES_EQUAL, /* T_TIMES_EQUAL */
    Y_DIVIDE, /* T_DIVIDE */
    Y_DIVIDE_EQUAL, /* T_DIVIDE_EQUAL */
    Y_PERCENT, /* T_PERCENT */
    Y_PERCENT_EQUAL, /* T_PERCENT_EQUAL */
    Y_BAD, /* T_SHARP */
    Y_DOT_DOT_DOT, /* T_DOT_DOT_DOT */
    Y_BAD, /* T_SHARP_SHARP */
    Y_ARROW, /* T_ARROW */
    Y_SEG_OP, /* T_SEG_OP */
    Y_ARROW_STAR, /* T_ARROW_STAR */
    Y_DOT_STAR, /* T_DOT_STAR */
    Y_ID, /* T_ID */
    Y_CONSTANT, /* T_CONSTANT */
    Y_STRING, /* T_STRING */
    Y_BAD, /* T_WHILE */
    Y___PRAGMA, /* T___PRAGMA */
    Y___SEGNAME, /* T___SEGNAME */
    Y_OPERATOR, /* T_OPERATOR */
    Y_BAD, /* T_IF */
    Y_FOR, /* T_FOR */
    Y_VIRTUAL, /* T_VIRTUAL */
    Y___INTERRUPT, /* T___INTERRUPT */
    Y_PRIVATE, /* T_PRIVATE */
    Y__PACKED, /* T__PACKED */
    Y_EXTERN, /* T_EXTERN */
    Y___TYPEOF, /* T___TYPEOF */
    Y___FAR16, /* T___FAR16 */
    Y___OFFSETOF, /* T___OFFSETOF */
    Y_SHORT, /* T_SHORT */
    Y_BAD, /* T_CONTINUE */
    Y___BASED, /* T___BASED */
    Y_NEW, /* T_NEW */
    Y__SEG16, /* T__SEG16 */
    Y___CDECL, /* T___CDECL */
    Y___FAR, /* T___FAR */
    Y___NEAR, /* T___NEAR */
    Y_REGISTER, /* T_REGISTER */
    Y___PASCAL, /* T___PASCAL */
    Y___FORTRAN, /* T___FORTRAN */
    Y_DOUBLE, /* T_DOUBLE */
    Y___SELF, /* T___SELF */
    Y_VOLATILE, /* T_VOLATILE */
    Y_BAD, /* T_CATCH */
    Y___STDCALL, /* T___STDCALL */
    Y_CONST, /* T_CONST */
    Y_STATIC, /* T_STATIC */
    Y_BAD, /* T_DEFAULT */
    Y_LONG, /* T_LONG */
    Y_UNION, /* T_UNION */
    Y__SYSCALL, /* T__SYSCALL */
    Y_FLOAT, /* T_FLOAT */
    Y_THIS, /* T_THIS */
    Y_ENUM, /* T_ENUM */
    Y___SEGMENT, /* T___SEGMENT */
    Y_TYPEDEF, /* T_TYPEDEF */
    Y_BAD, /* T_CASE */
    Y___EXPORT, /* T___EXPORT */
    Y_UNSIGNED, /* T_UNSIGNED */
    Y_BAD, /* T_GOTO */
    Y_BAD, /* T_RETURN */
    Y_ELSE, /* T_ELSE */
    Y_CLASS, /* T_CLASS */
    Y_PROTECTED, /* T_PROTECTED */
    Y_AUTO, /* T_AUTO */
    Y_BAD, /* T_BREAK */
    Y_ASM, /* T_ASM */
    Y_DELETE, /* T_DELETE */
    Y_THROW, /* T_THROW */
    Y_TEMPLATE, /* T_TEMPLATE */
    Y_STRUCT, /* T_STRUCT */
    Y___HUGE, /* T___HUGE */
    Y_BAD, /* T_SWITCH */
    Y_BAD, /* T_DO */
    Y_INT, /* T_INT */
    Y___SAVEREGS, /* T___SAVEREGS */
    Y_VOID, /* T_VOID */
    Y_INLINE, /* T_INLINE */
    Y_SIZEOF, /* T_SIZEOF */
    Y_CHAR, /* T_CHAR */
    Y___LOADDS, /* T___LOADDS */
    Y_FRIEND, /* T_FRIEND */
    Y_BAD, /* T_TRY */
    Y_PUBLIC, /* T_PUBLIC */
    Y_SIGNED, /* T_SIGNED */
    Y_BAD, /* T_MACRO_PARM */
    Y_BAD, /* T_MACRO */
    Y_BAD, /* T_FUNCTION */
    Y_BAD, /* T_FNEND */
    Y_BAD, /* T_JUMP */
    Y_BAD, /* T_JUMPTRUE */
    Y_BAD, /* T_JUMPFALSE */
    Y_BAD, /* T_PARM_LIST */
    Y_BAD, /* T_PARM */
    Y_BAD, /* T_CALL */
    Y_BAD, /* T_INDIRECT_CALL */
    Y_BAD, /* T_LABEL */
    Y_BAD, /* T_ADDRESS */
    Y_BAD, /* T_ENDFOR */
    Y_BAD, /* T_QUESTION */
    Y_BAD, /* T_CONVERT */
    Y_BAD, /* T_WHITE_SPACE */
    Y_BAD, /* T_BAD_CHAR */
    Y_BAD, /* T_SAVED_ID */
    Y_BAD, /* T_MACRO_SHARP_SHARP */
    Y_BAD, /* T_MACRO_SHARP */
    Y_BAD, /* T_BAD_TOKEN */
    Y_BAD, /* T_ATSIGN */
    Y_BAD, /* T_GLOBAL_TEMPLATE_NAME */
    Y_GLOBAL_ID, /* T_GLOBAL_ID */
    Y_GLOBAL_TYPE_NAME, /* T_GLOBAL_TYPE_NAME */
    Y_GLOBAL_OPERATOR, /* T_GLOBAL_OPERATOR */
    Y_GLOBAL_TILDE, /* T_GLOBAL_TILDE */
    Y_GLOBAL_NEW, /* T_GLOBAL_NEW */
    Y_GLOBAL_DELETE, /* T_GLOBAL_DELETE */
    Y_SCOPED_ID, /* T_SCOPED_ID */
    Y_SCOPED_TYPE_NAME, /* T_SCOPED_TYPE_NAME */
    Y_SCOPED_OPERATOR, /* T_SCOPED_OPERATOR */
    Y_SCOPED_TILDE, /* T_SCOPED_TILDE */
    Y_SCOPED_TIMES, /* T_SCOPED_TIMES */
    Y_IMPOSSIBLE, /* T_IMPOSSIBLE */
    Y_BAD, /* T_LSTRING */
    Y_TEMPLATE_SCOPED_ID, /* T_TEMPLATE_SCOPED_ID */
    Y_TEMPLATE_SCOPED_TYPE_NAME, /* T_TEMPLATE_SCOPED_TYPE_NAME */
    Y_TEMPLATE_SCOPED_OPERATOR, /* T_TEMPLATE_SCOPED_OPERATOR */
    Y_TEMPLATE_SCOPED_TILDE, /* T_TEMPLATE_SCOPED_TILDE */
    Y_TEMPLATE_SCOPED_TIMES, /* T_TEMPLATE_SCOPED_TIMES */
    Y_BAD, /* T_LAST_TOKEN */
    /* Verified reserved scanner slots170..256 are zero. */
};
static YYSTYPE yylval;
typedef struct look_ahead_storage {
    YYSTYPE yylval;
    CGIO_LOCATION yylocation;
    unsigned yytok;
} LOOK_AHEAD_STORAGE;
void *VstkIndex(VSTK_CTL *stack,int index);
unsigned char lexCategory(SCOPE scope,PTREE tree,unsigned char control)
{
    char *name=tree->u.id.name;
    SYMBOL_NAME found;
    SYMBOL sym;
    TYPE type,class_type;
    if (control&1) found=ScopeYYLexical(scope,name);
    else found=ScopeYYMember(scope,name);
    if (found && !found->name_syms) {
        sym=found->name_type;
        if (sym->id==12) return 2; /* old SC_CLASS_TEMPLATE */
        type=sym->sym_type;
        if (control&2) {
            class_type=StructType(type);
            if (class_type && (class_type->flag&0x800) &&
                class_type->u.c.info->name==name) return 2;
        }
        tree->type=type;
        return 1;
    }
    return 0;
}
int templateScopedChain(PARSE_STACK *stack) { (void)stack; STUB_TRAP(); return 0; }
int globalChain(PARSE_STACK *stack) { (void)stack; STUB_TRAP(); return 0; }
void LookPastName(void);
extern char *SavedId;
PTREE setLocation(PTREE tree,CGIO_LOCATION *location);
PTREE makeId(void)
{
    PTREE tree=PTreeId(SavedId);
    setLocation(tree,&yylocation);
    return tree;
}
PTREE PTreeUnary(unsigned char op,PTREE operand);
PTREE PTreeBinary(unsigned char op,PTREE left,PTREE right);
void nextRecordedToken(PARSE_STACK *stack);
PTREE makeUnary(unsigned char op,PTREE operand)
{ return setLocation(PTreeUnary(op,operand),&yylocation); }
PTREE makeBinary(unsigned char op,PTREE left,PTREE right)
{ return setLocation(PTreeBinary(op,left,right),&yylocation); }
PTREE currBinary(unsigned char op,PTREE left,PTREE right)
{
    PTREE tree=PTreeBinary(op,left,right);
    CGIO_LOCATION location;
    SrcFileGetTokenLocn(&location);
    return setLocation(tree,&location);
}
SEARCH_RESULT *ScopeFindMemberClassType(SCOPE scope,char *name)
{ (void)scope; (void)name; STUB_TRAP(); return NULL; }
TYPE findGenericType(SCOPE scope,char *name)
{ (void)scope; (void)name; STUB_TRAP(); return NULL; }
int ScopeAmbiguousSymbol(SEARCH_RESULT *result,SYMBOL sym);
TYPE StructOpened(TYPE type);
TYPE checkColonColon(SEARCH_RESULT *result,char *name,SCOPE not_nested)
{
    TYPE type;
    if(result==NULL) {
        if(not_nested!=NULL) {
            if(ScopeType(not_nested,4)) return findGenericType(not_nested,name);
            CErr2p(241,name);
        } else CErr2p(239,name);
        return NULL;
    }
    type=TypedefRemove(result->sym_name->name_type->sym_type);
    if(type->id!=TYP_CLASS) { CErr2p(185,name); return NULL; }
    ScopeAmbiguousSymbol(result,result->sym_name->name_type);
    return type;
}
void reuseGStack(PARSE_STACK *stack,unsigned char id)
{
    GStackPop(&stack->gstack);
    GStackPush(&stack->gstack,id);
}
struct data_init;
void DataInitStart(struct data_init *data,DECL_INFO *info);
void DataInitSimple(PTREE expr);
PTREE MakeNewExpr(PTREE global,PTREE placement,DECL_INFO *info,PTREE init);
DECL_INFO *MakeNewDeclarator(DECL_SPEC *spec,DECL_INFO *ptrs,DECL_INFO *arrays);
DECL_INFO *MakeNewDynamicArray(PTREE nelem);
DECL_SPEC *CheckTypeSpecifier(DECL_SPEC *spec);
void DataInitConstructorParms(PTREE expr);
void tryCtorStyleInit(PARSE_STACK *stack,DECL_INFO *info)
{
    PTREE expr_list=info->defarg_expr;
    if(expr_list!=NULL) {
        info->defarg_expr=NULL;
        GStackPush(&stack->gstack,5);
        DataInitStart((struct data_init *)&stack->gstack->u,info);
        DataInitConstructorParms(expr_list);
        GStackPop(&stack->gstack);
    }
    FreeDeclInfo(info);
}
int scopedChain(PARSE_STACK *stack,PTREE start,PTREE id,unsigned control)
{
    SCOPE scope=CurrScope,lexical_lookup;
    SEARCH_RESULT *result;
    TYPE type,class_type;
    char *name;
    PTREE curr=start;
    if(start!=NULL) {
        scope=FileScope;
        if((control&1) && start->u.subtree[1]->type!=NULL) {
            scope=start->u.subtree[1]->type->u.c.scope;
            lexical_lookup=NULL;
        } else lexical_lookup=scope;
    } else lexical_lookup=scope;
    yylval.tree=NULL;
    for(;;) {
        curr=currBinary(55,curr,id);
        nextRecordedToken(stack);
        nextRecordedToken(stack);
        name=id->u.id.name;
        if(lexical_lookup) result=ScopeFindLexicalClassType(scope,name);
        else result=ScopeFindMemberClassType(scope,name);
        type=checkColonColon(result,name,lexical_lookup);
        ScopeFreeResult(result);
        lexical_lookup=NULL;
        class_type=StructType(type);
        if(class_type) {
            if(StructOpened(class_type)) scope=class_type->u.c.scope;
            else if(CurToken!=0x29) { CErr2p(304,name); type=NULL; }
        }
        id->type=type;
        switch(CurToken) {
        case 0x36: LookPastName();
        case 0x91:
            id=makeId();
            if(LAToken==6) continue;
            yylval.tree=makeBinary(53,curr,id);
            if(name==id->u.id.name && CurrScope==FileScope) return Y_SCOPED_ID;
            return lexCategory(scope,id,0)?Y_SCOPED_TYPE_NAME:Y_SCOPED_ID;
        case 0xe:
            yylval.tree=makeUnary(11,curr); return Y_SCOPED_TILDE;
        case 0x3c:
            yylval.tree=makeUnary(54,curr); return Y_SCOPED_OPERATOR;
        case 0x29:
            yylval.tree=makeUnary(40,curr); return Y_SCOPED_TIMES;
        default:
            PTreeFreeSubtrees(curr); return Y_IMPOSSIBLE;
        }
    }
}
int doId(void)
{
    unsigned char category,control=1;
    PTREE tree=makeId();
    yylval.tree=tree;
    if (LAToken==0x1f) control=3;
    category=lexCategory(CurrScope,tree,control);
    if (!category) return Y_ID;
    if (category==1) return Y_TYPE_NAME;
    return Y_TEMPLATE_NAME;
}
struct string_constant *StringCreate(char *text,unsigned length);

PTREE PTreeUIntConstant(unsigned long value,type_id id);
PTREE PTreeFloatingConstant(double value,type_id id);
double SafeAtof(char *text);
PTREE setLocation(PTREE tree,CGIO_LOCATION *location)
{
    tree->locn.src_file=location->src_file;
    tree->locn.line=location->line;
    tree->locn.column=location->column;
    return tree;
}
int yylex(PARSE_STACK *stack)
{
    LOOK_AHEAD_STORAGE *saved;
    unsigned token;
    int no_super_token=0,special_colon_colon=0;
    PTREE tree;
    stack->favour_reduce=0;
    stack->favour_shift=0;
    if (stack->use_saved_tokens) {
        saved=(LOOK_AHEAD_STORAGE *)VstkIndex(&stack->look_ahead_storage,
                                            stack->look_ahead_index);
        yylval=saved->yylval;
        yylocation=saved->yylocation;
        token=saved->yytok;
        if (token==117) { stack->favour_shift=1; token=Y_LEFT_PAREN; }
        else if (token==116) { stack->favour_reduce=1; token=Y_LEFT_PAREN; }
        else if (token==Y_TYPE_NAME && lexCategory(CurrScope,yylval.tree,1)!=1) {
            yylval.tree->type=NULL;
            token=Y_ID;
        }
    } else {
        if (stack->no_super_tokens) {
            no_super_token=1;
            stack->no_super_tokens=0;
        }
        if (stack->special_colon_colon) {
            special_colon_colon=1;
            stack->special_colon_colon=0;
        }
        token=currToken;
        if (token==Y_IMPOSSIBLE) {
            switch (CurToken) {
            case 6:
                if (no_super_token) token=yytranslate[6];
                else if (special_colon_colon) token=templateScopedChain(stack);
                else token=globalChain(stack);
                break;
            case 0x36:
                LookPastName();
            case 0x91:
                if (LAToken==6 && !no_super_token)
                    token=scopedChain(stack,NULL,makeId(),0);
                else token=doId();
                break;
            case 0x38:
                tree=PTreeLiteral(StringCreate(Buffer,TokenSavedLen-1));
                yylval.tree=tree;
                setLocation(tree,&yylocation);
                token=Y_STRING;
                break;
            case 0xa3:
                tree=PTreeLiteralWide(StringCreate(Buffer,TokenSavedLen-1));
                yylval.tree=tree;
                setLocation(tree,&yylocation);
                token=Y_STRING;
                break;
            case 0x37:
                if (ConstType>=TYP_FLOAT && ConstType<=TYP_LONG_DOUBLE)
                    tree=PTreeFloatingConstant(SafeAtof(Buffer),(type_id)ConstType);
                else if (ConstType==TYP_UINT || ConstType==TYP_ULONG || ConstType==TYP_UCHAR)
                    tree=PTreeUIntConstant(Constant,(type_id)ConstType);
                else tree=PTreeIntConstant(Constant,(type_id)ConstType);
                yylval.tree=tree;
                setLocation(tree,&yylocation);
                token=Y_CONSTANT;
                break;
            default:
                if ((unsigned)CurToken>=257) STUB_TRAP();
                token=yytranslate[CurToken];
                break;
            }
        }
    }
    currToken=token;
    return token;
}
unsigned char normalYYAction(unsigned token,PARSE_STACK *stack,unsigned *action)
{
    unsigned short *ssp=stack->ssp;
    unsigned top_state=*ssp,raw_action,rule,lhs;
    unsigned bit_index=token>>3;
    unsigned mask=1<<(token&7);
    for (;;) {
        if (yybitcheck[bit_index+yybitbase[top_state]]&mask) {
            raw_action=yyaction[token+yyactionbase[top_state]];
            if (!(raw_action&0x8000)) { *action=raw_action; return 10; }
        } else {
            raw_action=yydefaction[top_state];
            if (!raw_action) return 7;
        }
        rule=raw_action&0x3fff;
        if (!(raw_action&0x4000)) { *action=rule+0x2f9; return 10; }
        lhs=yyplhstab[rule];
        top_state=yyaction[lhs+yygotobase[ssp[-1]]];
        *ssp=(unsigned short)top_state;
    }
}
unsigned short lookAheadYYAction(unsigned token,PARSE_STACK *stack,PARSE_STACK *scan)
{ (void)token; (void)stack; (void)scan; STUB_TRAP(); return 0; }
PTREE PTreeThis(void);
PTREE MakeDeleteExpr(PTREE global,unsigned char operation,PTREE expr);
PTREE MakeScopedDestructorId(PTREE scoped,PTREE id);
PTREE MakeScopedOperatorId(PTREE scoped,unsigned char operation);
void ScopeQualifyPush(SCOPE scope,SCOPE reset);
SCOPE ScopeQualifyPop(void);
void pushOperatorQualification(PTREE tree)
{
    PTREE scope_tree=tree->u.subtree[0]->u.subtree[1];
    TYPE class_type=StructType(scope_tree->type);
    if(class_type) ScopeQualifyPush(class_type->u.c.scope,CurrScope);
}
/* Outer parser automaton is a current-case discovery candidate. Reduction
 * semantics and their original generated tables are not qualified yet;
 * that domain traps in both builds and this owner has no accepted credit. */
void pushClassData(PARSE_STACK *stack,unsigned short flags,unsigned char extra,TYPE modifiers)
{
    GLOBAL_STACK **head=&stack->gstack;
    GStackPush(head,4);
    ClassPush(&(*head)->u.classdata);
    if (stack->template_decl) extra|=2;
    ClassInitState(flags,extra,modifiers);
}
unsigned short GOTOYYAction(PARSE_STACK *stack,unsigned rule)
{
    return yyaction[yyplhstab[rule]+yygotobase[*stack->ssp]];
}
enum {
    YYUSED=0x2f9, YYSTOP=91, YYAMBIGS0=173,
    YYAMBIGH0=388, YYAMBIGR0=272,
    RULE_PACKED_CLASS_EMPTY=476, RULE_TEMPLATE_KEY=522
};
void SetErrLoc(CGIO_LOCATION *location);
void ParseFlush(void);
int tokenMakesPTREE(unsigned token);
void setNoSuperTokens(PARSE_STACK *stack)
{
    stack->no_super_tokens=1;
}
int doFnbodyRewrite(void)
{
    if (ScopeType(CurrScope,1)) return 1;
    return ScopeType(CurrScope,4)!=0;
}
void ParseFlush(void);
PTREE getMultiToken(void)
{
    PTREE tree=NULL;
    if (tokenMakesPTREE(currToken)) {
        tree=yylval.tree;
        yylval.tree=NULL;
        currToken=Y_IMPOSSIBLE;
    } else ParseFlush();
    return tree;
}
REWRITE *RewritePackageFunction(PTREE tree);
void FunctionBody(DECL_INFO *info);
void TemplateFunctionAttachDefn(DECL_INFO *info);
void ClassStoreInlineFunc(DECL_INFO *info);
extern TYPE TypeError;
DECL_SPEC *sendType(PTREE tree)
{
    TYPE type;
    DECL_SPEC *spec;
    SCOPE scope=0;
    PTREE sub_tree;
    if (tree->op==9) {
        type=tree->u.subtree[1]->type;
        sub_tree=tree->u.subtree[0]->u.subtree[1];
        if (sub_tree) {
            TYPE class_type=StructType(sub_tree->type);
            if (class_type) scope=class_type->u.c.scope;
        } else scope=FileScope;
    } else type=tree->type;
    if (!type) type=TypeError;
    spec=PTypeActualTypeName(type,tree);
    if (scope) spec->scope=scope;
    return spec;
}
DECL_SPEC *sendClass(PTREE tree)
{
    DECL_SPEC *spec=sendType(tree);
    if (!StructType(spec->partial)) { PTypeRelease(spec); spec=0; }
    return spec;
}
PTREE CheckScopedId(PTREE tree);
PTREE MakeDestructorId(PTREE id);
PTREE MakeOperatorId(unsigned char op);
PTREE MakeMemInitItem(DECL_SPEC *spec,PTREE id,PTREE expressions);
PTREE PTreeBinary(unsigned char op,PTREE left,PTREE right);
PTREE PTreeUnary(unsigned char op,PTREE operand);
DECL_SPEC *ClassEnd(void);
void markFieldName(PTREE id);
PTREE PTreeReplaceLeft(PTREE tree,PTREE left);
PTREE PTreeReplaceRight(PTREE tree,PTREE right);
unsigned short ClassBaseQualifiers(unsigned short old,unsigned short add);
struct base_class *ClassBaseSpecifier(unsigned short flags,DECL_SPEC *spec);
struct base_class *ClassBaseList(struct base_class *list,struct base_class *base);
void ClassBaseClause(struct base_class *bases);
unsigned char doAction(unsigned token,PARSE_STACK *stack)
{
    unsigned action,rule,length;
    YYSTYPE yyval,*yyvp;
    unsigned char what,declaration,after_name;
    for (;;) {
        if (*stack->ssp==YYAMBIGS0 && token==Y_LEFT_PAREN) {
            if (stack->favour_shift) action=YYAMBIGH0;
            else if (stack->favour_reduce) action=YYAMBIGR0+YYUSED;
            else {
                CErr1(604);
                action=lookAheadYYAction(token,stack,stack);
            }
        } else {
            what=normalYYAction(token,stack,&action);
            if (what!=10) return what;
        }
        if (action<YYUSED) {
            if (action==YYSTOP) return 2;
            if (stack->ssp==stack->exhaust) return 8;
            *++stack->ssp=(unsigned short)action;
            *++stack->vsp=yylval;
            *++stack->lsp=yylocation;
            if (token==103 || token==108) pushOperatorQualification(yylval.tree);
            return 1;
        }
        rule=action-YYUSED;
        if (rule>=545) STUB_TRAP();
        length=yyplentab[rule];
        if (length) {
            stack->ssp-=length;
            stack->vsp-=length;
            stack->lsp-=length;
            if (stack->ssp<stack->sstack) { CErr1(122); CSuicide(); }
        } else if (stack->ssp==stack->exhaust) return 8;
        action=GOTOYYAction(stack,rule);
        *++stack->ssp=(unsigned short)action;
        yyvp=++stack->vsp;
        what=6;
        switch (rule) {
        case 326: /* actual-declarator ( expression-list ) */
            yyvp[0].dinfo=FinishDeclarator(stack->gstack->u.dspec,yyvp[0].dinfo);
            yyval.dinfo=InsertDeclInfo(CurrScope,yyvp[0].dinfo);
            /* The old action stores the list directly; later grammar adds
             * a separate source-location helper at this boundary. */
            yyval.dinfo->defarg_expr=yyvp[2].tree;
            break;
        case 132: case 340: case 400: case 408:
            yyval.dinfo=AddArrayDeclarator(yyvp[0].dinfo,yyvp[2].tree);
            break;
        case 341: case 402: case 410:
            yyval.dinfo=AddArrayDeclarator(yyvp[0].dinfo,NULL);
            break;
        case 403: case 411:
            yyval.dinfo=AddArrayDeclarator(MakeDeclarator(NULL,NULL),NULL);
            break;
        case 409:
            yyval.dinfo=AddArrayDeclarator(MakeDeclarator(NULL,NULL),yyvp[1].tree);
            break;
        case 9: /* constant-expression requiring an integral nonzero fallback */
            yyval.tree=PTreeNonZeroConstantExpr(yyvp[0].tree);
            break;
        case 385: /* type-id : type-specifiers */
            yyval.tree=DoDeclSpec(stack->gstack->u.dspec);
            GStackPop(&stack->gstack);
            break;
        case 528: /* template-class-instantiation : template-name < args */
            yyval.dspec=TemplateClassInstantiation(yyvp[0].tree,yyvp[2].tree,0);
            setWatchColonColon(stack,yyval.dspec);
            break;
        case 268:
            yyval.dspec=PTypeLinkage();
            break;

        case 320: /* linkage-id : strings (actual old rule number) */
            LinkagePush(yyvp[0].tree->u.string->string);
            StringTrash(yyvp[0].tree->u.string);
            PTreeFree(yyvp[0].tree);
            if(!ScopeType(CurrScope,0)) CErr1(115);
            /* Original leaves the unused semantic value unspecified. */
            yyval.pointer=NULL;
            break;
        case 8: /* expr-decl-stmt : expression (semicolon is lookahead) */
            yyval=yyvp[0];
            if(token!=43) {
                SetErrLoc(&yylocation);
                CErr1(6);
                what=9;
            }
            break;
        case 16: /* assignment-operator '=' */
            yyval.tree=setLocation(PTreeBinary(13,NULL,NULL),&stack->lsp[1]);
            break;
        case 17: /* assignment-operator *= */
            yyval.tree=setLocation(PTreeBinary(27,NULL,NULL),&stack->lsp[1]);
            break;
        case 18: /* assignment-operator /= */
            yyval.tree=setLocation(PTreeBinary(28,NULL,NULL),&stack->lsp[1]);
            break;
        case 19: /* assignment-operator %= */
            yyval.tree=setLocation(PTreeBinary(29,NULL,NULL),&stack->lsp[1]);
            break;
        case 20: /* assignment-operator += */
            yyval.tree=setLocation(PTreeBinary(25,NULL,NULL),&stack->lsp[1]);
            break;
        case 21: /* assignment-operator -= */
            yyval.tree=setLocation(PTreeBinary(26,NULL,NULL),&stack->lsp[1]);
            break;
        case 22: /* assignment-operator <<= */
            yyval.tree=setLocation(PTreeBinary(34,NULL,NULL),&stack->lsp[1]);
            break;
        case 23: /* assignment-operator >>= */
            yyval.tree=setLocation(PTreeBinary(33,NULL,NULL),&stack->lsp[1]);
            break;
        case 24: /* assignment-operator |= */
            yyval.tree=setLocation(PTreeBinary(31,NULL,NULL),&stack->lsp[1]);
            break;
        case 25: /* assignment-operator &= */
            yyval.tree=setLocation(PTreeBinary(30,NULL,NULL),&stack->lsp[1]);
            break;
        case 26: /* assignment-operator ^= */
            yyval.tree=setLocation(PTreeBinary(32,NULL,NULL),&stack->lsp[1]);
            break;
        case 28: case 33: /* bind both operands to the operator node */
            yyval.tree=PTreeReplaceLeft(yyvp[1].tree,yyvp[0].tree);
            yyval.tree=PTreeReplaceRight(yyval.tree,yyvp[2].tree);
            break;
        case 102: /* bind the operand to its unary operator node */
            yyval.tree=PTreeReplaceLeft(yyvp[0].tree,yyvp[1].tree);
            break;
        case 142: /* postfix-expression '[' expression ']' */
            yyval.tree=setLocation(PTreeBinary(47,yyvp[0].tree,yyvp[2].tree),
                &stack->lsp[2]);
            break;
        case 34: /* throw-expression : throw assignment-expression */
            yyval.tree=setLocation(PTreeUnary(71,yyvp[1].tree),&stack->lsp[1]);
            break;
        case 44: /* logical-or-expression '||' logical-and-expression */
            yyval.tree=setLocation(PTreeBinary(21,yyvp[0].tree,yyvp[2].tree),
                &stack->lsp[2]);
            break;
        case 60: /* and-expression '&' equality-expression */
            yyval.tree=setLocation(PTreeBinary(5,yyvp[0].tree,yyvp[2].tree),
                &stack->lsp[2]);
            break;
        case 66: /* equality-expression '!=' relational-expression */
            yyval.tree=setLocation(PTreeBinary(15,yyvp[0].tree,yyvp[2].tree),
                &stack->lsp[2]);
            break;
        case 93: /* unary-operator '*' */
            yyval.tree=setLocation(PTreeUnary(40,NULL),&stack->lsp[1]);
            break;
        case 96: /* unary-operator '-' */
            yyval.tree=setLocation(PTreeUnary(10,NULL),&stack->lsp[1]);
            break;
        case 94: /* unary-operator '&' */
            yyval.tree=setLocation(PTreeUnary(12,NULL),&stack->lsp[1]);
            break;
        case 74: /* relational-expression '>' shift-expression */
            yyval.tree=setLocation(PTreeBinary(16,yyvp[0].tree,yyvp[2].tree),
                &stack->lsp[2]);
            break;
        case 38: case 40: /* ordinary/template conditional expression */
            yyvp[2].tree=setLocation(PTreeBinary(43,yyvp[2].tree,yyvp[4].tree),
                &stack->lsp[4]);
            yyval.tree=setLocation(PTreeBinary(41,yyvp[0].tree,yyvp[2].tree),
                &stack->lsp[2]);
            break;
        case 75: /* relational-expression '>=' shift-expression */
            yyval.tree=setLocation(PTreeBinary(19,yyvp[0].tree,yyvp[2].tree),
                &stack->lsp[2]);
            break;
        case 72: /* relational-expression '<' shift-expression */
            yyval.tree=setLocation(PTreeBinary(18,yyvp[0].tree,yyvp[2].tree),
                &stack->lsp[2]);
            break;
        case 83: /* multiplicative-expression '*' cast-expression */
            yyval.tree=setLocation(PTreeBinary(2,yyvp[0].tree,yyvp[2].tree),
                &stack->lsp[2]);
            break;
        case 84: /* multiplicative-expression '/' cast-expression */
            yyval.tree=setLocation(PTreeBinary(3,yyvp[0].tree,yyvp[2].tree),
                &stack->lsp[2]);
            break;
        case 85: /* multiplicative-expression '%' cast-expression */
            yyval.tree=setLocation(PTreeBinary(4,yyvp[0].tree,yyvp[2].tree),
                &stack->lsp[2]);
            break;
        case 80: /* additive-expression '+' multiplicative-expression */
            yyval.tree=setLocation(PTreeBinary(0,yyvp[0].tree,yyvp[2].tree),
                &stack->lsp[2]);
            break;
        case 100: /* unary-expression : Y_PLUS_PLUS cast-expression */
        case 101: /* unary-expression : Y_MINUS_MINUS cast-expression */
            yyval.tree=setLocation(PTreeUnary(rule==100?36:37,yyvp[1].tree),&stack->lsp[1]);
            break;
        case 2: /* goal: completed declaration stream, no semantic result */
            token=0;
            yyval.pointer=NULL;
            break;
        case 3: /* goal: exception declaration is treated as one argument */
            token=0;
            yyval.dinfo=AddArgument(NULL,yyvp[1].dinfo);
            break;
        case 0: case 1: case 4: case 5: /* completed special-start production */
            token=0;
            yyval=yyvp[1];
            break;
        case 12: case 517: /* first expression or member-initializer list item */
        case 533: /* first template argument */
            yyval.tree=PTreeBinary(44,NULL,yyvp[0].tree);
            break;
        case 510: /* base-qualifiers-opt : empty */
            yyval.flags=0;
            break;
        case 511: case 512: case 513: case 514:
            yyval.flags=ClassBaseQualifiers((unsigned short)yyvp[0].flags,
                rule==511?1:rule==512?4:rule==513?2:8);
            break;
        case 506: /* base-clause : Y_COLON base-specifier-list */
            ClassBaseClause(yyvp[1].pointer);
            yyval.pointer=NULL; /* ignored semantic value */
            break;
        case 507: /* base-specifier-list : base-specifier */
            yyval.pointer=ClassBaseList(NULL,yyvp[0].pointer);
            break;
        case 508: /* append a further base-specifier */
            yyval.pointer=ClassBaseList(yyvp[0].pointer,yyvp[2].pointer);
            break;
        case 509: /* base-specifier : qualifiers qualified-class-specifier */
            yyval.pointer=ClassBaseSpecifier((unsigned short)yyvp[0].flags,yyvp[1].dspec);
            break;
        case 13: /* append an expression-list item */
        case 518: /* append a member-initializer */
        case 534: /* appended template argument */
            yyval.tree=setLocation(PTreeBinary(44,yyvp[0].tree,yyvp[2].tree),&stack->lsp[2]);
            break;
        case 519: /* mem-initializer using a type-specifier */
            yyval.tree=MakeMemInitItem(yyvp[0].dspec,NULL,yyvp[2].tree);
            break;
        case 520: /* mem-initializer using a member identifier */
            yyval.tree=MakeMemInitItem(NULL,yyvp[0].tree,yyvp[2].tree);
            break;
        case 470: /* class-body : class-substance Y_RIGHT_BRACE */
            GStackPop(&stack->gstack);
            GStackPop(&stack->gstack);
            yyval=yyvp[0];
            break;
        case 471: case 472: /* end class substance, returning its decl-spec */
            yyval.dspec=ClassEnd();
            what=0;
            break;
        case 169: /* scoped identifier */
            yyval.tree=CheckScopedId(yyvp[0].tree);
            break;
        case 163: case 164: case 165:
            markFieldName(yyvp[0].tree);
            yyval=yyvp[0];
            break;
        case 147: case 148: /* member-selection prefix: expression '.'/'->' */
            yyval.tree=setLocation(PTreeBinary(rule==147?48:23,
                                               yyvp[0].tree,NULL),&stack->lsp[2]);
            break;
        case 133: /* delete-expression : delete cast-expression */
        case 134: /* delete-expression : delete '[' ']' cast-expression */
            yyval.tree=setLocation(MakeDeleteExpr(NULL,rule==133?58:60,
                yyvp[rule==133?1:4].tree),&stack->lsp[1]);
            break;
        case 135: /* global delete */
        case 136: /* global array delete */
            yyval.tree=setLocation(MakeDeleteExpr(yyvp[0].tree,rule==135?59:61,
                yyvp[rule==135?1:4].tree),&stack->lsp[1]);
            break;
        case 151: /* primary-expression : this */
            yyval.tree=setLocation(PTreeThis(),&stack->lsp[1]);
            break;
        case 141: /* postfix-expression '[' expression ']' */
            yyval.tree=setLocation(PTreeBinary(24,yyvp[0].tree,yyvp[2].tree),
                &stack->lsp[2]);
            break;
        case 143: /* completed member selection */
            yyval.tree=PTreeReplaceRight(yyvp[0].tree,yyvp[1].tree);
            break;
        /* Old operator-function-type reductions: actual CGOP values. */
        case 187: yyval.flags=63; break; /* operator new */
        case 188: yyval.flags=58; break; /* operator delete */
        case 189: yyval.flags=65; break; /* operator new[] */
        case 190: yyval.flags=60; break; /* operator delete[] */
        case 192: yyval.flags=1; break; /* operator - */
        case 193: yyval.flags=2; break; /* operator * */
        case 195: yyval.flags=4; break; /* operator % */
        case 196: yyval.flags=7; break; /* operator ^ */
        case 197: yyval.flags=5; break; /* operator & */
        case 198: yyval.flags=6; break; /* operator | */
        case 199: yyval.flags=11; break; /* operator ~ */
        case 200: yyval.flags=22; break; /* operator ! */
        case 201: yyval.flags=13; break; /* operator = */
        case 202: yyval.flags=27; break; /* operator *= */
        case 203: yyval.flags=28; break; /* operator /= */
        case 204: yyval.flags=29; break; /* operator %= */
        case 205: yyval.flags=25; break; /* operator += */
        case 206: yyval.flags=26; break; /* operator -= */
        case 207: yyval.flags=34; break; /* operator <<= */
        case 208: yyval.flags=33; break; /* operator >>= */
        case 209: yyval.flags=31; break; /* operator |= */
        case 210: yyval.flags=30; break; /* operator &= */
        case 211: yyval.flags=32; break; /* operator ^= */
        case 212: yyval.flags=9; break; /* operator << */
        case 213: yyval.flags=8; break; /* operator >> */
        case 214: yyval.flags=14; break; /* operator == */
        case 215: yyval.flags=15; break; /* operator != */
        case 216: yyval.flags=17; break; /* operator <= */
        case 217: yyval.flags=19; break; /* operator >= */
        case 218: yyval.flags=18; break; /* operator < */
        case 219: yyval.flags=16; break; /* operator > */
        case 220: yyval.flags=20; break; /* operator && */
        case 221: yyval.flags=21; break; /* operator || */
        case 222: yyval.flags=36; break; /* operator ++ */
        case 223: yyval.flags=37; break; /* operator -- */
        case 224: yyval.flags=42; break; /* operator , */
        case 225: yyval.flags=46; break; /* operator ->* */
        case 226: yyval.flags=23; break; /* operator -> */
        case 227: yyval.flags=47; break; /* operator () */
        case 228: yyval.flags=24; break; /* operator [] */
        case 229: yyval.flags=56; break;
        case 170: /* qualified destructor id */
            yyval.tree=MakeScopedDestructorId(yyvp[0].tree,yyvp[1].tree);
            break;
        case 172: /* qualified operator id */
            yyval.tree=MakeScopedOperatorId(yyvp[0].tree,(unsigned char)yyvp[1].flags);
            ScopeQualifyPop();
            break;
        case 182: /* operator-name : Y_OPERATOR operator-function-type */
            yyval.tree=setLocation(MakeOperatorId((unsigned char)yyvp[1].flags),&stack->lsp[1]);
            break;
        case 162: /* unqualified destructor-id: tilde followed by class id */
            yyval.tree=setLocation(MakeDestructorId(yyvp[1].tree),&stack->lsp[2]);
            break;
        case RULE_TEMPLATE_KEY: /* template-key : Y_TEMPLATE */
            if (stack->template_decl) CErr1(123);
            else if (!ScopeType(CurrScope,0)) CErr1(114);
            stack->template_decl=1;
            stack->template_args=1;
            /* This action never assigns its unused semantic value in the
             * original; do not reproduce the uninitialized ESI contents. */
            yyval.pointer=NULL;
            break;
        case RULE_PACKED_CLASS_EMPTY: /* packed-class-opt : empty */
            yyval.flags=0;
            break;
        /* These genuine old productions share the same null semantic
         * value. Known examples are the empty class modifier and optional
         * qualifier forms; unresolved production names remain numerical. */
        case 7:
        case 10:
        case 30:
        case 35:
        case 115:
        case 116:
        case 191:
        case 356:
        case 371:
        case 414:
        case 419:
        case 478:
        case 490:
        case 543:
            yyval.pointer=NULL;
            break;
        /* The compiler's shared default action copies the first RHS value.
         * These labels are the actual old productions entering that block. */
        case 6:
        case 11:
        case 14:
        case 27:
        case 31:
        case 32:
        case 36:
        case 37:
        case 39:
        case 41:
        case 43:
        case 45:
        case 47:
        case 49:
        case 51:
        case 53:
        case 55:
        case 57:
        case 59:
        case 61:
        case 64:
        case 67:
        case 71:
        case 76:
        case 79:
        case 82:
        case 86:
        case 89:
        case 91:
        case 99:
        case 106:
        case 107:
        case 113:
        case 137:
        case 139:
        case 140:
        case 150:
        case 155:
        case 156:
        case 157:
        case 158:
        case 159:
        case 160:
        case 161:
        case 166:
        case 167:
        case 230:
        case 231:
        case 232:
        case 234:
        case 235:
        case 236:
        case 237:
        case 238:
        case 239:
        case 240:
        case 241:
        case 242:
        case 243:
        case 254:
        case 256:
        case 257:
        case 260:
        case 261:
        case 262:
        case 272:
        case 273:
        case 274:
        case 275:
        case 281:
        case 282:
        case 297:
        case 303:
        case 311:
        case 312:
        case 315:
        case 324:
        case 332:
        case 333:
        case 364:
        case 376:
        case 377:
        case 379:
        case 380:
        case 381:
        case 386:
        case 415:
        case 420:
        case 448:
        case 456:
        case 457:
        case 479:
        case 480:
        case 482:
        case 484:
        case 485:
        case 491:
        case 492:
        case 493:
        case 494:
        case 516:
        case 524:
        case 527:
        case 530:
        case 535:
        case 536:
        case 544:
            yyval=yyvp[0];
            break;
        case 473: /* class-key : packed-class-opt Y_CLASS class-mod-opt */
            pushClassData(stack,0,(unsigned char)yyvp[0].flags,yyvp[2].type);
            yyval.pointer=NULL; /* original action leaves unused value unset */
            break;
        case 474: /* class-key : packed-class-opt Y_STRUCT class-mod-opt */
            pushClassData(stack,2,(unsigned char)yyvp[0].flags,yyvp[2].type);
            yyval.pointer=NULL;
            break;
        case 475: /* class-key : packed-class-opt Y_UNION class-mod-opt */
            pushClassData(stack,1,(unsigned char)yyvp[0].flags,yyvp[2].type);
            yyval.pointer=NULL;
            break;
        case 245: /* simple declaration awaiting its semicolon */
            yyval=yyvp[0];
            if (token!=Y_SEMI_COLON) {
                SetErrLoc(&yylocation);
                CErr1(6);
                what=9;
            }
            break;
        case 246: case 247: /* completed simple declaration */
            yyval=yyvp[0];
            CheckDeclarationDSpec(stack->gstack->u.dspec,CurrScope);
            GStackPop(&stack->gstack);
            break;
        /* These actual productions share the second-RHS copy at643a8.
         * 430 is the reached declarator form; unrecovered names stay numeric. */
        case 112: case 114: case 117: case 154: case 342: case 343:
        case 404: case 405: case 412: case 413: case 430: case 432:
        case 436: case 438: case 539: case 540:
            yyval=yyvp[1];
            break;
        case 263: /* storage-class-specifier : Y_AUTO */
            yyval.dspec=PTypeStgClass(4);
            break;
        case 264: /* storage-class-specifier : Y_REGISTER */
            yyval.dspec=PTypeStgClass(2);
            break;
        case 265: /* storage-class-specifier : Y_EXTERN */
            yyval.dspec=PTypeStgClass(8);
            break;
        case 266: /* storage-class-specifier : Y_STATIC */
            yyval.dspec=PTypeStgClass(16);
            break;
        case 267: /* storage-class-specifier : Y_TYPEDEF */
            yyval.dspec=PTypeStgClass(1);
            break;
        case 269: /* specifier : Y_INLINE */
            yyval.dspec=PTypeSpecifier(8);
            break;
        case 270: /* specifier : Y_VIRTUAL */
            yyval.dspec=PTypeSpecifier(16);
            break;
        case 271: /* specifier : Y_FRIEND */
            yyval.dspec=PTypeSpecifier(4);
            break;
        case 279: /* specifier : Y_CONST */
            yyval.dspec=PTypeSpecifier(1);
            break;
        case 280: /* specifier : Y_VOLATILE */
            yyval.dspec=PTypeSpecifier(2);
            break;
        case 283: /* simple-type-specifier : Y_CHAR */
            yyval.dspec=PTypeScalar(1);
            break;
        case 284: /* simple-type-specifier : Y_SHORT */
            yyval.dspec=PTypeScalar(128);
            break;
        case 285: /* simple-type-specifier : Y_INT */
            yyval.dspec=PTypeScalar(2);
            break;
        case 286: /* simple-type-specifier : Y_LONG */
            yyval.dspec=PTypeScalar(4);
            break;
        case 287: /* simple-type-specifier : Y_SIGNED */
            yyval.dspec=PTypeScalar(32);
            break;
        case 288: /* simple-type-specifier : Y_UNSIGNED */
            yyval.dspec=PTypeScalar(64);
            break;
        case 289: /* simple-type-specifier : Y_FLOAT */
            yyval.dspec=PTypeScalar(8);
            break;
        case 290: /* simple-type-specifier : Y_DOUBLE */
            yyval.dspec=PTypeScalar(16);
            break;
        case 291: /* simple-type-specifier : Y_VOID */
            yyval.dspec=PTypeScalar(256);
            break;
        case 292: /* simple-type-specifier : Y_SEGMENT */
            yyval.dspec=PTypeScalar(512);
            break;
        case 372: /* cv-qualifier : const */
            yyval.flags=1;
            break;
        case 374: /* cv-qualifier : volatile */
            yyval.flags=2;
            break;
        case 486: /* access-specifier ':' */
            ClassPermission(yyvp[0].flags);
            yyval.pointer=NULL;
            break;
        case 487: /* access-specifier ':' after member declarations */
            ClassPermission(yyvp[1].flags);
            yyval.pointer=NULL;
            break;
        case 495: /* member declaration awaiting its semicolon */
            if (token!=Y_SEMI_COLON) {
                SetErrLoc(&yylocation);
                what=9;
                CErr1(6);
            }
            yyval.pointer=NULL; /* unused original semantic value */
            break;
        case 497: /* member declaration without declaration specifiers */
            yyval.pointer=NULL; /* unused original semantic value */
            break;
        case 496: /* member declaration with declaration specifiers */
            GStackPop(&stack->gstack);
            yyval.pointer=NULL;
            break;
        case 499: /* decl-specifiers ctor-declarator */
            InsertDeclInfo(CurrScope,yyvp[1].dinfo);
            FreeDeclInfo(yyvp[1].dinfo);
            GStackPop(&stack->gstack);
            yyval.pointer=NULL; /* original semantic result is unused */
            break;
        case 500: /* first member-declarator */
            VerifyMemberFunction(stack->gstack->u.dspec,yyvp[0].dinfo);
            yyval.pointer=NULL;
            break;
        case 501: /* additional member-declarator */
            VerifyMemberFunction(stack->gstack->u.dspec,yyvp[2].dinfo);
            yyval.pointer=NULL;
            break;
        case 502: /* member-declarator : declarator '=' constant-expression */
            VerifyPureFunction(yyvp[0].dinfo,yyvp[2].tree);
            yyval.dinfo=InsertDeclInfo(CurrScope,yyvp[0].dinfo);
            break;
        case 503: /* member-declarator : declarator */
            yyval.dinfo=InsertDeclInfo(CurrScope,yyvp[0].dinfo);
            DeclNoInit(yyval.dinfo);
            break;
        case 431: /* argument declaration with a declarator and default */
            yyval.dinfo=yyvp[1].dinfo;
            yyval.dinfo->defarg_expr=yyvp[3].tree;
            break;
        case 515: /* constructor initializer recording */
            yyval.rewrite=RewritePackageMemInit(getMultiToken());
            what=0;
            break;
        case 440: /* ctor-declarator */
        {
            DECL_SPEC *spec=stack->gstack->u.dspec;
            PTREE id=MakeConstructorId(spec);
            if (!id) what=7;
            yyval.dinfo=MakeDeclarator(spec,id);
            if (yyval.dinfo->flags&8) what=4;
            else {
                AddDeclarator(yyval.dinfo,
                    MakeFnType(&yyvp[1].dinfo,(unsigned char)yyvp[3].flags,yyvp[4].tree));
                yyval.dinfo=AddExplicitParms(yyval.dinfo,yyvp[1].dinfo);
                yyval.dinfo=FinishDeclarator(spec,yyval.dinfo);
            }
            break;
        }
        case 461: /* start-class : empty */
            ClassStart();
            pushDefaultDeclSpec(stack);
            yyval.pointer=NULL;
            break;
        case 447: /* completed function-definition */
            GStackPop(&stack->gstack);
            if (yyvp[0].dinfo->flags&0x10) GStackPop(&stack->gstack);
            if (yyvp[0].dinfo->body) {
                if (ScopeType(CurrScope,4)) TemplateFunctionAttachDefn(yyvp[0].dinfo);
                else ClassStoreInlineFunc(yyvp[0].dinfo);
            } else FreeDeclInfo(yyvp[0].dinfo);
            yyval.pointer=NULL;
            break;
        case 451: /* simple initializer: init-start assignment-expression */
            DataInitSimple(yyvp[1].tree);
            GStackPop(&stack->gstack);
            yyval.pointer=NULL; /* unused original semantic value */
            break;
        case 453: /* enter data initializer from its declaring-declarator */
            {
                DECL_INFO *info=stack->gstack->u.dinfo;
                reuseGStack(stack,5);
                DataInitStart((struct data_init *)&stack->gstack->u,info);
                yyval.pointer=NULL; /* unused original semantic value */
            }
            break;
        case 449: /* check-for-rewrite : empty */
            setNoSuperTokens(stack);
            yyval.pointer=NULL;
            break;
        case 450: /* absorb-body : empty */
            ParseFlush();
            if (doFnbodyRewrite())
                stack->gstack->u.dinfo->body=RewritePackageFunction(getMultiToken());
            else FunctionBody(stack->gstack->u.dinfo);
            what=0;
            yyval.pointer=NULL;
            break;
        case 443: /* function declaration without decl-specifiers, ctor-init */
            yyvp[0].dinfo->mem_init=yyvp[1].rewrite;
            /* fall through */
        case 444:
            yyval.dinfo=DeclFunction(NULL,yyvp[0].dinfo);
            GStackPush(&stack->gstack,3);
            stack->gstack->u.dinfo=yyval.dinfo;
            break;
        case 441: case 445: /* function declaration with ctor initializer */
            yyvp[1].dinfo->mem_init=yyvp[2].rewrite;
            /* fall through */
        case 442: case 446:
            yyval.dinfo=DeclFunction(stack->gstack->u.dspec,yyvp[1].dinfo);
            GStackPush(&stack->gstack,3);
            stack->gstack->u.dinfo=yyval.dinfo;
            break;
        case 541: /* exception specification omitted */
            yyval.tree=ThrowsAnything();
            break;
        case 365: /* ptr-operator : '*' cv-qualifiers-opt */
        case 366: /* ptr-operator : '&' cv-qualifiers-opt */
            yyval.type=MakePointerType(rule==366,(unsigned char)yyvp[1].flags);
            break;
        case 321: /* declarator-list : init-declarator */
            tryCtorStyleInit(stack,yyvp[0].dinfo);
            yyval.pointer=NULL;
            break;
        case 322: /* declarator-list ',' comma-init-declarator */
            tryCtorStyleInit(stack,yyvp[2].dinfo);
            yyval.pointer=NULL;
            break;
        case 327: /* declaring-declarator : declarator */
            yyval.dinfo=InsertDeclInfo(CurrScope,yyvp[0].dinfo);
            GStackPush(&stack->gstack,3);
            stack->gstack->u.dinfo=yyval.dinfo;
            break;
        case 336: /* ptr-mod ptr-mod-declarator */
        case 337: /* ptr-mod actual-declarator */
            yyval.dinfo=AddDeclarator(yyvp[1].dinfo,yyvp[0].type);
            break;
        case 339: /* actual-declarator function suffix */
            yyval.dinfo=AddDeclarator(yyvp[0].dinfo,
                MakeFnType(&yyvp[2].dinfo,(unsigned char)yyvp[4].flags,yyvp[5].tree));
            yyval.dinfo=AddExplicitParms(yyval.dinfo,yyvp[2].dinfo);
            break;
        case 338: /* actual-declarator : declarator-id */
            yyval.dinfo=MakeDeclarator(stack->gstack->u.dspec,yyvp[0].tree);
            if (!yyval.dinfo) what=7;
            else if (yyval.dinfo->flags&8) what=4;
            break;
        case 328: case 329: case 388: case 389: case 390: case 391:
            yyval.dinfo=FinishDeclarator(stack->gstack->u.dspec,yyvp[0].dinfo);
            break;
        case 108: /* new placement type-id initializer */
            yyval.tree=setLocation(MakeNewExpr(NULL,yyvp[1].tree,
                yyvp[2].dinfo,yyvp[3].tree),&stack->lsp[1]);
            break;
        case 109: /* new type-id initializer */
            yyval.tree=setLocation(MakeNewExpr(NULL,NULL,
                yyvp[1].dinfo,yyvp[2].tree),&stack->lsp[1]);
            break;
        case 110: /* global-new placement type-id initializer */
            yyval.tree=setLocation(MakeNewExpr(yyvp[0].tree,yyvp[1].tree,
                yyvp[2].dinfo,yyvp[3].tree),&stack->lsp[1]);
            break;
        case 111: /* global-new type-id initializer */
            yyval.tree=setLocation(MakeNewExpr(yyvp[0].tree,NULL,
                yyvp[1].dinfo,yyvp[2].tree),&stack->lsp[1]);
            break;
        case 118: case 383: /* new-type-id with no explicit declarator */
            yyval.dinfo=MakeNewTypeId(DoDeclSpec(stack->gstack->u.dspec));
            GStackPop(&stack->gstack);
            break;
        case 119: /* checked new-type specifiers and completed declarator */
            yyval=yyvp[1];
            GStackPop(&stack->gstack);
            break;
        case 120: /* new declarator: pointer operators and array dimensions */
            yyval.dinfo=MakeNewDeclarator(stack->gstack->u.dspec,
                yyvp[0].dinfo,yyvp[1].dinfo);
            break;
        case 121: /* new declarator: pointer operators */
            yyval.dinfo=MakeNewDeclarator(stack->gstack->u.dspec,yyvp[0].dinfo,NULL);
            break;
        case 122: /* new declarator: array dimensions */
            yyval.dinfo=MakeNewDeclarator(stack->gstack->u.dspec,NULL,yyvp[0].dinfo);
            break;
        case 131: case 401: /* first new array dimension: '[' expression ']' */
            yyval.dinfo=MakeNewDynamicArray(yyvp[1].tree);
            break;
        case 278: /* type-specifier-seq checked for type-id */
            yyval.dspec=CheckTypeSpecifier(yyvp[0].dspec);
            if(yyval.dspec==NULL) what=7;
            break;
        case 293: case 294: case 296: /* qualified type specifier */
            yyval.dspec=sendType(yyvp[0].tree);
            break;
        case 295: /* template-qualified type specifier */
            yyval.dspec=sendType(yyvp[1].tree);
            PTypeRelease(yyvp[0].dspec);
            break;
        case 298: /* qualified-class-specifier : qualified-class-type */
            yyval=yyvp[0];
            if(!yyval.dspec) what=7;
            break;
        case 299: case 300: case 301: /* qualified class type */
            yyval.dspec=sendClass(yyvp[0].tree);
            break;
        case 521: /* completed template-declaration */
            RewriteFree(ParseGetRecordingInProgress(NULL));
            stack->template_decl=0;
            yyval.pointer=NULL;
            break;
        case 525: case 526: /* class-template/function-template scope closure */
            yyval=yyvp[0];
            GStackPop(&stack->gstack);
            GStackPop(&stack->gstack);
            break;
        case 248: /* declaration with no declarator */
            yyval.dspec=CheckNoDeclarator(yyvp[0].dspec);
            if (!yyval.dspec) what=7;
            break;
        case 523: /* template-argument-decl : template-abstract-args */
            stack->template_args=0;
            GStackPush(&stack->gstack,6);
            TemplateDeclInit((TEMPLATE_DATA *)&stack->gstack->u,yyvp[0].dinfo);
            pushDefaultDeclSpec(stack);
            stack->template_record_tokens=RewriteRecordInit(&stack->template_record_locn);
            yyval.pointer=NULL; /* action has no assigned semantic value */
            break;
        case 250: case 251: /* combined decl-specifier sequence */
            yyval.dspec=PTypeCombine(yyvp[0].dspec,yyvp[1].dspec);
            yyval.dspec=PTypeDone(yyval.dspec,token==Y_SEMI_COLON);
            pushDeclSpec(stack,yyval.dspec);
            break;
        case 252: case 253: /* single decl-specifier sequence */
            yyval.dspec=PTypeDone(yyvp[0].dspec,token==Y_SEMI_COLON);
            pushDeclSpec(stack,yyval.dspec);
            break;
        case 421: /* ordinary argument list, first parameter */
        case 423: /* first argument declaration */
            yyval.dinfo=AddArgument(NULL,yyvp[0].dinfo);
            break;
        case 422: /* ordinary argument list, further parameter */
        case 424: /* argument list, comma, argument declaration */
            yyval.dinfo=AddArgument(yyvp[0].dinfo,yyvp[2].dinfo);
            break;
        case 426: case 427: case 537: /* argument/catch declaration closes its spec scope */
            yyval=yyvp[0];
            GStackPop(&stack->gstack);
            break;
        case 428: case 434: /* abstract parameter from its decl-specifiers */
            yyval.dinfo=DeclSpecDeclarator(yyvp[0].dspec);
            break;
        case 425: /* checked argument declaration specifiers */
            yyval.dspec=CheckArgDSpec(yyvp[0].dspec);
            if (!yyval.dspec) what=7;
            break;
        case 463: case 465: /* class-specifier with a base clause */
            yyval=yyvp[3];
            break;
        case 464: case 466: /* class-specifier without a base clause */
            yyval=yyvp[2];
            break;
        case 462: /* class-specifier : class-key class-name */
            yyval.dspec=ClassRefDef();
            GStackPop(&stack->gstack);
            break;
        case 467: /* no-class-name : empty */
            ClassName(NULL,1);
            yyval.pointer=NULL;
            break;
        case 468: /* class-name : make-id */
            declaration=0; /* CLASS_REFERENCE */
            switch (token) {
            case Y_LEFT_BRACE: case Y_COLON:
                if (stack->template_class_inst_defer) { what=5; declaration=2; }
                else declaration=1; /* CLASS_DEFINITION */
                break;
            case Y_SEMI_COLON: declaration=2; break;
            case Y_COMMA: case Y_GT: case Y_EQUAL:
                if (stack->template_args) declaration=3; /* CLASS_GENERIC */
                break;
            }
            after_name=ClassName(yyvp[0].tree,declaration);
            if (after_name==1) what=0;
            else if (after_name==2) what=9;
            yyval.pointer=NULL; /* this action has no assigned semantic value */
            break;
        default: STUB_TRAP(); yyval.pointer=NULL; break;
        }
        *stack->vsp=yyval;
        (++stack->lsp)->src_file=NULL;
        if (what!=6) {
            if (!what) ParseFlush();
            return what;
        }
    }
}
void parserNextToken(void)
{
    NextToken();
    SrcFileGetTokenLocn(&yylocation);
}
void nextRecordedToken(PARSE_STACK *stack)
{
    parserNextToken();
    if (stack->template_record_tokens)
        RewriteRecordToken(stack->template_record_tokens,&stack->template_record_locn);
}
void *VstkPop(VSTK_CTL *stack);
void nextYYLexToken(PARSE_STACK *stack)
{
    currToken=3;
    if (stack->use_saved_tokens) {
        ++stack->look_ahead_index;
        if (stack->look_ahead_index!=stack->look_ahead_count) return;
        if (!stack->look_ahead_active) {
            while (stack->look_ahead_count) {
                VstkPop(&stack->look_ahead_storage);
                --stack->look_ahead_count;
            }
            stack->look_ahead_index=0;
        }
        stack->use_saved_tokens=0;
    }
    nextRecordedToken(stack);
}
void syntaxError(void) { STUB_TRAP(); }
void makeStable(unsigned token) { (void)token; STUB_TRAP(); }
void TemplateHandleClassMember(struct decl_info *info) { (void)info; STUB_TRAP(); }
SCOPE ScopeQualifyPop(void);
void VstkClose(VSTK_CTL *stack);
void *StackPop(void **head);
void deleteStack(PARSE_STACK *stack)
{
    if (stack->vstack) CarveFree(carveVALUE_STACK,stack->vstack);
    if (stack->sstack) CarveFree(carveSTATE_STACK,stack->sstack);
    if (stack->lstack) CarveFree(carveLOCATION_STACK,stack->lstack);
    while (stack->qualifications) ScopeQualifyPop();
    while (stack->gstack) GStackPop(&stack->gstack);
    VstkClose(&stack->look_ahead_storage);
    if (stack->template_record_tokens) RewriteFree(stack->template_record_tokens);
    CurrScope=stack->reset_scope;
    StackPop((void **)&currParseStack);
}
int tokenMakesPTREE(unsigned token)
{
    return token>=89 && token<=110;
}
void ParseFlush(void)
{
    if (tokenMakesPTREE(currToken)) {
        PTreeFreeSubtrees(yylval.tree);
        yylval.tree=NULL;
    }
    currToken=3;
}
void LinkageReset(void);
void TemplateProcessInstantiations(void);
void parseEpilogue(void)
{
    TemplateProcessInstantiations();
    CompFlags.parsing_finished=1;
}
void ParseDecls(void)
{
    PARSE_STACK state;
    unsigned char what;
    syncLocation();
    for (;;) {
        if (CurToken==1) break;
        newDeclStack(&state);
        pushDefaultDeclSpec(&state);
        for (;;) {
            for (;;) {
                do { what=doAction(yylex(&state),&state); } while (!what);
                if (what!=1) break;
                nextYYLexToken(&state);
            }
            if (what>3) {
                if (what>6) {
                    if (what==7) syntaxError();
                    else if (what==8) CErr1(170);
                    makeStable(5);
                } else if (what==4) {
                    TemplateHandleClassMember(state.vsp->dinfo);
                    deleteStack(&state);
                    ParseFlush();
                    break;
                }
            }
            deleteStack(&state);
            LinkageReset();
            break;
        }
    }
    parseEpilogue();
}

REWRITE *ParseGetRecordingInProgress(CGIO_LOCATION **location)
{
    PARSE_STACK *stack=currParseStack;
    REWRITE *rewrite=stack->template_record_tokens;
    stack->template_record_tokens=NULL;
    if (location) *location=&stack->template_record_locn;
    return rewrite;
}

void *ParseCurrQualification(void)
{ return currParseStack!=NULL ? currParseStack->qualifications : NULL; }

void newExprStack(PARSE_STACK *stack,unsigned token)
{
    initParseStacks(stack,0);
    commonInit(stack);
    doAction(token,stack);
}
PTREE ParseMemInit(void)
{
    PARSE_STACK state;
    unsigned char what;
    PTREE tree;
    newExprStack(&state,114); /* actual old Y_MEM_INIT_SPECIAL */
    syncLocation();
    for (;;) {
        do { what=doAction(yylex(&state),&state); } while (!what);
        if (what!=1) break;
        nextYYLexToken(&state);
    }
    if (what>3) {
        if (what>6) {
            if (what==7) syntaxError();
            else if (what==8) CErr1(124);
            makeStable(11);
            tree=NULL;
        } else {
            /* The original leaves its result register undefined on this
             * invalid action status. This semantic domain is not supported. */
            STUB_TRAP();
            tree=NULL;
        }
    } else tree=state.vsp->tree;
    deleteStack(&state);
    return tree;
}

PTREE ParseExprDecl(void)
{
    PARSE_STACK state;
    unsigned char what;
    PTREE tree=NULL;
    newExprStack(&state,112);
    syncLocation();
    for(;;) {
        do { what=doAction(yylex(&state),&state); } while(!what);
        if(what!=1) break;
        nextYYLexToken(&state);
    }
    if(what>3) {
        if(what>6) {
            if(what==7) syntaxError();
            else if(what==8) CErr1(184);
            makeStable(5);
        }
    } else tree=state.vsp->tree;
    deleteStack(&state);
    return tree;
}

PTREE ParseExpr(int end_token)
{
    PARSE_STACK state;
    unsigned char what;
    PTREE tree;
    newExprStack(&state,111); /* actual old Y_EXPRESSION_SPECIAL */
    syncLocation();
    for (;;) {
        do { what=doAction(yylex(&state),&state); } while (!what);
        if (what!=1) break;
        nextYYLexToken(&state);
    }
    if (what>3) {
        if (what>6) {
            if (what==7) syntaxError();
            else if (what==8) CErr1(124);
            makeStable(end_token);
            tree=NULL;
        } else {
            /* Original result register is undefined for these invalid actions. */
            STUB_TRAP();
            tree=NULL;
        }
    } else tree=state.vsp->tree;
    deleteStack(&state);
    return tree;
}

void ParsePushQualification(void *qual)
{ StackPush(&currParseStack->qualifications,qual); }
void *ParsePopQualification(void)
{ return StackPop(&currParseStack->qualifications); }

void newClassInstStack(PARSE_STACK *stack)
{
    initParseStacks(stack,0);
    commonInit(stack);
    doAction(115,stack);
}
void CErrCheckpoint(unsigned *state);
int CErrOccurred(unsigned *state);
REWRITE *RewritePackageToken(void);
void RewriteRestoreToken(REWRITE *rewrite);
void SrcFileResetTokenLocn(CGIO_LOCATION *location);
/* Actual1994 CF6586c..659a0, before the later NULL-definition check and
 * C++ linkage push/pop. The stored rewrite supplies the nested lexer. */
DECL_SPEC *ParseClassInstantiation(REWRITE *defn,int no_definition)
{
    PARSE_STACK state;
    unsigned char what;
    DECL_SPEC *type=NULL;
    REWRITE *saved_token,*last_rewrite;
    TOKEN_SOURCE saved_source;
    CGIO_LOCATION locn;
    unsigned check;
    CErrCheckpoint(&check);
    saved_token=RewritePackageToken();
    SrcFileGetTokenLocn(&locn);
    ParseFlush();
    newClassInstStack(&state);
    if(no_definition) state.template_class_inst_defer=1;
    syncLocation();
    pushDefaultDeclSpec(&state);
    saved_source=SetTokenSource(RewriteToken);
    last_rewrite=RewriteRewind(defn);
    for(;;) {
        do { what=doAction(yylex(&state),&state); } while(what==0);
        if(what!=1) break;
        nextYYLexToken(&state);
    }
    RewriteClose(last_rewrite);
    ResetTokenSource(saved_source);
    if(what>3) {
        if(what>6) {
            if(what==7) syntaxError();
            else if(what==8) CErr1(170);
        } else if(what==5) {
            ParseFlush();
            do { what=doAction(43,&state); } while(what==0);
            type=state.vsp->dspec;
        }
    } else if(state.vsp->dspec!=NULL) type=state.vsp->dspec;
    deleteStack(&state);
    SrcFileResetTokenLocn(&locn);
    RewriteRestoreToken(saved_token);
    ParseFlush();
    if(type!=NULL) {
        if(CErrOccurred(&check)) { PTypeRelease(type); type=NULL; }
        else type=PTypeDone(type,1);
    }
    return type;
}

void newExceptionStack(PARSE_STACK *stack)
{
    initParseStacks(stack,0);
    commonInit(stack);
    doAction(113,stack);
}
DECL_INFO *ParseException(void)
{
    PARSE_STACK state;
    unsigned char what;
    DECL_INFO *exception=NULL;
    unsigned check;
    CErrCheckpoint(&check);
    newExceptionStack(&state);
    syncLocation();
    pushDefaultDeclSpec(&state);
    for(;;) {
        do { what=doAction(yylex(&state),&state); } while(!what);
        if(what!=1) break;
        nextYYLexToken(&state);
    }
    if(what>3) {
        if(what>6) {
            if(what==7) syntaxError();
            else if(what==8) CErr1(171);
            makeStable(8);
        }
    } else if(state.vsp->dinfo) exception=state.vsp->dinfo;
    deleteStack(&state);
    if(exception && CErrOccurred(&check)) {
        FreeDeclInfo(exception);
        exception=NULL;
    }
    return exception;
}

void ParseFunctionInstantiation(REWRITE *defn)
{
    PARSE_STACK state;
    unsigned char what;
    REWRITE *saved_rewrite;
    TOKEN_SOURCE saved_source;
    CGIO_LOCATION locn;
    /* Actual1994 has neither the later NULL guard nor linkage push/pop. */
    SrcFileGetTokenLocn(&locn);
    ParseFlush();
    newDeclStack(&state);
    syncLocation();
    pushDefaultDeclSpec(&state);
    saved_source=SetTokenSource(RewriteToken);
    saved_rewrite=RewriteRewind(defn);
    for(;;) {
        do { what=doAction(yylex(&state),&state); } while(!what);
        if(what!=1) break;
        nextYYLexToken(&state);
    }
    RewriteClose(saved_rewrite);
    ResetTokenSource(saved_source);
    if(what>3 && what>6) {
        if(what==7) syntaxError();
        else if(what==8) CErr1(170);
    }
    deleteStack(&state);
    SrcFileResetTokenLocn(&locn);
    CurToken=1;
    strcpy(Buffer,Tokens[1]);
}
