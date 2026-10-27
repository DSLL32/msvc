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


/* Adapted from initial6b9cb443 bld/plusplus/c/ptree.c.
 * Actual1994 CF44b20..44b4b initializes28-byte parse-tree nodes and
 * a16-byte symbol-location pool (bdb04), later initialized in symlocn.c.
 * Its12-byte source position and dwarf handle are consumed by SymbolLocnDefine.
 * These are real allocator controls, not prebuilt record contents. */
#include "compiler.h"
#include "ptree.h"
#include "carve.h"
#include "pending.h"
#include "symlocn.h"
#include "symbol.h"
#include "stringl.h"
#include <recon.h>
/* Recovered descriptive name: field-name grammar reductions mark byte24,
 * independently of an operator identifier's opcode in byte25. */
void markFieldName(PTREE id) { id->u.id.field_name|=1; }
PTREE PTreeReplaceLeft(PTREE tree,PTREE left)
{ tree->u.subtree[0]=left; return tree; }
PTREE PTreeReplaceRight(PTREE tree,PTREE right)
{ tree->u.subtree[1]=right; return tree; }
static carve_t carvePTREE, carveSYMBOL_LOCN;
void ptreeInit(INITFINI *defn)
{
    (void)defn;
    carvePTREE = CarveCreate(28, 32);
    carveSYMBOL_LOCN = CarveCreate(16, 256);
}


PTREE PTreeId(char *id)
{
    PTREE tree=PTreeAlloc();
    tree->op=6;
    tree->cgop=194; /* old CO_NAME_NORMAL */
    tree->u.id.field_name=0;
    tree->u.id.name=id;
    return tree;
}

PTREE PTreeAlloc(void)
{
    PTREE tree;
    tree=CarveAlloc(carvePTREE);
    tree->op=0;
    tree->cgop=203; /* actual old CO_NOP */
    tree->flags=0;
    tree->type=0;
    tree->locn.src_file=0;
    tree->locn.line=0;
    tree->locn.column=0;
    return tree;
}
void ptreeSetConstantType(PTREE tree,type_id id)
{
    /* The old constant stores its type-id in cgop, not a later flags bit. */
    tree->cgop=id;
    tree->type=GetBasicType(id);
}
PTREE PTreeIntConstant(long value,type_id id)
{
    PTREE tree;
    tree=PTreeAlloc();
    ptreeSetConstantType(tree,id);
    tree->op=2;
    tree->u.int_constant=value;
    return tree;
}

PTREE PTreeFree(PTREE tree)
{
    if (tree) CarveFree(carvePTREE,tree);
    return 0;
}
void SymbolLocnFree(SYM_TOKEN_LOCN *location)
{ CarveFree(carveSYMBOL_LOCN,location); }
SYM_TOKEN_LOCN *SymbolLocnAlloc(void)
{
    SYM_TOKEN_LOCN *locn=CarveAlloc(carveSYMBOL_LOCN);
    locn->dwh=0;
    return locn;
}

PTREE PTreeTraversePostfix(PTREE tree,PTREE (*visit_rtn)(PTREE));
void PTreeFreeSubtrees(PTREE tree)
{
    PTreeTraversePostfix(tree,PTreeFree);
}
/* Encountered1994 traversal categories at B786c: op8 unary,9 binary,
 * 10 duplicated expression. Kinds0..7 are leaves. */
