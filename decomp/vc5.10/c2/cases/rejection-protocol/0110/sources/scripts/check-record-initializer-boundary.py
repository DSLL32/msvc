#!/usr/bin/env python3
"""Finite initializer-member owner, source, pointer width and layout controls."""
import argparse,copy,json,re,shutil,subprocess,tempfile,time
from pathlib import Path
from record_data import verify_initializers
from state import BINARY,ROOT,sha

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();start=time.monotonic();c=json.loads((BINARY/'build/trial.json').read_text());positive=verify_initializers(BINARY,c);passed=[]
    with tempfile.TemporaryDirectory(prefix='initializer-field-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        clone=Path(folder);shutil.copytree(BINARY/'src',clone/'src');(clone/'build').mkdir();(clone/'cases/analysis').mkdir(parents=True)
        for name in ['c2.exe','c2.map']:shutil.copyfile(BINARY/'build'/name,clone/'build'/name)
        for name in ['record-initializers-analysis.json','record-initializers-disassembly.txt']:shutil.copyfile(BINARY/'cases/analysis'/name,clone/'cases/analysis'/name)
        snapshots={p.relative_to(clone):p.read_bytes()for p in clone.rglob('*')if p.is_file()}
        def source(name,before,after):
            p=clone/'src'/name;s=p.read_text();assert before in s;p.write_text(s.replace(before,after,1))
        def contract(change):
            p=clone/'cases/analysis/record-initializers-analysis.json';v=json.loads(p.read_text());before=json.dumps(v,sort_keys=True);change(v);assert before!=json.dumps(v,sort_keys=True);p.write_text(json.dumps(v,indent=2)+'\n')
        cases=[('wrong option guards initializer list',lambda c:source('code-records.c','    if (c2_output_name) {','    if (c2_codeview_types) {')),
            ('wrong symbol owner',lambda c:source('code-records.c','((C2InputSymbol *)records->symbol)->initializer','((C2InputSymbol *)records)->initializer')),
            ('wrong next link',lambda c:source('code-records.c','for (; records; records = records->next)\n            emit_initializer','for (; records; records = NULL)\n            emit_initializer')),
            ('initializer pointer replaced with symbol',lambda c:source('code-records.c','emit_initializer(((C2InputSymbol *)records->symbol)->initializer);','emit_initializer((C2InitRecord *)records->symbol);')),
            ('field offset changed',lambda c:contract(lambda v:v['field'].__setitem__('offset',54))),
            ('pointer width changed',lambda c:contract(lambda v:v['field'].__setitem__('width',2))),
            ('invented debug pointee',lambda c:contract(lambda v:v['field'].__setitem__('type','C2DebugType*'))),
            ('writer storage changed',lambda c:contract(lambda v:v['field'].__setitem__('writer_bytes','00'))),
            ('output name aliases debug flag',lambda c:contract(lambda v:v['guard'].__setitem__('original',0x48fa64))),
            ('new duplicate child identity',lambda c:contract(lambda v:v['child'].__setitem__('entry',0x44a6e5))),
            ('logical/source symbol alias changed',lambda c:next(r for r in c['driver']if r['name']=='@write_codeview_records@4').__setitem__('symbol','@write_record_initializers@8')),
            ('initializer layout guard omitted',lambda c:source('c2.h','offsetof(C2InputSymbol, initializer) == 50','offsetof(C2InputSymbol, initializer) == 54')),
            ('function alias size guard omitted',lambda c:source('c2.h','sizeof(C2FunctionSymbol) == 110','sizeof(C2FunctionSymbol) == 114'))]
        for name,change in cases:
            config=copy.deepcopy(c);change(config)
            try:verify_initializers(clone,config)
            except(AssertionError,KeyError,ValueError)as error:passed.append({'control':name,'reason':str(error)})
            else:raise AssertionError('initializer field mutation accepted: '+name)
            for p,raw in snapshots.items():(clone/p).write_bytes(raw)
        compiler=['clang','-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-DRECON_NATIVE_TEST','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),'-I'+str(BINARY/'src'),'-Werror=incompatible-pointer-types','-fsyntax-only']
        prefix='#include "c2.h"\ntypedef char Layout[(sizeof(C2InputSymbol)==110 && sizeof(C2FunctionSymbol)==110 && offsetof(C2InputSymbol,initializer)==50 && offsetof(C2FunctionSymbol,initializer)==50 && sizeof(((C2InputSymbol*)0)->initializer)==4 && sizeof(C2InitRecord)==28)?1:-1];\n'
        path=clone/'layout.c';path.write_text(prefix+'void good(C2InputSymbol *symbol,C2InitRecord *record) { symbol->initializer=record; emit_initializer(symbol->initializer); }\n');r=subprocess.run(compiler+[str(path)],capture_output=True);assert r.returncode==0,r.stderr.decode()
        path.write_text(prefix+'void bad(C2InputSymbol *symbol,C2Symbol *record) { symbol->initializer=record; }\n');r=subprocess.run(compiler+[str(path)],capture_output=True);assert r.returncode!=0;passed.append({'control':'unrelated symbol pointer cannot be initializer','reason':r.stderr.decode().splitlines()[0]})
        # Auxiliary nonNULL list checks exercise the reconstructed producer with
        # a typed observer child, not the still-masked target implementation.
        harness=clone/'nonzero.c';harness.write_text(r'''#include "code-records.h"
char *c2_output_name;
int c2_codeview_types;
static C2InitRecord *seen[2];
static unsigned count;
void __fastcall emit_initializer(C2InitRecord *record) { if (count < 2) seen[count] = record; ++count; }
int main(int argc, char **argv, char **environment) {
    C2InputSymbol first, second;
    C2InitRecord a, b;
    C2DebugLink one, two;
    first.initializer=&a; second.initializer=&b;
    one.next=&two; one.symbol=&first.symbol; two.next=0; two.symbol=&second.symbol;
    c2_output_name=0; c2_codeview_types=1;
    write_record_initializers(&one); if (count) return 1;
    c2_output_name="out.obj"; c2_codeview_types=0;
    write_record_initializers(0); if (count) return 2;
    write_record_initializers(&one);
    if (count != 2 || seen[0] != &a || seen[1] != &b) return 3;
    return 0;
}
''');exe=clone/'nonzero'
        command=['clang','-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-DRECON_NATIVE_TEST','-Wno-ignored-attributes','-O0','-g','-ffunction-sections','-fdata-sections','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),'-I'+str(BINARY/'src'),str(BINARY/'src/code-records.c'),str(harness),'-Wl,--gc-sections','-o',str(exe)]
        t=time.monotonic();r=subprocess.run(command,capture_output=True);assert r.returncode==0,r.stderr.decode();aux_build=time.monotonic()-t
        t=time.monotonic();r=subprocess.run([str(exe)],capture_output=True);assert r.returncode==0,r.stderr.decode();aux_execution=time.monotonic()-t
        passed.append({'control':'auxiliary actual producer distinct nonzero initializer/guard/list checks','reason':'NULL output+unrelated flag1 skips; nonNULL output+flag0 emits exact two distinct +50 initializer pointers in list order; NULL list skips. Typed observer replaces child only in isolated auxiliary ELF, zero targetbody credit.','build_seconds':aux_build,'execution_seconds':aux_execution,'harness_sha256':sha(harness.read_bytes())})
    a.output.write_text(json.dumps({'positive':positive,'count':len(passed),'rejected':passed,'seconds':time.monotonic()-start,'checker_sha256':sha(Path(__file__).read_bytes()),'validator_sha256':sha((BINARY/'scripts/record_data.py').read_bytes()),'scope':'Only same four-byte initializer field at pack2+50, unchanged110B aliases and existing28B root/emit target. Layout/type compile proof auxiliary; actual currentNULL list never reads these bytes, no default/pointee/body credit.'},indent=2)+'\n')
if __name__=='__main__':main()
