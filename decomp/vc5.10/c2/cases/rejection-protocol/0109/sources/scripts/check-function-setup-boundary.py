#!/usr/bin/env python3
"""Whole setup wrapper: scalar aliases, ordered noarg calls and byte-only clear."""
import argparse,json,re,struct,subprocess,tempfile,time
from pathlib import Path
from state import BINARY,ROOT,TARGET,sha
from data_evidence import reader

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();start=time.monotonic()
    value=json.loads((BINARY/'cases/analysis/begin-function-input-analysis.json').read_text());source=reader(TARGET);assert value['target_sha256']==sha(TARGET.read_bytes())
    assert value['candidate']['bytes']==76 and sha(source(0x428600,66))==value['candidate']['primary_sha256']and sha(source(0x46c001,10))==value['candidate']['cold_sha256']
    assert source(0x424726,10)==bytes.fromhex('c705e40b490001000000'),'other writer proves real DWORD storage'
    assert source(0x428620,7)==bytes.fromhex('c605e40b490000'),'wrapper writes only low byte'
    maps={m[1]:int(m[2],16)for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',(BINARY/'build/c2.map').read_text(),re.M)}
    rebuilt=reader(BINARY/'build/c2.exe');base=maps['_begin_function_input']
    for at,name in [(3,'_reset_register_marks'),(8,'_initialize_function_records'),(13,'_clear_function_input_state'),(54,'_reset_function_listing')]:
        call=base+at;assert rebuilt(call,5)==b'\xe8'+struct.pack('<i',maps[name]-call-5)
    assert rebuilt(base+18,10)==b'\xc7\x05'+struct.pack('<I',maps['_code_generation_state'])+struct.pack('<I',1)
    assert rebuilt(base+28,7)==b'\xc6\x05'+struct.pack('<I',maps['_function_input_flags'])+b'\0'
    assert rebuilt(base+35,10)==b'\xc7\x05'+struct.pack('<I',maps['_current_function'])+bytes(4)
    assert rebuilt(base+59,10)==b'\xc7\x05'+struct.pack('<I',maps['_function_analysis_available'])+bytes(4)
    assert rebuilt(base+70,1)==b'\xc3'
    c=json.loads((BINARY/'build/trial.json').read_text());definitions={r['name']:r for r in c['stubs']}
    for row in value['children']:
        r=definitions[row['name']];assert r['start']==row['entry']and r['end']==r['start']+1 and r['pending_extent']and r['name']not in c['accepted']+c['trial']
        a0,z=row['analysis'];assert sha(source(a0,z-a0))==row['sha256']and source(z-1,1)==b'\xc3'
    text=(BINARY/'src/function-setup.c').read_text();assert text.count('*(unsigned char *)&function_input_flags = 0;')==1
    passed=[]
    # Compare actual compiled source against a finite mutation of the field
    # write. The auxiliary observers implement only children for this test.
    with tempfile.TemporaryDirectory(prefix='setup-byte-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        folder=Path(folder);harness=folder/'nonzero.c';harness.write_text(r'''#include "functions.h"
#include <recon.h>
C2Function *current_function;
unsigned code_generation_state;
int c2_internal_listing;
static C2Function owner;
static unsigned phase, seed;
static int failed;
static void before(unsigned wanted) {
    if (phase!=wanted || current_function!=&owner || code_generation_state!=7 || (unsigned)function_input_flags!=seed || function_analysis_available!=9) failed=1;
    ++phase;
}
void reset_register_marks(void) { before(0); }
void initialize_function_records(void) { before(1); }
void clear_function_input_state(void) { before(2); }
void reset_function_listing(void) {
    if (phase!=3 || current_function || code_generation_state!=1 || (unsigned)function_input_flags!=(seed&~255U) || function_analysis_available!=9) failed=1;
    ++phase;
}
int main(int argc,char **argv,char **environment) {
    unsigned i;
    for (i=0;i<2;++i) {
        phase=0; failed=0; seed=i?0xabcdef12U:0x12345678U;
        current_function=&owner; code_generation_state=7; function_input_flags=(int)seed; function_analysis_available=9; c2_internal_listing=i;
        begin_function_input();
        if (failed || phase!=(i?4:3) || current_function || code_generation_state!=1 || (unsigned)function_input_flags!=(seed&~255U) || function_analysis_available) return 1;
    }
    return 0;
}
''');compile_prefix=['clang','-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-DRECON_NATIVE_TEST','-Wno-ignored-attributes','-O0','-g','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),'-I'+str(BINARY/'src')]
        def run(name,data):
            file=folder/(name+'.c');file.write_text(data);exe=folder/name;t=time.monotonic();r=subprocess.run(compile_prefix+[str(file),str(harness),'-o',str(exe)],capture_output=True);assert r.returncode==0,r.stderr.decode();build=time.monotonic()-t
            t=time.monotonic();r=subprocess.run([str(exe)],capture_output=True);return r.returncode,build,time.monotonic()-t
        result,build,execution=run('actual',text);assert result==0
        passed.append({'control':'actual nonzero byte-write/order/alias/listing paths','reason':'Two distinct DWORDsentinels preserve upper3bytes; three ordinarychildren precede writes; optional listing child follows codegen/flag/current reset and precedes analysisreset; same physical globals used. Children are isolated typedobservers only, no targetbody credit.','build_seconds':build,'execution_seconds':execution})
        cases=[('DWORD clear must fail','*(unsigned char *)&function_input_flags = 0;','function_input_flags = 0;'),('wrong current-function alias must fail','current_function = NULL;','current_function = &owner;'),('first two children exchanged','reset_register_marks();\n    initialize_function_records();','initialize_function_records();\n    reset_register_marks();'),('analysis reset too early','    if (c2_internal_listing)','    function_analysis_available = 0;\n    if (c2_internal_listing)'),('code-generation state wrong','code_generation_state = 1;','code_generation_state = 0;')]
        for name,before,after in cases:
            assert before in text;mutated=text.replace(before,after,1)
            if 'owner' in after:mutated=mutated.replace('#include <recon.h>','#include <recon.h>\nstatic C2Function owner;')
            result,build,execution=run('negative'+str(len(passed)),mutated);assert result!=0,'mutation passed: '+name
            passed.append({'control':name,'reason':'isolated compiled actual producer rejected semantic mutation','build_seconds':build,'execution_seconds':execution})
    a.output.write_text(json.dumps({'count':len(passed),'controls':passed,'seconds':time.monotonic()-start,'source_sha256':sha(text.encode()),'checker_sha256':sha(Path(__file__).read_bytes()),'analysis_sha256':sha((BINARY/'cases/analysis/begin-function-input-analysis.json').read_bytes()),'canonical_sha256':sha((BINARY/'build/c2.exe').read_bytes()),'scope':'Original76B wrapper and exact canonical writer/qualifiedcall operands; no newchildbody/table credit. All actual selected-first initializationstate/formals separately compared at genuine firstchild INT3.'},indent=2)+'\n')
if __name__=='__main__':main()