enum { PTS_UNARY=1,PTS_BINARY=2,PTS_HAS_CGOP=4,PTS_OPERATOR=3 };
static unsigned char ptreePTSFlags[]={
    0,0,0,0,0,0,0,0,PTS_UNARY|PTS_HAS_CGOP,
    PTS_BINARY|PTS_HAS_CGOP,PTS_UNARY
};
PTREE PTreeTraversePostfix(     // TRAVERSE A PTREE IN (LEFT,RIGHT,SELF) ORDER
    PTREE tree,                 // - tree to be traversed
    PTREE (*visit_rtn)          // - visiting routine
        ( PTREE curr ) )        // - - addr( current node )
{
    PTREE parent;
    PTREE temp;
    PTREE left;
    PTREE right;

    /*
        The states represent locations in the traditional recursive version
        of a postfix traversal function.

        postfix( tree )
        {
                                        PTREE_LEFT
            postfix( tree->left )
                                        PTREE_RIGHT
            postfix( tree->right )
                                        PTREE_PROCESS
            process( tree )
                                        PTREE_UNWIND
        }

        note: this routine is heavily used so it has been optimized
        to use gotos since the current codegen can't optimize an
        assignment of a constant to a variable and an immediate
        switch statement to a simple goto.  The switch() style has
        been preserved for less heavily used traversal routines.
    */
    if( tree == NULL || tree->op == 0 ) {
        return tree;
    }
    parent = NULL;
    PTREE_LEFT:
        if( tree->flags & 0x4000 ) {
            goto PTREE_UNWIND;
        } else if( ptreePTSFlags[ tree->op ] & PTS_OPERATOR ) {
            left = tree->u.subtree[0];
            if( left != NULL ) {
                /* has a left subtree; store current parent */
                temp = tree;
                tree = left;
                temp->u.subtree[0] = parent;
                temp->flags |= 0x8000;
                parent = temp;

                /* descend to left child node */
                goto PTREE_LEFT;
            }
            /* descend to right child node */
            goto PTREE_RIGHT;
        } else {
            /* no subtrees; process this node */
            goto PTREE_PROCESS;
        }
    PTREE_RIGHT:
        if( ptreePTSFlags[ tree->op ] & PTS_BINARY ) {
            right = tree->u.subtree[1];
            if( right != NULL ) {
                /* has a right subtree; store current parent */
                temp = tree;
                tree = right;
                temp->u.subtree[1] = parent;
                temp->flags &= ~0x8000;
                parent = temp;

                /* descend to right child node */
                goto PTREE_LEFT;
            } else {
                /* no right subtree; process this node */
                goto PTREE_PROCESS;
            }
        } else {
            /* no right subtree; process this node */
            goto PTREE_PROCESS;
        }
    PTREE_PROCESS:
        /* process node; may replace parent's pointer to this node */
        tree = (*visit_rtn)( tree );
        goto PTREE_UNWIND;
    PTREE_UNWIND:
        if( parent == NULL ) {
            return tree;
        }
        if( parent->flags & 0x8000 ) {
            parent->flags &= ~0x8000;
            /* set child pointer of parent to current subtree */
            temp = parent;
            parent = temp->u.subtree[0];
            temp->u.subtree[0] = tree;
            tree = temp;

            /* processed a left child; try for right hand child of parent */
            goto PTREE_RIGHT;
        } else {
            /* set child pointer of parent to current subtree */
            temp = parent;
            parent = temp->u.subtree[1];
            temp->u.subtree[1] = tree;
            tree = temp;

            /* processed a right child; process parent */
            goto PTREE_PROCESS;
        }
}



PTREE PTreeTListAppend(PTREE start,PTREE tree)
{
    tree->u.subtree[0]=start;
    return tree;
}
PTREE PTreeType(TYPE type)
{
    PTREE tree=PTreeAlloc();
    tree->op=5;
    tree->u.subtree[0]=NULL;
    tree->type=type;
    return tree;
}
PTREE ThrowsAnything(void)
{
    return PTreeTListAppend(NULL,PTreeType(GetBasicType(TYP_DOT_DOT_DOT)));
}

PTREE PTreeBinary(unsigned char op,PTREE left,PTREE right);
PTREE PTreeCopySrcLocation(PTREE target,PTREE source);
PTREE useScopeIfPossible(PTREE scoped,PTREE id)
{
    PTREE colon=scoped->u.subtree[0],old=colon->u.subtree[1],node;
    TYPE type;
    if(old!=NULL) {
        type=StructType(old->type);
        if(type!=NULL) {
            node=PTreeType(type);
            PTreeCopySrcLocation(node,old);
            id=PTreeBinary(55,node,id);
            PTreeCopySrcLocation(id,colon);
        }
    }
    return id;
}
/* The1994 CheckScopedId owns MakeScopedId's later factored-out body. */
PTREE CheckScopedId(PTREE tree)
{
    PTREE id;
    if (tree->op!=9) return tree;
    id=tree->u.subtree[1];
    tree->u.subtree[1]=NULL;
    id=useScopeIfPossible(tree,id);
    PTreeFreeSubtrees(tree);
    return id;
}

PTREE PTreeCopySrcLocation(PTREE target,PTREE source)
{
    if(target && source && source->locn.src_file) target->locn=source->locn;
    return target;
}

PTREE makeExpr(unsigned char op,unsigned char cgop,PTREE left,PTREE right)
{
    PTREE node=PTreeAlloc();
    node->type=NULL;
    node->cgop=cgop;
    node->u.subtree[0]=left;
    node->u.subtree[1]=right;
    node->op=op;
    return node;
}
PTREE PTreeBinary(unsigned char cgop,PTREE left,PTREE right)
{
    return makeExpr(9,cgop,left,right);
}
unsigned short PTreeEffFlags(PTREE tree)
{
    return tree ? (tree->flags&0x11) : 0;
}

