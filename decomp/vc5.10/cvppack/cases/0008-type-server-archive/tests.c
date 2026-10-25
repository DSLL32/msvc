/* Native-only boundary checks; the public case's fresh IO pair supplies its
 * actual original oracle. No PDB implementation or private layout is tested. */
#define RECON_SHIM_IMPL
#include <sys/wait.h>
#include <unistd.h>
#include <recon-tool.h>
#include "pack.h"
#include "pdb.h"
#include <recon.h>

static int requests(unsigned mode)
{
    unsigned signature,age,first,last,index,i;
    unsigned char *record,*first_record=NULL;
    unsigned char marker=0;
    strcpy(current_type_server_path,".\\archive.pdb");
    if(!PDBExportValidateInterface(0x13092b6))return 90;
    if(!PDBOpen(current_type_server_path,mode==4?"r":"rc",0,&type_database_error,NULL,&type_pdb))return 91;
    if(mode==2)PDBQuerySignature((void *)&marker);
    signature=PDBQuerySignature(type_pdb);age=PDBQueryAge(type_pdb);
    if(!PDBOpenTpi(type_pdb,"rc",&type_pdb_types))return 92;
    first=TypesQueryTiMinEx(type_pdb_types);last=TypesQueryTiMacEx(type_pdb_types);
    if(mode==3)TypesClose(type_pdb_types);
    if(mode==1)TypesQueryPbCVRecordForTiEx(type_pdb_types,first+1,&record);
    for(index=first;index<last;++index) {
        if(!TypesQueryPbCVRecordForTiEx(type_pdb_types,index,&record) || !record)return 93;
        if(index==first) {
            if(*(unsigned short *)record<4)return 94;
            first_record=record;marker=record[4]^0x55;record[4]=marker;
        } else if(record==first_record || first_record[4]!=marker)return 95;
        for(i=0;i<2;++i)if(!record[i] && !record[1-i])return 96;
    }
    if(PDBQuerySignature(type_pdb)!=signature || PDBQueryAge(type_pdb)!=age ||
       first_record[4]!=marker || !TypesClose(type_pdb_types) || !PDBClose(type_pdb))return 97;
    /* All returned span lifetimes have ended. Never inspect them after Close. */
    return 0;
}
static int check(unsigned mode,int expected)
{
    int status,pid=fork();if(pid<0)return 0;
    if(!pid)_exit(requests(mode));
    return waitpid(pid,&status,0)==pid && WIFEXITED(status) && WEXITSTATUS(status)==expected;
}
static int mutable_spans(void){return check(0,0);}
static int wrong_index(void){return check(1,125);}
static int wrong_owner(void){return check(2,125);}
static int early_close(void){return check(3,125);}
static int wrong_mode(void){return check(4,125);}
const ReconTest cvpack_pdb_tests[5] = {
 {"pdb-mutable-live-spans",1,mutable_spans},
 {"pdb-wrong-index",1,wrong_index},
 {"pdb-foreign-handle",1,wrong_owner},
 {"pdb-early-close",1,early_close},
 {"pdb-wrong-open-mode",1,wrong_mode}
};
