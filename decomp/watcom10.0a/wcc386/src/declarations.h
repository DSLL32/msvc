#ifndef WCC_DECLARATIONS_H
#define WCC_DECLARATIONS_H
#include "symbols.h"
#include <recon.h>
TYPEPTR AbsDecl(TYPEPTR type);
TYPEPTR *FuncProtoType(void);
int VoidType(TYPEPTR type);
TYPEPTR *GetProtoType(TYPEPTR type,int stg_class);
void GetFuncParmList(void);
void FreeEnums(void);
struct parm_list { struct parm_list *next_parm; TYPEPTR parm_type; };
struct parm_entry { struct parm_entry *next_parm; SYM_ENTRY sym; };
TYPEPTR AdjParmType(TYPEPTR type,unsigned flags);
void CheckUniqueName(char *name);
struct mod_info { unsigned modifier; int segment; };
TYPEPTR Pointer(TYPEPTR type,struct mod_info *info);
unsigned GetModifiers(void);
int AddSegName(char *name,char *class_name,int kind);
SYM_HANDLE SegSymHandle(int segment);
struct parm_list *NewParm(TYPEPTR type,struct parm_list *previous);
TYPEPTR *MakeParmList(struct parm_list *parm,int count,int reversed);
TYPEPTR FuncNode(TYPEPTR type,unsigned flags,TYPEPTR *parms);
int FuncHeadIndex(TYPEPTR *parms);
void ParseDeclPart2(TYPEPTR *result,TYPEPTR type,unsigned modifier);
void SymCreate(SYMPTR sym,char *name);
TYPEPTR DeclPart2(TYPEPTR type,unsigned modifier);
TYPEPTR DeclPart3(TYPEPTR type,unsigned modifier);
TYPEPTR ArrayDecl(TYPEPTR type);
TYPEPTR ArrayNode(TYPEPTR type);
void SetFuncSegment(SYMPTR sym,int segment);
void Expecting(char *text);
extern struct parm_entry *ParmList;
/* Old quad control block: recovered control links and the local symbol list.
 * The intervening loop/switch state remains unused until its owning helpers
 * are reconstructed; do not interpret these bytes as a later tree block. */
#pragma pack(1)
struct block_definition {
    struct block_definition *prev_block,*prev_loop;
    int block_type;
    unsigned top_label,break_label,continue_label;
    union {
        struct { int try_index,parent_index; unsigned char spare[20]; } try_scope;
        struct {
            unsigned init_quad,last_quad,condition_end,increment_start;
            unsigned induction_leaf;
            int initial_value,limit_value;
        } for_loop;
    } u;
    SYM_HANDLE sym_head;
    int infinite_loop;
};
#pragma pack()
extern struct block_definition *BlockStack;
int GetStgClass(void);
TYPEPTR TypeSpecifier(void);
TYPEPTR GetFieldTypeSpecifier(char *plain_int);
unsigned TypeQualifier(void);
int GetScalarType(unsigned *flags);
TYPEPTR StructDecl(DATA_TYPE kind,unsigned flags);
unsigned long GetFields(TYPEPTR type);
unsigned char UnQualifiedType(TYPEPTR type);
FIELDPTR FieldDecl(TYPEPTR type,TYPEPTR explicit_type);
FIELDPTR FieldCreate(char *name);
FIELDPTR NewField(FIELDPTR field,TYPEPTR owner);
unsigned GetTypeAlignment(TYPEPTR type);
unsigned long FieldAlign(unsigned long offset,FIELDPTR field,unsigned *worst);
void CheckBitfieldType(TYPEPTR type);
TYPEPTR EnumFieldType(TYPEPTR type,unsigned char plain_int,unsigned offset,unsigned width);
TYPEPTR EnumDecl(unsigned flags);
struct tag_entry *TagLookup(void);
struct tag_entry *NewTag(char *name);
struct tag_entry *VfyNewTag(struct tag_entry *tag,DATA_TYPE kind);
struct tag_entry *NullTag(void);
struct enum_entry *EnumLkAdd(struct tag_entry *tag);
void SetEnumThread(struct enum_entry *entry,struct enum_entry *next);
void SetEnumValue(struct enum_entry *entry,int value);
SYM_HANDLE SymLookTypedef(int hash,char *name,SYMPTR sym);
SYM_HANDLE SymAddL0(int hash,SYMPTR sym);
void CmpFuncDecls(SYMPTR new_sym,SYMPTR old_sym);
void XferPragInfo(char *old_name,char *new_name);
extern TYPEPTR PrevProtoType;
extern unsigned DefDataSegment;
int IdenticalType(TYPEPTR first,TYPEPTR second);
int VerifyType(TYPEPTR first,TYPEPTR second,SYMPTR sym);
SYM_HANDLE Sym0Look(int hash,char *name);
void CErrSymName(int code,SYMPTR sym,SYM_HANDLE handle);
void StaticInit(SYMPTR sym,SYM_HANDLE handle);
unsigned GetInitExpr(void);
SYM_HANDLE MakeNewSym(SYMPTR sym,char id,TYPEPTR type,int stg_class);
extern int SymTypedef;
void FreeParmList(void);
void Declarator(SYMPTR sym,TYPEPTR type,TYPEPTR explicit_type);
void InvDecl(void);
SYM_HANDLE SymLook(int hash,char *name);
struct enum_info { int value,level; struct tag_entry *parent; };
int EnumLookup(int hash,char *name,struct enum_info *info);
void VfyNewSym(int hash,char *name);
SYM_HANDLE SymAdd(int hash,SYMPTR sym);
SYM_HANDLE FuncDecl(SYMPTR sym,int stg_class);
SYM_HANDLE VarDecl(SYMPTR sym,int stg_class);
SYM_HANDLE InitDeclarator(SYMPTR sym,int stg_class,TYPEPTR type,TYPEPTR explicit_type);
void FlushBadCode(void);
void Chk_Struct_Union_Enum(TYPEPTR type);
int DeclList(SYM_HANDLE *dummy);
void FuncDefn(SYMPTR sym);
void SymDump(void);
unsigned MakeSymLeaf(SYMPTR sym,SYM_HANDLE handle);
void LinkFunctionQuads(unsigned index);
TYPEPTR ArgPromotion(TYPEPTR type,unsigned flags);
void ReverseParms(void);
void AddParms(void);
void ChkProtoType(void);
void ChkParms(void);
void ParmDeclList(void);
struct textsegment *NewTextSeg(char *name,char *class_name,char *group);
extern struct textsegment *DefCodeSegment;
void BeginFunc(void);
void Statement(void);
#endif