PTREE *PTreeRef(PTREE *target)
{
    PTREE node;
    for(;;) {
        node=*target;
        if(!node || node->op!=9 || node->cgop!=42) break;
        target=&node->u.subtree[1];
    }
    return target;
}

PTREE PTreeErrorNode(PTREE tree);
int PTreeIsError(PTREE tree) { return tree && tree->op==0; }
int PTreePropogateError(PTREE tree)
{
    int error;
    if(PTreeIsError(tree)) error=1;
    else if(tree->op==9 && (PTreeIsError(tree->u.subtree[1]) ||
                           PTreeIsError(tree->u.subtree[0]))) error=1;
    else if(tree->op==8 && PTreeIsError(tree->u.subtree[0])) error=1;
    else error=0;
    if(error) PTreeErrorNode(tree);
    return error;
}
PTREE *PTreeRefLeft(PTREE tree) { return PTreeRef(&tree->u.subtree[0]); }

PTREE PTreeOpLeft(PTREE tree);
PTREE PTreeOp(PTREE *target)
{
    PTREE tree=*PTreeRef(target);
    if(tree && tree->op==10) return PTreeOpLeft(tree);
    return tree;
}
/* The original named left wrapper is also the shared continuation of the
 * duplicated-expression loop. Keep the two genuine entries and tail edges. */
PTREE PTreeOpLeft(PTREE tree) { return PTreeOp(&tree->u.subtree[0]); }

PTREE PTreeOpRight(PTREE tree) { return PTreeOp(&tree->u.subtree[1]); }

/* Encountered operator-flag table at DATA B7879..B796b:121 unsigned16 rows.
 * The initial source ppopsdef.h/ppopscnv.h supplies these named flag and
 * conversion roles. Its later operator reorder is not imported. Comments
 * identify known1994 operators; remaining meanings retain honest ordinals.
 * Only this complete table is represented, not its neighboring objects. */
