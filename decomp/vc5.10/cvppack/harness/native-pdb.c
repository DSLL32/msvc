/* Linux host-only response adapter for the actual case's MSPDB C exports.
 * This is neither a PDB parser nor reconstructed dependency code. */
#define RECON_SHIM_IMPL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "pack.h"
#include "pdb.h"
#include <recon.h>

typedef struct PdbRecordResponse {
    unsigned index, result, length;
    unsigned char *bytes;
} PdbRecordResponse;
static PdbRecordResponse records[4096];
static unsigned enabled, stage, position, minimum, maximum;
static unsigned validate_result, open_result, signature, age, open_tpi_result;
static unsigned cached_signature, cached_age, types_close_result, pdb_close_result;
static unsigned char pdb_owner, tpi_owner;
static unsigned char **record_slot;

static void require(int condition,const char *what)
{ if(!condition)recon_unsupported(what); }
static unsigned word(FILE *file)
{
    unsigned char b[4];require(fread(b,1,4,file)==4,"CVPACK PDB response truncated word");
    return b[0]|(unsigned)b[1]<<8|(unsigned)b[2]<<16|(unsigned)b[3]<<24;
}
static void verify_input_file(FILE *response,const char *name,unsigned size)
{
    FILE *input=fopen(name,"rb");unsigned i;
    require(input && word(response)==size,"CVPACK PDB complete input size");
    for(i=0;i<size;++i){int expected=fgetc(response),actual=fgetc(input);
        require(expected!=EOF && actual==expected,"CVPACK PDB complete input bytes");}
    require(fgetc(input)==EOF && !ferror(input),"CVPACK PDB complete input extent");fclose(input);
}
static void finished(void)
{ require(!enabled || stage==12,"CVPACK PDB incomplete dependency sequence"); }
static void active(unsigned expected)
{ require(enabled && stage==expected,"CVPACK PDB unobserved method or sequence"); }
void cvpack_pdb_load(const char *filename)
{
    FILE *file;unsigned i;unsigned char magic[4];int saved_errno=errno;
    require(!enabled,"CVPACK PDB duplicate setup");file=fopen(filename,"rb");
    require(file && fread(magic,1,4,file)==4 && !memcmp(magic,"PPB1",4),"CVPACK PDB response header");
    verify_input_file(file,"archive.dll",4608);verify_input_file(file,"archive.pdb",53248);
    validate_result=word(file);open_result=word(file);signature=word(file);age=word(file);
    open_tpi_result=word(file);minimum=word(file);maximum=word(file);
    require(validate_result==1 && open_result==1 && open_tpi_result==1 &&
        minimum==4096 && maximum==4115,"CVPACK PDB observed scalar bounds");
    for(i=0;i<maximum-minimum;++i) {
        PdbRecordResponse *record=&records[i];
        record->index=word(file);record->result=word(file);record->length=word(file);
        require(record->index==minimum+i && record->result==1 && record->length>=4 &&
            record->length<=65536,"CVPACK PDB returned record bounds");
        record->bytes=malloc(record->length);
        require(record->bytes && fread(record->bytes,1,record->length,file)==record->length,
                "CVPACK PDB returned record bytes");
        require(record->length==2+record->bytes[0]+(unsigned)record->bytes[1]*256,
                "CVPACK PDB returned record length");
    }
    cached_signature=word(file);cached_age=word(file);
    types_close_result=word(file);pdb_close_result=word(file);
    require(cached_signature==signature && cached_age==age && types_close_result==1 &&
        pdb_close_result==1 && word(file)==0x444e4544 && fgetc(file)==EOF && !ferror(file),
        "CVPACK PDB response terminal");
    fclose(file);enabled=1;require(!atexit(finished),"CVPACK PDB completion hook");errno=saved_errno;
}
int __cdecl PDBExportValidateInterface(unsigned version)
{
    active(0);require(version==0x13092b6,"CVPACK PDB interface version");stage=1;return validate_result;
}
int __cdecl PDBOpen(const char *path,const char *mode,unsigned requested_signature,
                    int *error,char *error_text,void **pdb)
{
    active(1);
    require(path==current_type_server_path && !strcmp(path,".\\archive.pdb") && mode && !strcmp(mode,"rc") &&
        !requested_signature && error==&type_database_error && !error_text && pdb==&type_pdb,
        "CVPACK PDB Open inputs and owners");
    /* The successful original's error output is not consumed. Its initialization
     * is unclaimed: keep this native slot's own value rather than inventing it. */
    *pdb=&pdb_owner;stage=2;return open_result;
}
unsigned __cdecl PDBQuerySignature(void *pdb)
{
    require(enabled && pdb==&pdb_owner && (stage==2 || stage==8),"CVPACK PDB signature owner or sequence");
    if(stage==2){stage=3;return signature;}stage=9;return cached_signature;
}
unsigned __cdecl PDBQueryAge(void *pdb)
{
    require(enabled && pdb==&pdb_owner && (stage==3 || stage==9),"CVPACK PDB age owner or sequence");
    if(stage==3){stage=4;return age;}stage=10;return cached_age;
}
int __cdecl PDBOpenTpi(void *pdb,const char *mode,void **types)
{
    active(4);require(pdb==&pdb_owner && mode && !strcmp(mode,"rc") && types==&type_pdb_types,
                      "CVPACK PDB TPI inputs and owners");
    *types=&tpi_owner;stage=5;return open_tpi_result;
}
unsigned __cdecl TypesQueryTiMinEx(void *types)
{ active(5);require(types==&tpi_owner,"CVPACK PDB minimum owner");stage=6;return minimum; }
unsigned __cdecl TypesQueryTiMacEx(void *types)
{ active(6);require(types==&tpi_owner,"CVPACK PDB maximum owner");stage=7;return maximum; }
int __cdecl TypesQueryPbCVRecordForTiEx(void *types,unsigned index,unsigned char **record)
{
    PdbRecordResponse *response;
    active(7);require(types==&tpi_owner && position<maximum-minimum &&
        index==minimum+position && record,"CVPACK PDB record owner/index");
    if(!record_slot)record_slot=record;
    require(record==record_slot,"CVPACK PDB stable record output slot");
    response=&records[position++];*record=response->bytes;
    /* The App owns its mutations to these independent, disjoint live spans.
     * Do not reinstall their original bytes on any later call. */
    if(position==maximum-minimum)stage=8;
    return response->result;
}
int __cdecl TypesClose(void *types)
{
    unsigned i;active(10);require(types==&tpi_owner,"CVPACK PDB Close TPI owner");
    for(i=0;i<maximum-minimum;++i){free(records[i].bytes);records[i].bytes=NULL;}
    stage=11;return types_close_result;
}
int __cdecl PDBClose(void *pdb)
{ active(11);require(pdb==&pdb_owner,"CVPACK PDB Close owner");stage=12;return pdb_close_result; }
