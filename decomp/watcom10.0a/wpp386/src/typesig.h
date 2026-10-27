#ifndef WPP_TYPESIG_H
#define WPP_TYPESIG_H
#include "types.h"
#include "symbol.h"
#include <recon.h>
/* Actual1994 signature ring record, before the later base link. */
typedef struct type_sig TYPE_SIG;
struct type_sig {
    TYPE_SIG *next;
    TYPE type;
    SYMBOL sym,dtor,default_ctor,copy_ctor;
    unsigned cgref:1, cggen:1, free:1;
};
typedef char TypeSigSize[(sizeof(TYPE_SIG)==28)?1:-1];
TYPE_SIG *TypeSigFind(unsigned access,TYPE type,CGIO_LOCATION *locn,int *error);
void TypeSigReferenced(TYPE_SIG *sig);
#endif