enum { PTO_NULL=0,PTO_UNARY=1,PTO_BINARY=2,PTO_BOOLEAN=4,PTO_OVLOAD=8 };
enum {
    OPCNV_PLUS=0x10,
    OPCNV_MINUS=0x20,
    OPCNV_ONE_AR=0x30,
    OPCNV_ONE_RAP=0x40,
    OPCNV_POST_OP=0x50,
    OPCNV_TWO_AR=0x60,
    OPCNV_ONE_INT=0x70,
    OPCNV_TWO_INT=0x80,
    OPCNV_SHIFT=0x90,
    OPCNV_RELEQ=0xa0,
    OPCNV_COLON=0xb0,
    OPCNV_RELN=0xc0,
    OPCNV_LOG1=0xd0,
    OPCNV_LOG2=0xe0,
    OPCNV_SUBSCR=0xf0,
    OPCNV_PTR=0x100,
    OPCNV_FUNCTION=0x110,
    OPCNV_EQAP=0x120,
    OPCNV_EQAR=0x130,
    OPCNV_EQINT=0x140,
    OPCNV_QUESTION=0x150,
    OPCNV_ARROW=0x160,
    OPCNV_EQUALS=0x170,
    OPCNV_ARROW_STAR=0x180
};
static unsigned short oper_flags[121]={
    PTO_BINARY | PTO_OVLOAD | OPCNV_PLUS, /* 0: PLUS */
    PTO_BINARY | PTO_OVLOAD | OPCNV_MINUS, /* 1: MINUS */
    PTO_BINARY | PTO_OVLOAD | OPCNV_TWO_AR, /* 2: TIMES */
    PTO_BINARY | PTO_OVLOAD | OPCNV_TWO_AR, /* 3: DIVIDE */
    PTO_BINARY | PTO_OVLOAD | OPCNV_TWO_INT, /* 4: PERCENT */
    PTO_BINARY | PTO_OVLOAD | OPCNV_TWO_INT, /* 5: AND */
    PTO_BINARY | PTO_OVLOAD | OPCNV_TWO_INT, /* 6: OR */
    PTO_BINARY | PTO_OVLOAD | OPCNV_TWO_INT, /* 7: XOR */
    PTO_BINARY | PTO_OVLOAD | OPCNV_SHIFT, /* 8: RSHIFT */
    PTO_BINARY | PTO_OVLOAD | OPCNV_SHIFT, /* 9: LSHIFT */
    PTO_UNARY | PTO_OVLOAD | OPCNV_ONE_AR, /* 10: NEGATE */
    PTO_UNARY | PTO_OVLOAD | OPCNV_ONE_INT, /* 11: COMPLEMENT */
    PTO_UNARY | PTO_OVLOAD, /* 12: ADDR_OF */
    PTO_BINARY | PTO_OVLOAD | OPCNV_EQUALS, /* 13: EQUAL */
    PTO_BINARY | PTO_BOOLEAN | PTO_OVLOAD | OPCNV_RELEQ, /* 14: EQ */
    PTO_BINARY | PTO_BOOLEAN | PTO_OVLOAD | OPCNV_RELEQ, /* 15: NE */
    PTO_BINARY | PTO_BOOLEAN | PTO_OVLOAD | OPCNV_RELN, /* 16: GT */
    PTO_BINARY | PTO_BOOLEAN | PTO_OVLOAD | OPCNV_RELN, /* 17: LE */
    PTO_BINARY | PTO_BOOLEAN | PTO_OVLOAD | OPCNV_RELN, /* 18: LT */
    PTO_BINARY | PTO_BOOLEAN | PTO_OVLOAD | OPCNV_RELN, /* 19: GE */
    PTO_BINARY | PTO_BOOLEAN | PTO_OVLOAD | OPCNV_LOG2, /* 20: AND_AND */
    PTO_BINARY | PTO_BOOLEAN | PTO_OVLOAD | OPCNV_LOG2, /* 21: OR_OR */
    PTO_UNARY | PTO_BOOLEAN | PTO_OVLOAD | OPCNV_LOG1, /* 22: NOT */
    PTO_BINARY | PTO_OVLOAD | OPCNV_ARROW, /* 23: ARROW */
    PTO_BINARY | PTO_OVLOAD | OPCNV_SUBSCR, /* 24: SUBSCRIPT */
    PTO_BINARY | PTO_OVLOAD | OPCNV_EQAP, /* 25: EQUAL_PLUS */
    PTO_BINARY | PTO_OVLOAD | OPCNV_EQAP, /* 26: EQUAL_MINUS */
    PTO_BINARY | PTO_OVLOAD | OPCNV_EQAR, /* 27: EQUAL_TIMES */
    PTO_BINARY | PTO_OVLOAD | OPCNV_EQAR, /* 28: EQUAL_DIVIDE */
    PTO_BINARY | PTO_OVLOAD | OPCNV_EQINT, /* 29: EQUAL_PERCENT */
    PTO_BINARY | PTO_OVLOAD | OPCNV_EQINT, /* 30: EQUAL_AND */
    PTO_BINARY | PTO_OVLOAD | OPCNV_EQINT, /* 31: EQUAL_OR */
    PTO_BINARY | PTO_OVLOAD | OPCNV_EQINT, /* 32: EQUAL_XOR */
    PTO_BINARY | PTO_OVLOAD | OPCNV_EQINT, /* 33: EQUAL_RSHIFT */
    PTO_BINARY | PTO_OVLOAD | OPCNV_EQINT, /* 34: EQUAL_LSHIFT */
    PTO_UNARY | PTO_OVLOAD | OPCNV_ONE_AR, /* 35: UPLUS */
    PTO_UNARY | PTO_OVLOAD | OPCNV_ONE_RAP, /* 36: PRE_PLUS_PLUS */
    PTO_UNARY | PTO_OVLOAD | OPCNV_ONE_RAP, /* 37: PRE_MINUS_MINUS */
    PTO_UNARY | PTO_OVLOAD | OPCNV_POST_OP, /* 38: POST_PLUS_PLUS */
    PTO_UNARY | PTO_OVLOAD | OPCNV_POST_OP, /* 39: POST_MINUS_MINUS */
    PTO_UNARY | PTO_OVLOAD | OPCNV_PTR, /* 40: INDIRECT */
    PTO_BINARY | OPCNV_QUESTION, /* 41: old operator 41 */
    PTO_BINARY | PTO_OVLOAD, /* 42: COMMA */
    PTO_BINARY | OPCNV_COLON, /* 43: QUESTION */
    PTO_BINARY, /* 44: COLON */
    PTO_BINARY, /* 45: DOT_STAR */
    PTO_BINARY | PTO_OVLOAD | OPCNV_ARROW_STAR, /* 46: ARROW_STAR */
    PTO_BINARY | PTO_OVLOAD | OPCNV_FUNCTION, /* 47: FUNCTION */
    PTO_BINARY, /* 48: DOT */
    PTO_UNARY | OPCNV_ONE_INT, /* 49: old operator 49 */
    PTO_UNARY | PTO_OVLOAD, /* 50: old operator 50 */
    PTO_UNARY | PTO_OVLOAD, /* 51: old operator 51 */
    PTO_BINARY | PTO_OVLOAD, /* 52: old operator 52 */
    PTO_BINARY, /* 53: STORAGE */
    PTO_BINARY, /* 54: OPERATOR */
    PTO_BINARY, /* 55: COLON_COLON */
    PTO_BINARY, /* 56: old operator 56 */
    PTO_BINARY, /* 57: CONVERT */
    PTO_BINARY, /* 58: DELETE */
    PTO_BINARY, /* 59: DELETE_G */
    PTO_BINARY, /* 60: DELETE_ARRAY */
    PTO_BINARY, /* 61: DELETE_ARRAY_G */
    PTO_BINARY, /* 62: CTOR */
    PTO_BINARY, /* 63: NEW */
    PTO_BINARY, /* 64: NEW_G */
    PTO_BINARY, /* 65: NEW_ARRAY */
    PTO_BINARY, /* 66: old operator 66 */
    PTO_BINARY, /* 67: old operator 67 */
    PTO_UNARY, /* 68: old operator 68 */
    PTO_BINARY, /* 69: RETURN */
    PTO_BINARY, /* 70: old operator 70 */
    PTO_UNARY, /* 71: old operator 71 */
    PTO_BINARY, /* 72: old operator 72 */
    PTO_UNARY | PTO_OVLOAD, /* 73: old operator 73 */
    PTO_UNARY | PTO_OVLOAD, /* 74: old operator 74 */
    PTO_UNARY | PTO_OVLOAD, /* 75: old operator 75 */
    PTO_UNARY | PTO_OVLOAD, /* 76: old operator 76 */
    PTO_UNARY | PTO_OVLOAD, /* 77: old operator 77 */
    PTO_UNARY | PTO_OVLOAD, /* 78: old operator 78 */
    PTO_UNARY | PTO_OVLOAD, /* 79: old operator 79 */
    PTO_UNARY | PTO_OVLOAD, /* 80: old operator 80 */
    PTO_UNARY | PTO_OVLOAD, /* 81: old operator 81 */
    PTO_UNARY | PTO_OVLOAD, /* 82: old operator 82 */
    PTO_UNARY | PTO_OVLOAD, /* 83: old operator 83 */
    PTO_UNARY | PTO_OVLOAD, /* 84: old operator 84 */
    PTO_UNARY | PTO_OVLOAD, /* 85: old operator 85 */
    PTO_UNARY | PTO_OVLOAD, /* 86: old operator 86 */
    PTO_BINARY | PTO_OVLOAD, /* 87: old operator 87 */
    PTO_BINARY | PTO_OVLOAD, /* 88: old operator 88 */
    PTO_BINARY | PTO_OVLOAD, /* 89: old operator 89 */
    PTO_UNARY | PTO_OVLOAD, /* 90: old operator 90 */
    PTO_NULL, /* 91: old operator 91 */
    PTO_UNARY, /* 92: old operator 92 */
    PTO_UNARY, /* 93: old operator 93 */
    PTO_UNARY | PTO_OVLOAD, /* 94: old operator 94 */
    PTO_BINARY, /* 95: old operator 95 */
    PTO_UNARY, /* 96: old operator 96 */
    PTO_BINARY, /* 97: old operator 97 */
    PTO_UNARY, /* 98: old operator 98 */
    PTO_BINARY, /* 99: DTOR */
    PTO_BINARY, /* 100: old operator 100 */
    PTO_UNARY, /* 101: old operator 101 */
    PTO_BINARY, /* 102: old operator 102 */
    PTO_BINARY, /* 103: old operator 103 */
    PTO_UNARY, /* 104: old operator 104 */
    PTO_BINARY | PTO_OVLOAD, /* 105: old operator 105 */
    PTO_BINARY | PTO_OVLOAD, /* 106: old operator 106 */
    PTO_BINARY | PTO_OVLOAD, /* 107: old operator 107 */
    PTO_BINARY | PTO_OVLOAD, /* 108: old operator 108 */
    PTO_UNARY, /* 109: old operator 109 */
    PTO_BINARY, /* 110: old operator 110 */
    PTO_UNARY, /* 111: old operator 111 */
    PTO_UNARY, /* 112: old operator 112 */
    PTO_UNARY, /* 113: old operator 113 */
    PTO_UNARY, /* 114: old operator 114 */
    PTO_UNARY, /* 115: old operator 115 */
    PTO_UNARY, /* 116: old operator 116 */
    PTO_UNARY, /* 117: old operator 117 */
    PTO_UNARY, /* 118: old operator 118 */
    PTO_BINARY, /* 119: old operator 119 */
    PTO_BINARY, /* 120: old operator 120 */
};
unsigned short PTreeOpFlags(PTREE tree)
{
    unsigned short flags=PTO_NULL;
    if(ptreePTSFlags[tree->op]&PTS_HAS_CGOP) flags=oper_flags[tree->cgop];
    return flags;
}

