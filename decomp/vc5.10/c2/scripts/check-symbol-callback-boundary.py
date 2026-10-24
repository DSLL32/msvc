#!/usr/bin/env python3
"""Finite497cec callable ABI, source guards and compile layout controls."""
import argparse
import copy
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import time
import callback_evidence
from state import BINARY, ROOT, sha


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();begin=time.monotonic()
    config=json.loads((BINARY/'build/trial.json').read_text());graph=json.loads((BINARY/'build/graph-audit.json').read_text());positive=callback_evidence.verify(config,graph);passed=[]
    def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n')
    with tempfile.TemporaryDirectory(prefix='symbol-slot-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        root=Path(folder);clone=root/'clone';shutil.copytree(BINARY/'src',clone/'src');(clone/'build').mkdir();(clone/'cases/analysis').mkdir(parents=True)
        for name in ['c2.exe','c2.map']:shutil.copyfile(BINARY/'build'/name,clone/'build'/name)
        for name in ['output-callback-boundary.json','section-callback-storage-contract.json','directive-callback-contract.json','symbol-callback-contract.json','symbol-callback-disassembly.txt','symbol-callback-stack-disassembly.txt','symbol-callback-call-inventory.txt']:
            shutil.copyfile(BINARY/'cases/analysis'/name,clone/'cases/analysis'/name)
        callback_evidence.BINARY=clone
        try:
            assert callback_evidence.verify(config,graph)==positive
            def row(c,name):return next(r for r in c['stubs']if r['name']==name)
            def source(path,before,after):
                target=clone/path;old=target.read_text();assert before in old;new=old.replace(before,after,1);assert new!=old;target.write_text(new)
            def contract(c,change):
                path=clone/c['symbol_callback_contract'];v=json.loads(path.read_text());before=json.dumps(v,sort_keys=True);change(v);assert before!=json.dumps(v,sort_keys=True);write(path,v);c['symbol_callback_contract_sha256']=sha(path.read_bytes())
            cases=[('unknown second slot call',lambda c:source('src/main.c','#include <recon.h>','#include <recon.h>\nvoid bad(void) { output_slot_497de4.target(); }')),
                ('typed slot address escape',lambda c:source('src/main.c','#include <recon.h>','#include <recon.h>\nvoid bad(void) { void *p = &output_slot_497cec; }')),
                ('typed target direct escape',lambda c:source('src/main.c','#include <recon.h>','#include <recon.h>\nvoid bad(C2Symbol *p) { output_callback_41fa04(p); }')),
                ('fastcall convention erased',lambda c:source('src/output-callbacks.h','typedef void (__fastcall *C2SymbolWriter)','typedef void (__cdecl *C2SymbolWriter)')),
                ('symbol parameter erased',lambda c:source('src/output-callbacks.h','*C2SymbolWriter)(C2Symbol *symbol)','*C2SymbolWriter)(void *symbol)')),
                ('slot storage reverts to generic',lambda c:source('src/output-callbacks.c','C2SymbolCallback output_slot_497cec;','C2PendingCallback output_slot_497cec;')),
                ('consumer next-field changed',lambda c:source('src/code-records.c','pending_debug_symbols = pending_debug_symbols->next;','pending_debug_symbols = NULL;')),
                ('consumer symbol field changed',lambda c:source('src/code-records.c','output_slot_497cec.target(pending_debug_symbols->symbol);','output_slot_497cec.target((C2Symbol *)pending_debug_symbols);')),
                ('second consumer duplicate allowed line',lambda c:source('src/code-records.c','#include <recon.h>','#include <recon.h>\nvoid bad(void) {\noutput_slot_497cec.target(pending_debug_symbols->symbol);\n}')),
                ('pending typed ABI marker removed',lambda c:row(c,'_output_callback_41fa04').__setitem__('pending_signature',True)),
                ('pending target falsely accepted',lambda c:c['accepted'].append('_output_callback_41fa04')),
                ('pending body extent invented',lambda c:row(c,'_output_callback_41fa04').__setitem__('end',0x41fc96)),
                ('pending body flag cleared',lambda c:row(c,'_output_callback_4849fa').__setitem__('pending_extent',False)),
                ('target alias wrong canonical symbol',lambda c:row(c,'_output_callback_4849fa').__setitem__('symbol','@output_callback_41fa04@4')),
                ('prototype unknown cdecl',lambda c:contract(c,lambda v:v.__setitem__('type','void (__cdecl *)(C2Symbol *symbol)'))),
                ('slot alternative omitted',lambda c:contract(c,lambda v:v['slot']['alternatives'].pop())),
                ('caller inventory omitted',lambda c:contract(c,lambda v:v.__setitem__('callers',[]))),
                ('caller introduces stack argument',lambda c:contract(c,lambda v:v['callers'][0].__setitem__('stack_arguments',1))),
                ('caller result consumed',lambda c:contract(c,lambda v:v['callers'][1].__setitem__('result','used'))),
                ('forwarding caller omitted',lambda c:contract(c,lambda v:v.__setitem__('additional_direct_call',{}))),
                ('recursive caller target changed',lambda c:contract(c,lambda v:v['recursive_call'].__setitem__('target',0x41fa05))),
                ('normal return site omitted',lambda c:contract(c,lambda v:v['targets'][0]['normal_ret0_sites'].pop())),
                ('cold ABI owner shortened',lambda c:contract(c,lambda v:v['targets'][0]['code_chunks'][0].__setitem__(1,0x469d36))),
                ('unproven normal return cleanup',lambda c:contract(c,lambda v:v['stack_witness'].__setitem__('RET16_sites',[]))),
                ('canonical call neighboring slot',lambda c:contract(c,lambda v:v['canonical_callsite'].__setitem__('offset',21))),
                ('canonical pointer member wrong offset',lambda c:contract(c,lambda v:v['canonical_callsite'].__setitem__('load_bytes','8b4800')))]
            snapshots={str(p.relative_to(clone)):p.read_bytes()for p in clone.rglob('*')if p.is_file()}
            for name,change in cases:
                c=copy.deepcopy(config);change(c)
                try:callback_evidence.verify(c,graph)
                except(AssertionError,KeyError,ValueError)as error:passed.append({'control':name,'reason':str(error)})
                else:raise AssertionError('symbolcallback boundary mutation accepted: '+name)
                for n,data in snapshots.items():(clone/n).write_bytes(data)
        finally:callback_evidence.BINARY=BINARY
        compiler=['clang','-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-DRECON_NATIVE_TEST','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),'-I'+str(BINARY/'src'),'-Werror=incompatible-function-pointer-types','-fsyntax-only']
        prefix='#include "output-callbacks.h"\nstruct Aligned { char byte; C2SymbolCallback slot; };\ntypedef char Width[(sizeof(C2SymbolCallback)==4 && offsetof(C2SymbolCallback,target)==0 && offsetof(struct Aligned,slot)==4)?1:-1];\n'
        path=root/'layout.c';path.write_text(prefix+'void good(C2Symbol *p) { output_slot_497cec.target(p); }\n');r=subprocess.run(compiler+[str(path)],capture_output=True);assert r.returncode==0,r.stderr.decode()
        for name,body in [('typed slot too few arguments','void bad(void) { output_slot_497cec.target(); }'),('typed slot too many arguments','void bad(C2Symbol*p) { output_slot_497cec.target(p,p); }'),('other untyped slot remains a struct','void bad(C2Symbol*p) { output_slot_497de4(p); }')]:
            path.write_text(prefix+body+'\n');r=subprocess.run(compiler+[str(path)],capture_output=True);assert r.returncode!=0,'compiled negative accepted: '+name;passed.append({'control':name,'reason':r.stderr.decode().splitlines()[0]})
    write(a.output,{'positive':positive['symbol_callback'],'count':len(passed),'rejected':passed,'seconds':time.monotonic()-begin,'checker_sha256':sha(Path(__file__).read_bytes()),'validator_sha256':sha((BINARY/'scripts/callback_evidence.py').read_bytes()),'configuration_sha256':sha((BINARY/'build/trial.json').read_bytes()),'scope':'Exact497cec two-target fastcallC2Symbol* view;19 other slots retain their prior guards. Compile-positive32bit size4/alignment4/offset0 and exact one parameter, then3compile negatives.26 finite source/contract/ownership/ABI negatives; no target execution/body credit.'})

if __name__=='__main__':main()
