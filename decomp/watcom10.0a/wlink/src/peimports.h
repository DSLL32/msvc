#ifndef WLINK_PEIMPORTS_H
#define WLINK_PEIMPORTS_H
#include "link.h"
#include <recon.h>

typedef struct import_name {
    struct import_name *next;
    dll_sym_info *dll;
    snamelist *imp;
} import_name;
typedef struct module_import {
    struct module_import *next;
    snamelist *mod;
    import_name *imports;
    unsigned num_entries;
} module_import;
extern module_import *PEImpList;
extern unsigned NumMods, NumImports;
#endif