PTREE *PTreeRefRight(PTREE node)
{
    return PTreeRef(&node->u.subtree[1]);
}

PTREE PTreeUnary(unsigned char op,PTREE operand)
{
    return makeExpr(8,op,operand,NULL);
}

extern TYPE TypeError;
char *CppDestructorName(void);
TYPE mustBeLexicalTypeName(SCOPE scope,char *name)
{
    SEARCH_RESULT *result=ScopeFindNaked(scope,name);
    SYMBOL sym;
    TYPE type;
    if(result==NULL) { CErr2p(252,name); return TypeError; }
    sym=result->sym_name->name_type;
    if(sym==NULL) {
        ScopeFreeResult(result);
        result=ScopeFindLexicalClassType(scope,name);
        if(result==NULL) { CErr2p(252,name); return TypeError; }
        sym=result->sym_name->name_type;
    }
    type=sym->sym_type;
    ScopeFreeResult(result);
    return type;
}
PTREE SimpleDestructorId(TYPE type)
{
    PTREE id=PTreeId(CppDestructorName());
    id->cgop=197;
    id->type=type;
    return id;
}
PTREE makeDestructorId(SCOPE scope,PTREE id,TYPE class_type)
{
    char *name=id->u.id.name;
    PTreeFree(id);
    if(class_type==NULL) class_type=mustBeLexicalTypeName(scope,name);
    return SimpleDestructorId(class_type);
}
PTREE MakeDestructorId(PTREE id)
{ return makeDestructorId(CurrScope,id,NULL); }

