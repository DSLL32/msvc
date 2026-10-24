#!/usr/bin/env python3
"""Finite assembly initializer call ABI and isolated nonzero-list checks."""
import argparse,copy,json,re,struct,subprocess,tempfile,time,sys,importlib.util
from pathlib import Path
from state import BINARY,ROOT,TARGET,sha
from data_evidence import reader

def verify(config,source=None):
    value=json.loads((BINARY/'cases/analysis/assembly-records-analysis.json').read_text());original=reader(TARGET)
    assert value['target_sha256']==sha(TARGET.read_bytes())
    assert value['primary']==[0x41f58c,0x41f59c]and value['code_chunks']==[[0x469cbe,0x469ce6]]and value['bytes']==56
    assert sha(original(0x41f58c,16))==value['primary_sha256']and sha(original(0x469cbe,40))==value['cold_sha256']
    assert sha(original(0x48a157,250))==value['emit']['sha256']and original(0x48a205,1)==b'\xc3'
    assert sha((BINARY/'cases/analysis/assembly-records-disassembly.txt').read_bytes())==value['disassembly_sha256']
    text=(BINARY/'src/assembly-output.c').read_text()if source is None else source
    exact='void __fastcall write_assembly_records(C2DebugLink *records)\n{\n    if (assembly_output_active) {\n        for (; records; records = records->next)\n            write_assembly_initializer(lookup_initializer(records->symbol->serial));\n    }\n}'
    assert text.count(exact)==1,'whole actual producer must retain assembly flag/list/symbol-serial and EAX pointer forwarding'
    definitions={r['name']:r for r in config['driver']+config['stubs']}
    for name,address,native in [('@lookup_initializer@4',0x486b1d,'lookup_initializer'),('@write_assembly_initializer@4',0x48a157,'write_assembly_initializer')]:
        r=definitions[name];assert r['start']==address and r['native']==native
        assert name not in config['accepted']+config['trial']and r['pending_extent']and r['end']==address+1
    maps={m[1]:int(m[2],16)for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',(BINARY/'build/c2.map').read_text(),re.M)}
    rebuilt=reader(BINARY/'build/c2.exe');base=maps['@write_assembly_records@4']
    assert rebuilt(base+7,7)==b'\x83\x3d'+struct.pack('<I',maps['_assembly_output_active'])+b'\0'
    assert rebuilt(base+35,6)==bytes.fromhex('8b42048b4828')
    for offset,target in [(41,'@lookup_initializer@4'),(48,'@write_assembly_initializer@4')]:
        call=base+offset;assert rebuilt(call,5)==b'\xe8'+struct.pack('<i',maps[target]-call-5)
    assert rebuilt(base+46,2)==b'\x8b\xc8'and rebuilt(base+58,1)==b'\xc3'
    return {'source_sha256':sha(text.encode()),'analysis_sha256':sha((BINARY/'cases/analysis/assembly-records-analysis.json').read_bytes()),'canonical_sha256':sha((BINARY/'build/c2.exe').read_bytes()),'scope':'56B producer only; genuine pending lookup and initializer emitter remain masked/no bodycredit. Lookup error path shares only existing separately masked/unaccepted internal-error identity, not a new overlapping extent.'}

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();start=time.monotonic();c=json.loads((BINARY/'build/trial.json').read_text());positive=verify(c);passed=[];source=(BINARY/'src/assembly-output.c').read_text()
    for name,before,after in [('wrong assembly guard','    if (assembly_output_active) {\n        for (; records;','    if (c2_assembly_enabled) {\n        for (; records;'),('wrong serial owner','lookup_initializer(records->symbol->serial)','lookup_initializer(records->symbol->flags)'),('wrong list advancement','for (; records; records = records->next)','for (; records; records = NULL)'),('lookup result replaced with symbol','write_assembly_initializer(lookup_initializer(records->symbol->serial));','write_assembly_initializer((C2InitRecord*)records->symbol);')]:
        assert before in source;changed=source.replace(before,after,1);assert changed!=source
        try:verify(c,changed)
        except AssertionError as e:passed.append({'control':name,'reason':str(e)})
        else:raise AssertionError('mutation accepted: '+name)
    for name,key,value in [('lookup pointer identity changed','start',0x486b1e),('pending lookup body wrongly credited','end',0x486b55)]:
        config=copy.deepcopy(c);row=next(r for r in config['stubs']if r['name']=='@lookup_initializer@4');assert row[key]!=value;row[key]=value
        try:verify(config)
        except AssertionError as e:passed.append({'control':name,'reason':str(e)})
        else:raise AssertionError('mutation accepted: '+name)
    with tempfile.TemporaryDirectory(prefix='assembly-record-aux-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        folder=Path(folder);harness=folder/'nonzero.c';harness.write_text(r'''#include "code-records.h"
static C2InitRecord a,b;
static unsigned phase;
static int failed;
C2InitRecord *__fastcall lookup_initializer(unsigned serial) {
    if ((phase==0 && serial==85) || (phase==2 && serial==92)) { ++phase; return serial==85?&a:&b; }
    failed=1; return 0;
}
void __fastcall write_assembly_initializer(C2InitRecord *record) {
    if ((phase==1 && record==&a) || (phase==3 && record==&b)) ++phase;
    else failed=1;
}
int main(int argc,char **argv,char **environment) {
    C2Symbol first,second; C2DebugLink one,two;
    first.serial=85; second.serial=92;
    one.next=&two; one.symbol=&first; two.next=0; two.symbol=&second;
    assembly_output_active=0; write_assembly_records(&one); if (phase||failed) return 1;
    assembly_output_active=1; write_assembly_records(0); if (phase||failed) return 2;
    write_assembly_records(&one); if (phase!=4||failed) return 3;
    return 0;
}
''');exe=folder/'nonzero';cmd=['clang','-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-DRECON_NATIVE_TEST','-Wno-ignored-attributes','-O0','-g','-ffunction-sections','-fdata-sections','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),'-I'+str(BINARY/'src'),str(BINARY/'src/assembly-output.c'),str(harness),'-Wl,--gc-sections','-o',str(exe)]
        t=time.monotonic();r=subprocess.run(cmd,capture_output=True);assert r.returncode==0,r.stderr.decode();build=time.monotonic()-t
        t=time.monotonic();r=subprocess.run([str(exe)],capture_output=True);assert r.returncode==0;execution=time.monotonic()-t
        passed.append({'control':'isolated nonzero serial/lookup-result forwarding order','reason':'flag0+nonNULLlist andflag1+NULLlist bothskip; flag1+twoactualnodes passes serial85/92 and exactdistinct returned initializer pointers in lookup/emit order. Both children typed observers only in isolatedauxiliary ELF, no targetbody credit.','build_seconds':build,'execution_seconds':execution,'harness_sha256':sha(harness.read_bytes())})
    a.output.write_text(json.dumps({'positive':positive,'count':len(passed),'rejected':passed,'seconds':time.monotonic()-start,'checker_sha256':sha(Path(__file__).read_bytes())},indent=2)+'\n')
if __name__=='__main__':main()
