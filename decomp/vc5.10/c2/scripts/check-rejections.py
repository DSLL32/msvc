#!/usr/bin/env python3
"""Mutate real candidate proofs to verify fail-closed acceptance. Linux only."""
import argparse
import json
from pathlib import Path
import re
import shutil
import tempfile
from accept import preflight
from state import sha


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--function',required=True);p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--validation',type=Path)
    a=p.parse_args()
    control=preflight(a.function,a.original,a.native,a.validation)
    passed=[]
    def write_json(path,value):path.write_text(json.dumps(value,indent=2)+'\n')
    def json_at(path):return json.loads(path.read_text())
    def raw_changed(root,side,name):
        receipt=json_at(root/side/'receipt.json')
        fields={'events.txt':'events_sha256','state.json':'state_sha256'}
        if name in fields:receipt[fields[name]]=sha((root/side/name).read_bytes())
        elif name.endswith('.bin')and name.startswith(('stdout','stderr')):
            stream=name.removesuffix('.bin');data=(root/side/name).read_bytes();receipt[stream]={'bytes':len(data),'sha256':sha(data)}
        write_json(root/side/'receipt.json',receipt)
    def mutate_state(root,key,value):
        path=root/'n/state.json';state=json_at(path);state[key]=value;write_json(path,state);raw_changed(root,'n','state.json')
    def missing_frontier(root):
        path=root/'o/events.txt';path.write_text(re.sub(r'^trap .*\n','',path.read_text(),flags=re.M));raw_changed(root,'o','events.txt')
    def wrong_code(root):
        path=root/'o/events.txt';path.write_text(path.read_text().replace('trap 80000003 ','trap c0000005 '));raw_changed(root,'o','events.txt')
    def wrong_globals(root):
        path=root/'n/state.json';state=json_at(path);assert state['globals'];key=next(iter(state['globals']));state['globals'][key]^=1;write_json(path,state);raw_changed(root,'n','state.json')
    def audit_directory(root):return root/('v'if a.validation else'o')
    def audit_path(root):return audit_directory(root)/('receipt.json'if a.validation else'audit-receipt.json')
    def bind_audit(root):
        if not a.validation:
            path=root/'o/receipt.json';receipt=json_at(path);receipt['audit_receipt_sha256']=sha(audit_path(root).read_bytes());write_json(path,receipt)
    def prefix_only_audit(root):
        graphpath=audit_directory(root)/'graph-audit.json';graph=json_at(graphpath);graph['functions'].pop(a.function);graph['checked_functions']-=1;write_json(graphpath,graph)
        apath=audit_path(root);audit=json_at(apath);audit['report_sha256']=sha(graphpath.read_bytes());write_json(apath,audit)
        bind_audit(root)
    def wrong_trial(root):
        for side in ('o','n'):
            path=root/side/'configuration.json';data=json_at(path);data['accepted']=data['trial_provenance']['base_accepted'];write_json(path,data)
            rp=root/side/'receipt.json';r=json_at(rp);r['configuration_sha256']=sha(path.read_bytes());write_json(rp,r)
    def wrong_candidates(root):
        for side in ('o','n'):
            path=root/side/'receipt.json';receipt=json_at(path);receipt['candidates']=[];write_json(path,receipt)
    def wrong_audit(root,key):
        path=audit_path(root);audit=json_at(path);audit[key]='0'*64;write_json(path,audit)
        bind_audit(root)
    def raw_pc_consistent(root):
        path=root/'n/state.json';state=json_at(path);old=state['pc'];state['pc']+=4;write_json(path,state);raw_changed(root,'n','state.json')
        path=root/'n/gdb.stdout';raw=path.read_text();raw=re.sub(r'^(eip\s+)0x[0-9a-f]+',lambda m:m[1]+hex(old+4),raw,flags=re.M);path.write_text(raw)
        path=root/'n/receipt.json';receipt=json_at(path);receipt['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write_json(path,receipt)
    def changed_snapshot(root):
        path=root/'n/compiled-inputs.json';provenance=json_at(path);name=next(iter(provenance['inputs']));(root/'n/compiled-inputs'/name).write_bytes(b'changed')
    mutations=[
        ('missing raw frontier',missing_frontier),('wrong raw exception code',wrong_code),
        ('different native logical frontier',lambda r:mutate_state(r,'function','unrelated')),
        ('wrong native PC with internally matching state hash',lambda r:mutate_state(r,'pc',1)),
        ('wrong native PC with matching raw register observation',raw_pc_consistent),
        ('changed immutable compiled source',changed_snapshot),
        ('changed GDB state decoder input',lambda r:(r/'n/gdb-state.py').write_bytes(b'changed')),
        ('different runtime/global state',wrong_globals),
        ('different actual arguments',lambda r:mutate_state(r,'arguments',[])),
        ('inherited empty backend flags',lambda r:mutate_state(r,'MSC_CMD_FLAGS','')),
        ('different source-entry progression',lambda r:mutate_state(r,'trace',[])),
        ('old accepted prefix only was audited',prefix_only_audit),
        ('both internally matching trial snapshots omit candidate',wrong_trial),
        ('both receipt candidate lists omit declared trial',wrong_candidates),
        ('wrong validator tool',lambda r:wrong_audit(r,'tool_sha256')),
        ('wrong MAP',lambda r:wrong_audit(r,'rebuilt_map_sha256')),
        ('wrong original symbol index',lambda r:wrong_audit(r,'runtime_sha256')),
        ('wrong rebuilt symbol index',lambda r:wrong_audit(r,'rebuilt_runtime_sha256')),
        ('wrong SEH/chunk metadata',lambda r:wrong_audit(r,'metadata_sha256')),
        ('changed provider bytes',lambda r:(r/'o/loaded-msvcrt.bin').write_bytes(b'wrong provider')),
        ('changed masked executable',lambda r:(r/'o/C2.EXE').write_bytes(b'wrong executable')),
        ('changed partial stderr',lambda r:(r/'n/stderr.bin').write_bytes(b'wrong')),
        ('changed raw child command',lambda r:(r/'o/child-command-input.bin').write_bytes(b'wrong')),
        ('changed raw child environment',lambda r:(r/'o/child-environment-input.bin').write_bytes(b'MSC_CMD_FLAGS=\0\0')),
    ]
    with tempfile.TemporaryDirectory(prefix='candidate-negatives-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as scratch:
        for index,(name,change)in enumerate(mutations):
            root=Path(scratch)/str(index);shutil.copytree(a.original,root/'o');shutil.copytree(a.native,root/'n')
            if a.validation:shutil.copytree(a.validation.parent,root/'v')
            change(root)
            if a.validation:
                path=root/'v/receipt.json';validation=json_at(path);validation['original_execution_receipt_sha256']=sha((root/'o/receipt.json').read_bytes());write_json(path,validation)
            try:preflight(a.function,root/'o',root/'n',root/'v/receipt.json'if a.validation else None)
            except (AssertionError,KeyError,ValueError):passed.append(name)
            else:raise AssertionError('negative accepted: '+name)
    print(json.dumps({'function':a.function,'count':len(passed),'rejected':passed,'control_frontier':control[1]['matched'],'accepted':False},indent=2))

if __name__=='__main__':main()