PTREE DoDeclSpec(DECL_SPEC *spec);
void PTypeRelease(DECL_SPEC *spec);
PTREE MakeMemInitItem(DECL_SPEC *spec,PTREE id,PTREE expressions)
{
    if (!id) {
        id=DoDeclSpec(spec);
        PTypeRelease(spec);
    }
    return PTreeBinary(62,id,expressions);
}

void PTreeErrorExprSym(PTREE expr,unsigned error,SYMBOL sym)
{ (void)expr; (void)error; (void)sym; STUB_TRAP(); }

char *CppSpecialName(unsigned name);
PTREE PTreeThis(void)
{
    PTREE tree=PTreeId(CppSpecialName(0));
    tree->cgop=196;
    return tree;
}

static CGIO_LOCATION *extract_locn;
PTREE set_error_location(PTREE tree)
{
    if(tree->locn.src_file) extract_locn=&tree->locn;
    return tree;
}
PTREE PTreeExtractLocn(PTREE expr,CGIO_LOCATION *location)
{
    if(expr && expr->locn.src_file) {
        *location=expr->locn;
        return expr;
    }
    extract_locn=NULL;
    expr=PTreeTraversePostfix(expr,set_error_location);
    if(extract_locn) *location=*extract_locn;
    else memset(location,0,sizeof(*location));
    return expr;
}

PTREE PTreeTraversePrefix(PTREE tree,PTREE (*visit)(PTREE))
{
    enum { PREFIX_LEFT,PREFIX_RIGHT,PREFIX_PROCESS,PREFIX_UNWIND } state;
    PTREE parent=NULL,temp;
    if (!tree || tree->op==0) return tree;
    state=PREFIX_PROCESS;
    for (;;) {
        switch (state) {
        case PREFIX_PROCESS:
            tree=visit(tree);
            /* fall through */
        case PREFIX_LEFT:
            if (ptreePTSFlags[tree->op]&PTS_OPERATOR) {
                if (tree->u.subtree[0]) {
                    temp=tree;
                    tree=temp->u.subtree[0];
                    temp->u.subtree[0]=parent;
                    temp->flags|=0x8000;
                    parent=temp;
                    state=PREFIX_PROCESS;
                } else state=PREFIX_RIGHT;
            } else state=PREFIX_UNWIND;
            break;
        case PREFIX_RIGHT:
            if ((ptreePTSFlags[tree->op]&PTS_BINARY) && tree->u.subtree[1]) {
                temp=tree;
                tree=temp->u.subtree[1];
                temp->u.subtree[1]=parent;
                temp->flags&=~0x8000;
                parent=temp;
                state=PREFIX_PROCESS;
            } else state=PREFIX_UNWIND;
            break;
        case PREFIX_UNWIND:
            if (!parent) return tree;
            temp=parent;
            if (parent->flags&0x8000) {
                parent->flags&=~0x8000;
                parent=temp->u.subtree[0];
                temp->u.subtree[0]=tree;
                state=PREFIX_RIGHT;
            } else {
                parent=temp->u.subtree[1];
                temp->u.subtree[1]=tree;
                state=PREFIX_UNWIND;
            }
            tree=temp;
            break;
        default: return tree;
        }
    }
}
void PTreeAssign(PTREE target,PTREE source)
{ (void)target; (void)source; STUB_TRAP(); }

