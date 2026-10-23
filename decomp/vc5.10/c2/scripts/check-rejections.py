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
    def changed_residual(root):
        path=audit_directory(root)/'import-residuals.json';values=json_at(path)
        values[0]['rebuilt']['lifetime_sha256']='0'*64;write_json(path,values)
        apath=audit_path(root);audit=json_at(apath);audit['import_residuals_sha256']=sha(path.read_bytes());write_json(apath,audit);bind_audit(root)
    source_audit=a.validation.parent if a.validation else a.original
    if json_at(source_audit/'import-residuals.json'):
        mutations.append(('changed cached-import evidence with matching receipt hash',changed_residual))
    current_state=json_at(a.native/'state.json')
    if 'data_state'in current_state:
        def changed_native_table(root):
            path=root/'n/state.json';state=json_at(path);state['data_state']['options'][0]['kind']^=1;write_json(path,state);raw_changed(root,'n','state.json')
        def changed_native_alias(root):
            path=root/'n/state.json';state=json_at(path);state['data_state']['options'][0]['value']+=4;write_json(path,state);raw_changed(root,'n','state.json')
        def changed_native_default(root):
            path=root/'n/state.json';state=json_at(path);state['data_state']['objects']['c2_cpu_generation']=3;write_json(path,state);raw_changed(root,'n','state.json')
        def changed_original_data(root):
            path=root/'o/data-state.bin';data=bytearray(path.read_bytes());data[8]^=1;path.write_bytes(data)
            path=root/'o/receipt.json';r=json_at(path);r['data_state_sha256']=sha((root/'o/data-state.bin').read_bytes());write_json(path,r)
        def changed_data_receipt(root):
            path=audit_directory(root)/'data-evidence.json';data=json_at(path);data['option_records']-=1;write_json(path,data)
            apath=audit_path(root);r=json_at(apath);r['data_evidence_sha256']=sha(path.read_bytes());write_json(apath,r);bind_audit(root)
        mutations += [('changed native option kind',changed_native_table),('different logical destination alias',changed_native_alias),
                      ('different initial CPU option',changed_native_default),('changed raw original table with matching hash',changed_original_data),
                      ('changed contextual data receipt with matching hash',changed_data_receipt)]
    if 'apply'in current_state:
        def changed_apply(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['apply'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
        mutations += [('wrong option-action table row',lambda r:changed_apply(r,'option',current_state['apply']['option']-12)),
                      ('wrong option-action copy mode',lambda r:changed_apply(r,'mode',1)),
                      ('lost current-input alias',lambda r:changed_apply(r,'current_alias',0))]
    if 'fatal'in current_state:
        def changed_fatal(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['fatal'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
        mutations += [('different fatal message',lambda r:changed_fatal(r,'message',8)),
                      ('different fatal argument',lambda r:changed_fatal(r,'argument','different')),
                      ('different fatal phase',lambda r:changed_fatal(r,'phase','p1')),
                      ('fatal argument loses current-input alias',lambda r:changed_fatal(r,'current_alias',0)),
                      ('fatal argument loses stored-pointer alias',lambda r:changed_fatal(r,'unexpected_alias',0))]
    if 'lookup'in current_state:
        def changed_lookup(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['lookup'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
        mutations += [('different lookup message ID',lambda r:changed_lookup(r,'message',1008)),
                      ('different lookup missing-message policy',lambda r:changed_lookup(r,'allow_missing',1)),
                      ('lookup loses stored option alias',lambda r:changed_lookup(r,'current_alias',0))]
    if 'message_load'in current_state:
        def changed_loader(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['message_load'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
        mutations += [('wrong resource buffer identity',lambda r:changed_loader(r,'buffer',current_state['message_load']['buffer']+1)),
                      ('wrong resource capacity',lambda r:changed_loader(r,'capacity',383)),
                      ('wrong recursive missing-message policy',lambda r:changed_loader(r,'allow_missing',0)),
                      ('initializer recursion guard was not set',lambda r:changed_loader(r,'initialized',0)),
                      ('changed uninitialized resource buffer',lambda r:changed_loader(r,'bytes','01'+current_state['message_load']['bytes'][2:]))]
    if 'diagnostic'in current_state:
        def changed_diagnostic(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['diagnostic'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
        mutations += [('wrong diagnostic category',lambda r:changed_diagnostic(r,'category',2000)),
                      ('wrong diagnostic format',lambda r:changed_diagnostic(r,'format','changed')),
                      ('format is not the shared message buffer',lambda r:changed_diagnostic(r,'format_alias',0)),
                      ('diagnostic vararg loses original input alias',lambda r:changed_diagnostic(r,'argument_alias',0)),
                      ('changed cached warning payload',lambda r:(r/'o/cache-message-301.bin').write_bytes(b'changed\0'))]
    if 'cached_message_id'in current_state:
        mutations.append(('different cached message ID',lambda r:mutate_state(r,'cached_message_id',302)))
    if 'cached_message_id'in current_state:
        def earlier_observation_only(root):
            path=root/'o/events.txt';data=path.read_text();head,separator,tail=data.rpartition('cached-message id=303')
            assert separator and 'cached-message id=303'in head
            path.write_text(head+'cached-message id=302'+tail);raw_changed(root,'o','events.txt')
        mutations.append(('earlier correct observation cannot hide wrong final trap input',earlier_observation_only))
    if 'helper_inputs_sha256'in json_at(a.original/'receipt.json'):
        mutations.append(('changed immutable capture helper executable',lambda r:(r/'o/capture-helper.exe').write_bytes(b'changed helper')))
        def changed_controller(root):
            manifest=json_at(root/'o/helper-inputs.json');name=next(n for n in manifest['inputs']if n.endswith('debug-run.c'))
            (root/'o/helper-inputs'/name).write_bytes(b'changed controller')
        mutations.append(('changed immutable compiled controller source',changed_controller))
    if 'format_prefix'in current_state:
        def changed_format(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['format_prefix'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
        mutations += [('wrong prefix character argument',lambda r:changed_format(r,'character',70)),
                      ('wrong prefix number argument',lambda r:changed_format(r,'number',1008)),
                      ('wrong output pointer within caller text',lambda r:changed_format(r,'output_offset',12))]
    if 'integer_format'in current_state:
        def changed_integer(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['integer_format'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
        mutations += [('different integer formatter value',lambda r:changed_integer(r,'value',1008)),
                      ('different integer formatter base',lambda r:changed_integer(r,'base',16)),
                      ('integer output pointer loses parent-buffer offset',lambda r:changed_integer(r,'output_offset',3))]
    if 'flags'in current_state:
        mutations.append(('NULL environment flags changed to empty string',lambda r:mutate_state(r,'flags','')))
    if 'command_state'in current_state:
        mutations.append(('different backend argc/argv state',lambda r:mutate_state(r,'command_state',{'argc':0,'argv':[]})))
        mutations.append(('changed original argument-vector bytes',lambda r:(r/'o/command-argv.bin').write_bytes(b'wrong\0\0')))
    if 'callback'in current_state:
        mutations.append(('different native callback identity',lambda r:mutate_state(r,'callback',1)))
    if current_state.get('completed'):
        skip={'missing raw frontier','wrong raw exception code','wrong native PC with internally matching state hash','wrong native PC with matching raw register observation','different runtime/global state'}
        mutations=[(name,fn)for name,fn in mutations if name not in skip]
        def original_event(root,change):
            path=root/'o/events.txt';path.write_text(change(path.read_text()));raw_changed(root,'o','events.txt')
        def native_event(root,change):
            path=root/'n/gdb.stdout';path.write_text(change(path.read_text()))
            receipt=json_at(root/'n/receipt.json');receipt['gdb_outputs']['gdb.stdout']=sha(path.read_bytes());write_json(root/'n/receipt.json',receipt)
        def omit_candidate(root):
            original_event(root,lambda text:re.sub(r'^entry 00454b04\n','',text,flags=re.M))
            native_event(root,lambda text:re.sub(r'^C2-SOURCE-ENTRY _c2_cleanup .*\n','',text,flags=re.M))
            path=root/'n/state.json';state=json_at(path);state['trace'].remove('_c2_cleanup');write_json(path,state);raw_changed(root,'n','state.json')
        mutations += [
            ('missing actual original process exit',lambda r:original_event(r,lambda t:re.sub(r'^exit .*\n','',t,flags=re.M))),
            ('different actual original process status',lambda r:original_event(r,lambda t:t.replace('exit 00000001','exit 00000000'))),
            ('missing actual native process exit',lambda r:native_event(r,lambda t:re.sub(r'^C2-PROCESS-EXIT .*\n','',t,flags=re.M))),
            ('duplicated actual native process exit',lambda r:native_event(r,lambda t:t+'C2-PROCESS-EXIT 1\n')),
            ('native completion flag is false',lambda r:mutate_state(r,'completed',False)),
            ('native completion status differs',lambda r:mutate_state(r,'exit_status',0)),
            ('unchecked NULL frontier cannot stand for completion',lambda r:mutate_state(r,'function',None)),
            ('original trap cannot stand for completion',lambda r:original_event(r,lambda t:t+'trap 80000003 00454b04 eip=00454b05\n')),
            ('native SIGTRAP cannot stand for completion',lambda r:native_event(r,lambda t:t+'SIGTRAP\n')),
            ('candidate omitted from both otherwise matching terminal traces',omit_candidate),
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
