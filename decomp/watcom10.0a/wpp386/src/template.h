#ifndef WPP_TEMPLATE_H
#define WPP_TEMPLATE_H
#include "types.h"
#include "scope.h"
#include "symbol.h"
#include "rewrite.h"
#include "ptree.h"
#include <recon.h>
#pragma pack(1)
struct class_inst {
    struct class_inst *next;
    SCOPE scope;
    CGIO_LOCATION locn;
    unsigned flags;
};
typedef char ClassInstSize[(sizeof(struct class_inst)==24)?1:-1];
typedef struct template_member TEMPLATE_MEMBER;
struct template_member { TEMPLATE_MEMBER *next; REWRITE *defn; char **arg_names; };
typedef char TemplateMemberSize[(sizeof(TEMPLATE_MEMBER)==12)?1:-1];
typedef struct template_info TEMPLATE_INFO;
struct template_info {
    TEMPLATE_INFO *next;
    REWRITE *defn;
    struct class_inst *instantiations;
    struct template_member *member_defns;
    unsigned num_args;
    TYPE unbound_type;
    TYPE *type_list;
    char **arg_names;
    unsigned flags; /* corrupted1, definition-found2, free4 */
};
typedef struct template_data TEMPLATE_DATA;
struct template_data {
    TEMPLATE_DATA *next;
    DECL_INFO *args;
    SCOPE decl_scope;
    void *defn,*member_defn;
    char *template_name;
    unsigned errors;
    unsigned char symbol_id;
    unsigned char all_generic:1,defn_found:1,member_found:1,defn_added:1,reserved:4;
};
#pragma pack()
typedef char TemplateDataSize[(sizeof(TEMPLATE_DATA)==30)?1:-1];
void TemplateDeclInit(TEMPLATE_DATA *data,DECL_INFO *args);
void TemplateDeclFini(void);
int ProcessTemplateArgs(DECL_INFO *args);
void TemplateClassDeclaration(char *name);
int TemplateClassDefinition(char *name);
DECL_SPEC *TemplateClassInstantiation(PTREE id,PTREE parms,unsigned control);
#endif