PTREE PTreeUIntConstant(unsigned long value,type_id id)
{
    PTREE tree=PTreeAlloc();
    ptreeSetConstantType(tree,id);
    tree->op=3;
    tree->u.uint_constant=value;
    return tree;
}

PTREE PTreeCDtorExtra(void)
{
    PTREE tree=PTreeId(CppSpecialName(5));
    tree->cgop=200;
    return tree;
}

PTREE PTreeSetLocn(PTREE expr,CGIO_LOCATION *location)
{
    if(location && location->src_file) expr->locn=*location;
    return expr;
}

int IsOverloadedFunc(SYMBOL sym);
SYMBOL FunctionSymbol(PTREE node)
{
    SYMBOL sym;
    if (!node) return NULL;
    if (node->op==8 && node->cgop==12) node=PTreeOpLeft(node);
    if (node->op!=7 || node->cgop!=194) return NULL;
    sym=node->u.symcg.symbol;
    /* This1994 helper selects overloaded functions only; the later source
     * accepts any function symbol at this boundary. */
    return IsOverloadedFunc(sym) ? sym : NULL;
}

PTREE PTreeIc(unsigned opcode,unsigned value)
{
    PTREE node=PTreeAlloc();
    node->op=11;
    node->u.ic.opcode=opcode;
    node->u.ic.value=value;
    return node;
}

PTREE PTreeDupExpr(PTREE expr)
{
    PTREE node=PTreeAlloc();
    node->op=10;
    node->u.dup.partner=NULL;
    node->u.dup.expr=expr;
    node->type=expr->type;
    node->flags=expr->flags;
    return PTreeCopySrcLocation(node,expr);
}

char *CppOperatorName(unsigned oper);
PTREE MakeOperatorId(unsigned char op)
{
    PTREE tree=PTreeId(CppOperatorName(op));
    tree->cgop=198;
    tree->u.id.operator_code=op;
    return tree;
}

int IsStringConstant(PTREE expr,int *multi_line)
{
    struct string_constant *literal;
    *multi_line=0;
    if(expr->op!=1) return 0;
    literal=(struct string_constant *)expr->u.subtree[0];
    if(literal->concat && literal->multi_line) *multi_line=1;
    return 1;
}

PTREE PTreeIdSym(SYMBOL sym)
{ return PTreeId(sym->name->name); }

PTREE PTreeSetErrLoc(PTREE expr)
{ (void)expr; STUB_TRAP(); return NULL; }

/* Actual1994 linker constants use32-bit integral payloads. The later
 * symAccessNeedsCode helper and DLL-import test are absent here. */
static unsigned linkerConstantOffset;
static PTREE linkerConstantSymbolNode;
static int linkerConstantFlag;
int SymIsAutomatic(SYMBOL sym);
int NodeIsUnaryOp(PTREE tree,unsigned char op);
PTREE linker_constant_tree_node(PTREE expr)
{
    PTREE other;
    SYMBOL sym;
    if(!linkerConstantFlag) return expr;
    switch(expr->op) {
    case 2: case 3:
        if(linkerConstantSymbolNode==NULL) linkerConstantSymbolNode=expr;
        break;
    case 5: break;
    case 1: case 4:
        if(linkerConstantSymbolNode!=NULL) linkerConstantFlag=0;
        else linkerConstantSymbolNode=expr;
        break;
    case 7:
        if(expr->cgop!=194 || linkerConstantSymbolNode!=NULL)
            linkerConstantFlag=0;
        else {
            sym=expr->u.symcg.symbol;
            if(sym!=NULL && SymIsAutomatic(sym)) linkerConstantFlag=0;
            else linkerConstantSymbolNode=expr;
        }
        break;
    case 8:
        switch(expr->cgop) {
        case 12:
            other=linkerConstantSymbolNode;
            if(other!=NULL && other->op==7 && other->cgop==194 &&
               SymIsAutomatic(other->u.symcg.symbol)) linkerConstantFlag=0;
            break;
        case 40: break;
        case 101:
            if(linkerConstantSymbolNode!=NULL) linkerConstantFlag=0;
            break;
        default: linkerConstantFlag=0; break;
        }
        break;
    case 9:
        switch(expr->cgop) {
        case 44: case 57: break;
        case 1:
            other=expr->u.subtree[1];
            if(other->op==2 || other->op==3)
                linkerConstantOffset-=other->u.uint_constant;
            else linkerConstantFlag=0;
            break;
        case 0: case 48:
            other=expr->u.subtree[1];
            if(other->op==2 || other->op==3)
                linkerConstantOffset+=other->u.uint_constant;
            else linkerConstantFlag=0;
            break;
        default: linkerConstantFlag=0; break;
        }
        break;
    default: linkerConstantFlag=0; break;
    }
    return expr;
}
int IsLinkerConstant(PTREE tree,PTREE *ctree,unsigned *offset)
{
    if(tree->flags&1) return 0;
    if(NodeIsUnaryOp(tree,109)) { *ctree=tree; *offset=0; return 1; }
    linkerConstantOffset=0;
    linkerConstantFlag=1;
    linkerConstantSymbolNode=NULL;
    PTreeTraversePostfix(tree,linker_constant_tree_node);
    if(!linkerConstantFlag || linkerConstantSymbolNode==NULL) return 0;
    *ctree=linkerConstantSymbolNode;
    *offset=linkerConstantOffset;
    return 1;
}

