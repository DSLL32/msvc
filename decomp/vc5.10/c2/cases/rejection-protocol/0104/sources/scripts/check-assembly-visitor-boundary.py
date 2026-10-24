#!/usr/bin/env python3
"""Finite assembly code-address pass guards; callback signatures stay unresolved."""
import argparse,copy,json,re,shutil,tempfile,time,importlib.util
from pathlib import Path
import callback_evidence as evidence
from data_evidence import reader
from state import BINARY,TARGET,sha

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();start=time.monotonic()
    c=json.loads((BINARY/'build/trial.json').read_text());g=json.loads((BINARY/'build/graph-audit.json').read_text())
    maps={m[1]:int(m[2],16)for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',(BINARY/'build/c2.map').read_text(),re.M)}
    original=reader(TARGET);rebuilt=reader(BINARY/'build/c2.exe');passed=[]
    with tempfile.TemporaryDirectory(prefix='assembly-address-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        clone=Path(folder);shutil.copytree(BINARY/'src',clone/'src');(clone/'cases/analysis').mkdir(parents=True)
        for name in ['assembly-visitor-contract.json','assembly-visitor-disassembly.txt']:shutil.copyfile(BINARY/'cases/analysis'/name,clone/'cases/analysis'/name)
        saved={p.relative_to(clone):p.read_bytes()for p in clone.rglob('*')if p.is_file()};evidence.BINARY=clone
        def source(file,before,after):
            p=clone/'src'/file;s=p.read_text();assert before in s;s=s.replace(before,after,1);p.write_text(s)
        def contract(config,change):
            p=clone/config['assembly_visitor_contract'];v=json.loads(p.read_text());before=json.dumps(v,sort_keys=True);change(v);assert before!=json.dumps(v,sort_keys=True);p.write_text(json.dumps(v,indent=2)+'\n');config['assembly_visitor_contract_sha256']=sha(p.read_bytes())
        def row(config,name):return next(r for r in config['stubs']if r['name']==name)
        def check(config,graph):return evidence.verify_assembly_addresses(config,graph,maps,original,rebuilt)
        try:
            positive=check(c,g)
            cases=[('address passed as string',lambda c,g:source('assembly-output.c','visit_symbols(0, assembly_callback_4894ca);','visit_symbols(0, "assembly_callback_4894ca");')),
                ('scope flag changed',lambda c,g:source('assembly-output.c','visit_symbols(1, assembly_callback_489596);','visit_symbols(0, assembly_callback_489596);')),
                ('callback directly invoked',lambda c,g:source('main.c','#include <recon.h>','#include <recon.h>\nvoid bad(void) { assembly_callback_4894ca(); }')),
                ('callback address escaped',lambda c,g:source('main.c','#include <recon.h>','#include <recon.h>\nvoid bad(void) { C2AssemblyVisitor p = assembly_callback_489596; }')),
                ('visitor called outside finite producer',lambda c,g:source('main.c','#include <recon.h>','#include <recon.h>\nvoid bad(void) { visit_symbols(0, assembly_callback_489693); }')),
                ('callback given invented formal',lambda c,g:source('output-callbacks.h','void assembly_callback_4894ca();','void assembly_callback_4894ca(C2Symbol *symbol);')),
                ('address pointer becomes struct',lambda c,g:source('output-callbacks.h','typedef void (__cdecl *C2AssemblyVisitor)();','typedef struct C2AssemblyVisitor { void (*target)(); } C2AssemblyVisitor;')),
                ('visitor formal erased',lambda c,g:source('output-callbacks.h','int local, C2AssemblyVisitor visitor','int local, void *visitor')),
                ('visitor loses genuine hardtrap',lambda c,g:source('stubs.c','void __fastcall visit_symbols(int local, C2AssemblyVisitor visitor)\n{\n    __asm { int 3 }\n}','void __fastcall visit_symbols(int local, C2AssemblyVisitor visitor)\n{\n}')),
                ('callback falsely accepted',lambda c,g:c['accepted'].append('_assembly_callback_4894ca')),
                ('unknown callback signature cleared',lambda c,g:row(c,'_assembly_callback_489596').__setitem__('pending_signature',False)),
                ('callback body extent invented',lambda c,g:row(c,'_assembly_callback_489693').__setitem__('end',0x489924)),
                ('visitor consumer falsely accepted',lambda c,g:c['accepted'].append('@visit_symbols@8')),
                ('visitor future signature hold removed',lambda c,g:row(c,'@visit_symbols@8').__setitem__('requires_resolved_signatures',[])),
                ('code reference omitted',lambda c,g:g['original_accepted_graphs']['_write_assembly_symbols']['references'].remove(0x48a251)),
                ('pointer encoding offset changed',lambda c,g:contract(c,lambda v:v['canonical_passes'][0].__setitem__('pointer_offset',13))),
                ('scope register encoding offset changed',lambda c,g:contract(c,lambda v:v['canonical_passes'][0].__setitem__('flag_offset',18))),
                ('call encoding offset changed',lambda c,g:contract(c,lambda v:v['canonical_passes'][0].__setitem__('call_offset',20))),
                ('one pass omitted',lambda c,g:contract(c,lambda v:v['passes'].pop())),
                ('cold extent shortened',lambda c,g:contract(c,lambda v:v['code_chunks'][0].__setitem__(1,0x469cbd)))]
            for name,change in cases:
                config=copy.deepcopy(c);graph=copy.deepcopy(g);change(config,graph)
                try:check(config,graph)
                except(AssertionError,KeyError,ValueError)as e:passed.append({'control':name,'reason':str(e)})
                else:raise AssertionError('mutation accepted: '+name)
                for p,raw in saved.items():(clone/p).write_bytes(raw)
        finally:evidence.BINARY=BINARY
    spec=importlib.util.spec_from_file_location('assembly_accept',BINARY/'scripts/accept.py');accept=importlib.util.module_from_spec(spec);spec.loader.exec_module(accept)
    base=copy.deepcopy(c);base['accepted'].remove('_write_assembly_symbols');base['trial']=['@visit_symbols@8']
    trial=copy.deepcopy(c);visitor=next(r for r in trial['stubs']if r['name']=='@visit_symbols@8');trial['stubs'].remove(visitor);trial['driver'].append(visitor);trial['accepted'].append(visitor['name'])
    accept.load=lambda:(base,None,None);accept.trial_configuration=lambda raw:(trial,None)
    try:accept.preflight('@visit_symbols@8',Path('/unreached-original'),Path('/unreached-native'))
    except AssertionError as error:
        assert str(error)=='callee would invoke unresolved callback signatures';passed.append({'control':'real acceptance preflight blocks unresolved visitor consumer','reason':str(error)})
    else:raise AssertionError('actual future consumer acceptance gate failed')
    a.output.write_text(json.dumps({'positive':positive,'count':len(passed),'rejected':passed,'seconds':time.monotonic()-start,'checker_sha256':sha(Path(__file__).read_bytes()),'validator_sha256':sha((BINARY/'scripts/callback_evidence.py').read_bytes()),'scope':'Six exact scalar EDX code-address passes, ECX scope flags, four named masked unresolved callbacks, no invocation/body credit; genuine future consumer hardtrap and explicit acceptance gate.'},indent=2)+'\n')
if __name__=='__main__':main()
