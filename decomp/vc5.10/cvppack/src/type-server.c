/* Reuse or open a module's external type server [0040bca0,0040c080). */
#include <mbstring.h>
#include "pack.h"
#include "pdb.h"
#include <recon.h>

#pragma intrinsic(memcpy, strcpy, strcat, strlen, memset)

static unsigned last_type_server_module;
static char last_type_server_path[260];
char current_type_server_path[260], type_database_path[260];
int type_database_error;
unsigned type_database_count;
extern int verbose;

void register_type_server(const unsigned char *leaf)
{
    char filename[260], extension[256], *end;
    unsigned previous_count;
    int opened;
    memcpy(current_type_server_path, leaf + 11, leaf[10]);
    current_type_server_path[leaf[10]] = 0;
    if (type_module_count == last_type_server_module + 1 &&
        !_mbsicmp((unsigned char *)last_type_server_path,
                   (unsigned char *)current_type_server_path)) {
        if (PDBQuerySignature(type_pdb) != *(const unsigned *)(leaf + 2))
            fatal_error(1024, module_name(current_module_info), current_type_server_path);
        if (PDBQueryAge(type_pdb) < *(const unsigned *)(leaf + 6))
            fatal_error(1023, module_name(current_module_info), current_type_server_path);
        last_type_server_module = type_module_count;
        previous_count = module_precompiled_limit;
        type_record_count = type_database_count;
        module_type_entries = 1;
        if (previous_count > type_database_count)
            memset(type_records + type_database_count, 0,
                   (previous_count - type_database_count) * sizeof(TypeRecord));
        module_precompiled_limit = 0;
        return;
    }
    strcpy(last_type_server_path, current_type_server_path);
    last_type_server_module = type_module_count;
    if (type_pdb) {
        if (type_pdb_types) type_pdb_close_status = TypesClose(type_pdb_types);
        type_pdb_close_status |= PDBClose(type_pdb);
    }
    if (!PDBExportValidateInterface(0x13092b6)) fatal_error(1027, NULL, NULL);
    _splitpath(current_type_server_path, NULL, NULL, filename, extension);
    strcat(filename, extension);
    build_type_server_directory(current_type_server_path);
    end = current_type_server_path + strlen(current_type_server_path);
    if (end[-1] != '/' && end[-1] != '\\' && end[-1] != ':') *end++ = '\\';
    strcpy(end, filename);
    opened = PDBOpen(current_type_server_path, "rc", 0, &type_database_error, NULL, &type_pdb);
    if (opened && PDBQuerySignature(type_pdb) != *(const unsigned *)(leaf + 2)) {
        PDBClose(type_pdb);
        opened = 0;
    }
    if (!opened) {
        strcpy(current_type_server_path, last_type_server_path);
        opened = PDBOpen(current_type_server_path, "rc", 0, &type_database_error, NULL, &type_pdb);
        if (opened && PDBQuerySignature(type_pdb) != *(const unsigned *)(leaf + 2)) {
            PDBClose(type_pdb);
            opened = 0;
        }
        if (!opened) fatal_error(1022, current_type_server_path, NULL);
    }
    if (PDBQueryAge(type_pdb) < *(const unsigned *)(leaf + 6))
        fatal_error(1023, module_name(current_module_info), current_type_server_path);
    if (!PDBOpenTpi(type_pdb, "rc", &type_pdb_types))
        fatal_error(1022, current_type_server_path, NULL);
    read_type_database(current_type_server_path);
}

/* Derive the type-server search directory from this module's counted object
 * name [0040b930,0040b9e9). Old libraries may supply that name by index. */
void build_type_server_directory(char *path)
{
    unsigned short library=*(unsigned short *)(current_module_buffer+2);
    unsigned char *name;
    char drive[4],module[260],directory[256];
    if (!library) name=current_module_buffer+8+12*(*(unsigned short *)(current_module_buffer+4));
    else {
        if (image_linker_major==2 && image_linker_minor<60) --library;
        name=module_file_name(library);
    }
    memcpy(module,name+1,*name);
    module[*name]=0;
    _splitpath(module,drive,directory,NULL,NULL);
    _makepath(path,drive,directory,NULL,NULL);
    if (!*path) { path[0]='.';path[1]=0; }
}

/* Import a type server's persistent mutable record spans [0040b9f0,0040bc86).
 * The four-entry original dispatch table is owned separately at bc88..bc98. */
void read_type_database(const char *path)
{
    unsigned first,limit,index,old_count;
    unsigned char *data;
    TypeRecord *record;
    unsigned short kind;
    if (!path) {
        if (type_pdb) {
            if (type_pdb_types) type_pdb_close_status=TypesClose(type_pdb_types);
            type_pdb_close_status|=PDBClose(type_pdb);
        }
        if (!PDBOpen(type_database_path,"rc",0,&type_database_error,NULL,&type_pdb) ||
            !PDBOpenTpi(type_pdb,"rc",&type_pdb_types))
            fatal_error(1022,type_database_path,NULL);
    } else strcpy(type_database_path,path);
    if (verbose) printf(message_text(8015),current_type_server_path,'\n');
    if (module_precompiled_limit) {
        memset(type_records,0,module_precompiled_limit*sizeof(TypeRecord));
        module_precompiled_limit=0;
    }
    first=TypesQueryTiMinEx(type_pdb_types);
    limit=TypesQueryTiMacEx(type_pdb_types);
    for (index=first;index<limit;++index) {
        type_pdb_close_status=TypesQueryPbCVRecordForTiEx(type_pdb_types,index,&data);
        old_count=type_record_count;
        if (module_precompiled_limit) {
            memset(type_records,0,module_precompiled_limit*sizeof(TypeRecord));
            module_precompiled_limit=0;
        }
        if (++type_record_count>=type_record_capacity) {
            type_record_capacity+=6000;
            type_records=realloc(type_records,type_record_capacity*sizeof(TypeRecord));
            if (!type_records) fatal_error(1001,NULL,NULL);
            memset(type_records+old_count,0,6000*sizeof(TypeRecord));
        }
        record=type_records+old_count;
        record->data=data;
        record->mapped_index=0;
        record->status=(record->status|0x400)&~0x800;
        kind=data?*(unsigned short *)(data+2):0;
        record->status=(record->status&~0x4000)|
            ((data && kind>=0x1004 && kind<=0x1007?(data[6]>>7)&1:0)<<14);
    }
    type_database_count=type_record_count;
}