PTREE MakeScopedOperatorId(PTREE scoped,unsigned char operation)
{
    PTREE id=MakeOperatorId(operation);
    PTreeCopySrcLocation(id,scoped);
    id=useScopeIfPossible(scoped,id);
    PTreeFreeSubtrees(scoped);
    return id;
}

PTREE MakeScopedDestructorId(PTREE scoped,PTREE id)
{
    TYPE class_type,unbound=NULL;
    PTREE tree=useScopeIfPossible(scoped,NULL);
    if(tree) {
        class_type=StructType(tree->u.subtree[0]->type);
        if(class_type->flag&4) unbound=class_type;
        id=makeDestructorId(class_type->u.c.scope,id,unbound);
        PTreeCopySrcLocation(id,scoped);
        id=PTreeReplaceRight(tree,id);
    }
    PTreeFreeSubtrees(scoped);
    return id;
}

PTREE MakeDeleteExpr(PTREE global,unsigned char operation,PTREE expr)
{
    PTreeFreeSubtrees(global);
    return PTreeUnary(operation,expr);
}

/* The1994 constant owns the double itself; the released implementation
 * instead owns a later CPP_FLOAT representation object. */
PTREE PTreeFloatingConstant(double value,type_id id)
{
    PTREE tree=PTreeAlloc();
    ptreeSetConstantType(tree,id);
    tree->op=4;
    tree->u.floating_constant=value;
    return tree;
}

TYPE MakeArrayOf(unsigned count,TYPE type);

/* Initial ptree.c strLiteral, matched to CF44c60. The old node directly
 * uses the stored byte length plus its terminator (no later AWStrLen call). */
PTREE strLiteral(struct string_constant *literal,type_id base)
{
    PTREE tree=PTreeAlloc();
    tree->op=1;
    tree->u.string=literal;
    tree->type=MakeArrayOf(literal->len+1,GetBasicType(base));
    tree->flags|=2;
    return tree;
}
PTREE PTreeLiteral(struct string_constant *literal)
{ return strLiteral(literal,TYP_CHAR); }
PTREE PTreeLiteralWide(struct string_constant *literal)
{ return strLiteral(literal,TYP_WCHAR); }

PTREE AnalyseValueExpr(PTREE expr);
void PTreeErrorExpr(PTREE expr,unsigned error);
/* The1994 tree owns a double. Sign-bit selection includes negative zero;
 * the period compiler emits its real FDI4/FDU4 conversion helpers. */
PTREE PTreeForceIntegral(PTREE expr)
{
    expr=AnalyseValueExpr(expr);
    if(expr->op==2 || expr->op==3) return expr;
    if(expr->op==4) {
        CErr1(71);
        if(expr->u.floating_bits.high&0x80000000UL) {
            expr->op=2;
            expr->u.int_constant=(long)expr->u.floating_constant;
            ptreeSetConstantType(expr,TYP_SINT);
        } else {
#ifdef __linux__
            /* Unreached old software-FP unsigned conversion has no native
             * qualified contract yet. The period build keeps its FDU4 ABI. */
            STUB_TRAP();
#endif
            expr->op=3;
            expr->u.uint_constant=(unsigned long)expr->u.floating_constant;
            ptreeSetConstantType(expr,TYP_UINT);
        }
        return expr;
    }
    if(expr->op!=0) PTreeErrorExpr(expr,58);
    PTreeFreeSubtrees(expr);
    return NULL;
}
PTREE PTreeNonZeroConstantExpr(PTREE expr)
{
    if(ScopeType(CurrScope,4)) {
        PTreeFreeSubtrees(expr);
        expr=PTreeIntConstant(1,TYP_SINT);
    } else {
        expr=PTreeForceIntegral(expr);
        if(expr==NULL) expr=PTreeIntConstant(1,TYP_SINT);
    }
    return expr;
}
