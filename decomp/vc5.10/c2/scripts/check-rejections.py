#!/usr/bin/env python3
"""Mutate real candidate proofs to verify fail-closed acceptance. Linux only."""
import argparse
import json
from pathlib import Path
import re
import shutil
import tempfile
import time
import tarfile
from accept import preflight
from state import BINARY, sha


def main():
    begin=time.monotonic()
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--function',required=True);p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--validation',type=Path)
    p.add_argument('--reviewed-baseline',type=Path,help='reuse only previously passed controls after reviewing unchanged protocol premises')
    p.add_argument('--affected',help='also rerun previously passed controls matching this expression')
    p.add_argument('--review-note',help='record why the reused protocol/control implementations remain applicable')
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
        ('different actual arguments',lambda r:mutate_state(r,'arguments',['unrecognized-test-input'])),
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
    if '0x491074'in current_state.get('globals',{}):
        def changed_machine_value(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['globals'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
        def missing_final_intrinsic_observation(root):
            path=root/'o/events.txt';data=path.read_text();at=data.rfind('observe %08x '%int(control[1]['original'],16));assert at>=0
            data=data[:at]+re.sub(r'^intrinsics-initialized .+\n','',data[at:],flags=re.M);path.write_text(data);raw_changed(root,'o','events.txt')
        mutations += [('machine selected-save register mask differs',lambda r:changed_machine_value(r,'0x491074',0xc8)),
                      ('machine call-clobber register mask differs',lambda r:changed_machine_value(r,'0x495f34',7)),
                      ('machine stack alignment differs',lambda r:changed_machine_value(r,'0x49745c',4)),
                      ('earlier intrinsic observation cannot replace final frontier observation',missing_final_intrinsic_observation)]
    if 'symbol_type_input'in current_state:
        def changed_type_input(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['symbol_type_input'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
            path=root/'n/gdb.stdout';raw=path.read_text();raw=re.sub(r'^C2-SYMBOL-TYPE-INPUT .*$',lambda m:'C2-SYMBOL-TYPE-INPUT '+json.dumps(state['symbol_type_input'],sort_keys=True),raw,flags=re.M);path.write_text(raw)
            path=root/'n/receipt.json';receipt=json_at(path);receipt['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write_json(path,receipt)
        def changed_type_raw(root,name,index):
            path=root/'o'/name;data=bytearray(path.read_bytes());data[index]^=1;path.write_bytes(data)
        formal_control=('type reader omits actual symbol formal',lambda r:changed_type_input(r,'formals',[]))if current_state['symbol_type_input']['formals']else('no-argument reader invents actual formal',lambda r:changed_type_input(r,'formals',[{'name':'symbol','type':'C2Symbol *'}]))
        mutations += [formal_control,
                      ('type reader actual symbol pool offset differs',lambda r:changed_type_input(r,'symbol_offset',552)),
                      ('type reader loses actual caller-symbol alias',lambda r:changed_type_input(r,'caller_symbol_alias',False)),
                      ('type reader caller record kind differs',lambda r:changed_type_input(r,'caller_kind',4)),
                      ('type reader caller retention differs',lambda r:changed_type_input(r,'retain_locals',1)),
                      ('type reader function record fields differ',lambda r:changed_type_input(r,'symbol_bytes','00'*110)),
                      ('type reader filename registry head offset differs',lambda r:changed_type_input(r,'filename_offset',540)),
                      ('type reader filename registry serial differs',lambda r:changed_type_input(r,'filename_serial',85)),
                      ('type reader filename name alias differs',lambda r:changed_type_input(r,'filename_name_offset',516)),
                      ('type reader filename list termination differs',lambda r:changed_type_input(r,'filename_next_null',False)),
                      ('type reader filename and template names lose shared pointer',lambda r:changed_type_input(r,'filename_name_alias',False)),
                      ('type reader filename record extent differs',lambda r:changed_type_input(r,'filename_size',16)),
                      ('type reader input scratch residual bytes cleared',lambda r:changed_type_input(r,'input_bytes',(b'mix_unsigned\0'+bytes(2045)).hex())),
                      ('type reader raw filename serial differs',lambda r:changed_type_raw(r,'filename-node.bin',0)),
                      ('type reader raw filename name pointer differs',lambda r:changed_type_raw(r,'filename-node.bin',4)),
                      ('type reader raw filename next pointer differs',lambda r:changed_type_raw(r,'filename-node.bin',8)),
                      ('type reader raw current record kind differs',lambda r:changed_type_raw(r,'function-symbol.bin',4)),
                      ('type reader raw current record untouched tail differs',lambda r:changed_type_raw(r,'function-symbol.bin',109)),
                      ('type reader rounded function padding differs',lambda r:changed_type_raw(r,'pool-block.bin',659))]
        if bytes.fromhex(current_state['symbol_type_input']['symbol_bytes'])[54]:
            def missing_decoded_type(root):
                data=bytearray.fromhex(current_state['symbol_type_input']['symbol_bytes']);data[54]&=0xf0
                changed_type_input(root,'symbol_bytes',data.hex())
            mutations.append(('decoded basic type bits lost while storage bits remain',missing_decoded_type))
    if 'filename_input'in current_state:
        def changed_filename_input(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['filename_input'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
            path=root/'n/gdb.stdout';raw=path.read_text();raw=re.sub(r'^C2-FILENAME-INPUT .*$',lambda m:'C2-FILENAME-INPUT '+json.dumps(state['filename_input'],sort_keys=True),raw,flags=re.M);path.write_text(raw)
            path=root/'n/receipt.json';receipt=json_at(path);receipt['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write_json(path,receipt)
        def changed_filename_pool(root,index):
            path=root/'o/pool-block.bin';data=bytearray(path.read_bytes());data[index]^=1;path.write_bytes(data)
        def changed_template_pointer(root):
            path=root/'o/symbol-template.bin';data=bytearray(path.read_bytes());data[24]^=1;path.write_bytes(data)
        mutations += [('filename registry omits actual symbol formal',lambda r:changed_filename_input(r,'formals',[])),
                      ('filename registry loses actual record alias',lambda r:changed_filename_input(r,'symbol_alias',False)),
                      ('filename copy returned wrong pool offset',lambda r:changed_filename_input(r,'name_pool_offset',516)),
                      ('filename copy returned different text',lambda r:changed_filename_input(r,'name','wrong.c')),
                      ('filename copy changed unrelated transient record',lambda r:changed_filename_input(r,'template_bytes','00'*64)),
                      ('filename copy changed original source input',lambda r:changed_filename_input(r,'input_bytes','00'*2058)),
                      ('filename copy changed decorated source input',lambda r:changed_filename_input(r,'decorated_bytes','00'*2059)),
                      ('filename raw returned pointer alias differs',changed_template_pointer),
                      ('filename raw allocated first byte differs',lambda r:changed_filename_pool(r,512)),
                      ('filename raw allocated NUL differs',lambda r:changed_filename_pool(r,534)),
                      ('filename raw rounded allocation padding differs',lambda r:changed_filename_pool(r,535))]
    if 'symbol_copy_input'in current_state:
        def changed_copy_input(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['symbol_copy_input'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
            path=root/'n/gdb.stdout';raw=path.read_text();raw=re.sub(r'^C2-SYMBOL-COPY-INPUT .*$',lambda m:'C2-SYMBOL-COPY-INPUT '+json.dumps(state['symbol_copy_input'],sort_keys=True),raw,flags=re.M);path.write_text(raw)
            path=root/'n/receipt.json';receipt=json_at(path);receipt['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write_json(path,receipt)
        def changed_copy_raw(root,index):
            path=root/'o/decorated-name.bin';data=bytearray(path.read_bytes());data[index]^=1;path.write_bytes(data)
        def wrong_name_counter(root):
            path=root/'n/state.json';state=json_at(path);state['globals']['0x491040']=494;write_json(path,state);raw_changed(root,'n','state.json')
        def changed_name_dispatch(root):
            path=audit_directory(root)/'data-evidence.json';data=json_at(path);data['name_dispatch'][0]['workspace_minimum_bytes']=2058;write_json(path,data)
            path=audit_path(root);receipt=json_at(path);receipt['data_evidence_sha256']=sha((audit_directory(root)/'data-evidence.json').read_bytes());write_json(path,receipt);bind_audit(root)
        mutations += [('name copier omits actual pool formal',lambda r:changed_copy_input(r,'formals',[{'name':'name','type':'char *'}])),
                      ('name copier loses actual workspace-plus-one alias',lambda r:changed_copy_input(r,'name_alias',False)),
                      ('name copier actual pool differs',lambda r:changed_copy_input(r,'pool',1)),
                      ('name copier caller pool differs',lambda r:changed_copy_input(r,'caller_pool',1)),
                      ('name copier caller symbol alias differs',lambda r:changed_copy_input(r,'caller_symbol_alias',False)),
                      ('name copier workspace extent differs',lambda r:changed_copy_input(r,'workspace_size',2058)),
                      ('name copier native decoration character differs',lambda r:changed_copy_input(r,'decorated_bytes','40'+current_state['symbol_copy_input']['decorated_bytes'][2:])),
                      ('name copier native untouched workspace tail differs',lambda r:changed_copy_input(r,'decorated_bytes',current_state['symbol_copy_input']['decorated_bytes'][:-2]+'01')),
                      ('name copier mutated original input scratch',lambda r:changed_copy_input(r,'input_bytes','00'*2058)),
                      ('name copier raw decoration character differs',lambda r:changed_copy_raw(r,0)),
                      ('name copier raw filename NUL differs',lambda r:changed_copy_raw(r,23)),
                      ('name copier raw untouched workspace tail differs',lambda r:changed_copy_raw(r,2058)),
                      ('name accounting omitted22 characters',wrong_name_counter),
                      ('name dispatch and workspace proof differs with rebound hash',changed_name_dispatch)]
    if 'symbol_intern_input'in current_state:
        def changed_intern_input(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['symbol_intern_input'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
            path=root/'n/gdb.stdout';raw=path.read_text();raw=re.sub(r'^C2-SYMBOL-INTERN-INPUT .*$',lambda m:'C2-SYMBOL-INTERN-INPUT '+json.dumps(state['symbol_intern_input'],sort_keys=True),raw,flags=re.M);path.write_text(raw)
            path=root/'n/receipt.json';receipt=json_at(path);receipt['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write_json(path,receipt)
        def changed_intern_raw(root,name,index):
            path=root/'o'/name;data=bytearray(path.read_bytes());data[index]^=1;path.write_bytes(data)
        def changed_intern_pool(root):
            path=root/'o/events.txt';path.write_text(path.read_text().replace('symbol-intern-input symbol=00497308 name=00496a00 pool=0','symbol-intern-input symbol=00497308 name=00496a00 pool=1'));raw_changed(root,'o','events.txt')
        def missing_intern_observation(root):
            path=root/'n/gdb.stdout';path.write_text(re.sub(r'^C2-SYMBOL-INTERN-INPUT .*\n','',path.read_text(),flags=re.M))
            path=root/'n/receipt.json';receipt=json_at(path);receipt['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write_json(path,receipt)
        formals=current_state['symbol_intern_input']['formals']
        mutations += [('name registration omits actual stack pool formal',lambda r:changed_intern_input(r,'formals',formals[:2])),
                      ('name registration loses actual symbol alias',lambda r:changed_intern_input(r,'symbol_alias',False)),
                      ('name registration loses actual scratch alias',lambda r:changed_intern_input(r,'name_alias',False)),
                      ('name registration actual pool differs',lambda r:changed_intern_input(r,'pool',1)),
                      ('name registration caller kind differs',lambda r:changed_intern_input(r,'kind',17)),
                      ('name registration caller retention differs',lambda r:changed_intern_input(r,'retain_locals',1)),
                      ('name registration transient record differs',lambda r:changed_intern_input(r,'template_bytes','00'*64)),
                      ('name registration returned filename differs',lambda r:changed_intern_input(r,'name_bytes','00'*2058)),
                      ('name registration native untouched scratch tail differs',lambda r:changed_intern_input(r,'name_bytes',current_state['symbol_intern_input']['name_bytes'][:-2]+'01')),
                      ('name registration raw returned filename differs',lambda r:changed_intern_raw(r,'symbol-name.bin',0)),
                      ('name registration raw terminator differs',lambda r:changed_intern_raw(r,'symbol-name.bin',22)),
                      ('name registration raw untouched scratch tail differs',lambda r:changed_intern_raw(r,'symbol-name.bin',2057)),
                      ('name registration raw untouched record tail differs',lambda r:changed_intern_raw(r,'symbol-template.bin',63)),
                      ('name registration original actual stack pool differs',changed_intern_pool),
                      ('name registration missing actual GDB observation',missing_intern_observation)]
    if 'symbol_name_input'in current_state:
        def changed_name_input(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['symbol_name_input'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
            path=root/'n/gdb.stdout';raw=path.read_text();raw=re.sub(r'^C2-SYMBOL-NAME-INPUT .*$',lambda m:'C2-SYMBOL-NAME-INPUT '+json.dumps(state['symbol_name_input'],sort_keys=True),raw,flags=re.M);path.write_text(raw)
            path=root/'n/receipt.json';receipt=json_at(path);receipt['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write_json(path,receipt)
        def changed_name_raw(root,name,index):
            path=root/'o'/name;data=bytearray(path.read_bytes());data[index]^=1;path.write_bytes(data)
        def changed_original_name_abi(root):
            path=root/'o/events.txt';text=path.read_text().replace('symbol-name-input buffer=00496a00 capacity=2058','symbol-name-input buffer=00496a01 capacity=2058');path.write_text(text);raw_changed(root,'o','events.txt')
        def missing_name_observation(root):
            path=root/'n/gdb.stdout';path.write_text(re.sub(r'^C2-SYMBOL-NAME-INPUT .*\n','',path.read_text(),flags=re.M))
            path=root/'n/receipt.json';receipt=json_at(path);receipt['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write_json(path,receipt)
        mutations += [('name reader omits actual capacity formal',lambda r:changed_name_input(r,'formals',[{'name':'buffer','type':'char *'}])),
                      ('name reader buffer loses global scratch alias',lambda r:changed_name_input(r,'buffer_alias',False)),
                      ('name reader actual capacity differs',lambda r:changed_name_input(r,'capacity',2057)),
                      ('name reader caller kind differs',lambda r:changed_name_input(r,'kind',17)),
                      ('name reader caller retention differs',lambda r:changed_name_input(r,'retain_locals',1)),
                      ('name reader decoded unsigned serial differs',lambda r:changed_name_input(r,'template_bytes','00'*64)),
                      ('name reader native untouched scratch tail differs',lambda r:changed_name_input(r,'name_bytes','00'*2057+'01')),
                      ('name reader raw decoded serial differs',lambda r:changed_name_raw(r,'symbol-template.bin',40)),
                      ('name reader raw untouched template tail differs',lambda r:changed_name_raw(r,'symbol-template.bin',63)),
                      ('name reader raw untouched scratch tail differs',lambda r:changed_name_raw(r,'symbol-name.bin',2057)),
                      ('name reader original actual buffer argument differs',changed_original_name_abi),
                      ('name reader missing actual GDB observation',missing_name_observation)]
    if 'symbol_serial_input'in current_state:
        def changed_serial_input(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['symbol_serial_input'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
            # Rebind the matching raw JSON too, so semantic guards themselves
            # must reject this control rather than only the duplicate view.
            path=root/'n/gdb.stdout';raw=path.read_text();raw=re.sub(r'^C2-SYMBOL-SERIAL-INPUT .*$',lambda m:'C2-SYMBOL-SERIAL-INPUT '+json.dumps(state['symbol_serial_input'],sort_keys=True),raw,flags=re.M);path.write_text(raw)
            path=root/'n/receipt.json';receipt=json_at(path);receipt['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write_json(path,receipt)
        def changed_serial_byte(root,name,index):
            path=root/'o'/name;data=bytearray(path.read_bytes());data[index]^=1;path.write_bytes(data)
        def missing_serial_observation(root):
            path=root/'n/gdb.stdout';path.write_text(re.sub(r'^C2-SYMBOL-SERIAL-INPUT .*\n','',path.read_text(),flags=re.M))
            path=root/'n/receipt.json';receipt=json_at(path);receipt['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write_json(path,receipt)
        def changed_record_dispatch(root):
            path=audit_directory(root)/'data-evidence.json';data=json_at(path);data['record_dispatch'][0]['kinds']=22;write_json(path,data)
            path=audit_path(root);receipt=json_at(path);receipt['data_evidence_sha256']=sha((audit_directory(root)/'data-evidence.json').read_bytes());write_json(path,receipt);bind_audit(root)
        mutations += [('unsigned reader invents actual formal parameter',lambda r:changed_serial_input(r,'formals',[{'name':'extra','type':'int'}])),
                      ('unsigned reader caller record kind differs',lambda r:changed_serial_input(r,'kind',17)),
                      ('unsigned reader caller retain-locals differs',lambda r:changed_serial_input(r,'retain_locals',1)),
                      ('unsigned reader template size differs',lambda r:changed_serial_input(r,'source_template_size',63)),
                      ('unsigned reader template kind differs',lambda r:changed_serial_input(r,'template_bytes','00'*64)),
                      ('unsigned reader native name buffer tail differs',lambda r:changed_serial_input(r,'name_bytes','00'*2057+'01')),
                      ('unsigned reader native fifth pool differs',lambda r:changed_serial_input(r,'fifth_pool','01'+'00'*11)),
                      ('unsigned reader raw template kind differs',lambda r:changed_serial_byte(r,'symbol-template.bin',4)),
                      ('unsigned reader raw untouched template tail differs',lambda r:changed_serial_byte(r,'symbol-template.bin',63)),
                      ('unsigned reader raw name scratch tail differs',lambda r:changed_serial_byte(r,'symbol-name.bin',2057)),
                      ('unsigned reader raw fifth pool tail differs',lambda r:changed_serial_byte(r,'source-name-pool.bin',11)),
                      ('record decoder existing frame-pointer flag alias differs',lambda r:changed_machine_value(r,'0x4973c0',0)),
                      ('unsigned reader missing actual GDB observation',missing_serial_observation),
                      ('record decoder dispatch content proof differs with rebound hash',changed_record_dispatch)]
    if 'symbol_record_input'in current_state:
        def changed_record_input(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['symbol_record_input'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
        def changed_raw_scope(root,index):
            path=root/'o/symbol-scope.bin';data=bytearray(path.read_bytes());data[index]^=1;path.write_bytes(data)
        def changed_dispatch(root):
            path=audit_directory(root)/'data-evidence.json';data=json_at(path);data['symbol_dispatch'][0]['blocks']=10;write_json(path,data)
            path=audit_path(root);receipt=json_at(path);receipt['data_evidence_sha256']=sha((audit_directory(root)/'data-evidence.json').read_bytes());write_json(path,receipt);bind_audit(root)
        mutations += [('symbol record retain-locals flag differs',lambda r:changed_record_input(r,'retain_locals',1)),
                      ('symbol record caller scope is not NULL',lambda r:changed_record_input(r,'caller_null_scope',False)),
                      ('symbol record caller retention differs',lambda r:changed_record_input(r,'caller_retention',1)),
                      ('symbol record scope loses actual global-table alias',lambda r:changed_record_input(r,'global_scope_alias',False)),
                      ('symbol record scope omits previous pointer',lambda r:changed_record_input(r,'scope_bytes',4096)),
                      ('symbol record buckets start at wrong offset',lambda r:changed_record_input(r,'bucket_offset',0)),
                      ('raw original scope previous pointer changed',lambda r:changed_raw_scope(r,0)),
                      ('raw original scope final bucket byte changed',lambda r:changed_raw_scope(r,4099)),
                      ('symbol dispatch content proof changed with matching receipt hash',changed_dispatch)]
    if 'global_symbols_input'in current_state:
        def changed_global_input(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['global_symbols_input'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
        def changed_original_scope(root):
            path=root/'o/events.txt';data=path.read_text();at=data.rfind('observe %08x '%int(control[1]['original'],16));assert at>=0
            data=data[:at]+data[at:].replace('global-symbols-input scope=00000000','global-symbols-input scope=00000001');path.write_text(data);raw_changed(root,'o','events.txt')
        mutations += [('global symbol reader scope is not NULL',lambda r:changed_global_input(r,'null_scope',False)),
                      ('global symbol reader active stream differs',lambda r:changed_global_input(r,'active_globals_alias',False)),
                      ('raw original global reader scope differs',changed_original_scope)]
    if 'sections'in current_state:
        def changed_sections(root,change):
            path=root/'n/state.json';state=json_at(path);change(state['sections']);write_json(path,state);raw_changed(root,'n','state.json')
        def changed_section_field(root,index,offset):
            def change(value):
                data=bytearray.fromhex(value['rows'][index]['bytes']);data[offset]^=1;value['rows'][index]['bytes']=data.hex()
            changed_sections(root,change)
        def changed_final_node(root,index,offset):
            path=root/'o/sections.bin';data=bytearray(path.read_bytes());data[index*86+offset]^=1;path.write_bytes(data)
        def changed_final_hash(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['symbol_hash']['section_offsets'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
        mutations += [('final sections omit TLS record',lambda r:changed_sections(r,lambda v:v['rows'].pop())),
                      ('final section serial differs',lambda r:changed_section_field(r,4,40)),
                      ('final section code alignment differs',lambda r:changed_section_field(r,0,62)),
                      ('final section default alignment differs',lambda r:changed_section_field(r,1,62)),
                      ('final section attribute differs',lambda r:changed_section_field(r,4,66)),
                      ('final section combine differs',lambda r:changed_section_field(r,2,67)),
                      ('final section target kind differs',lambda r:changed_section_field(r,2,68)),
                      ('final section output order differs',lambda r:changed_section_field(r,4,78)),
                      ('final section untouched tail differs',lambda r:changed_section_field(r,3,85)),
                      ('final section name differs',lambda r:changed_sections(r,lambda v:v['rows'][0].__setitem__('name','TEXT'))),
                      ('final section class differs',lambda r:changed_sections(r,lambda v:v['rows'][1].__setitem__('class_name','BSS'))),
                      ('final CONST name and class lose shared address',lambda r:changed_sections(r,lambda v:v['rows'][2].__setitem__('class_pointer',v['rows'][2]['class_pointer']+4))),
                      ('final distinct class pointers collapse',lambda r:changed_sections(r,lambda v:v['rows'][1].__setitem__('class_pointer',v['rows'][0]['class_pointer']))),
                      ('final section pending context alias differs',lambda r:changed_sections(r,lambda v:v['rows'][3].__setitem__('context_alias',False))),
                      ('final section allocation alias differs',lambda r:changed_sections(r,lambda v:v['rows'][4].__setitem__('pool_offset',72))),
                      ('final section list tail loses node identity',lambda r:changed_sections(r,lambda v:v.__setitem__('tail_alias',False))),
                      ('final section output globals reordered',lambda r:changed_sections(r,lambda v:v.__setitem__('outputs',[1,0,2,3,None,None,4]))),
                      ('final debug section global is unexpectedly nonnull',lambda r:changed_sections(r,lambda v:v['output_nulls'].__setitem__(4,False))),
                      ('final section opaque anchor alias differs',lambda r:changed_sections(r,lambda v:v.__setitem__('context_identity_alias',False))),
                      ('final section default alignment global differs',lambda r:changed_sections(r,lambda v:v.__setitem__('default_alignment',8))),
                      ('final hash bucket points to wrong allocation',lambda r:changed_final_hash(r,'9',72)),
                      ('final hash contains extra bucket',lambda r:changed_final_hash(r,'1023',72)),
                      ('original final section context identity differs',lambda r:changed_final_node(r,1,36)),
                      ('original final section list terminator differs',lambda r:changed_final_node(r,4,50)),
                      ('original final section string changed',lambda r:(r/'o/section-name-4.bin').write_bytes(b'TLS\0')),
                      ('original final section addresses changed',lambda r:(r/'o/section-addresses.bin').write_bytes(bytes(20)))]
    if 'section_defaults'in current_state:
        def changed_section_defaults(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['section_defaults'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
        def changed_original_list(root):
            path=root/'o/section-list.bin';data=bytearray(path.read_bytes());data[4]^=4;path.write_bytes(data)
        def changed_unmodeled_alias(root):
            import struct
            path=root/'o/unmodeled-original-data.bin';data=bytearray(path.read_bytes());struct.pack_into('<I',data,0x497f58-0x48e000,0);path.write_bytes(data)
            receipt=json_at(root/'o/receipt.json');receipt['unmodeled_original_data']['sha256']=sha(data);write_json(root/'o/receipt.json',receipt)
        mutations += [('section default node untouched bytes differ',lambda r:changed_section_defaults(r,'bytes',current_state['section_defaults']['bytes'][:-2]+'01')),
                      ('section default name differs',lambda r:changed_section_defaults(r,'name','_DATA')),
                      ('section default class differs',lambda r:changed_section_defaults(r,'class_name','DATA')),
                      ('section default global name alias differs',lambda r:changed_section_defaults(r,'name_alias',False)),
                      ('section default allocated node alias differs',lambda r:changed_section_defaults(r,'pool_offset',8)),
                      ('section default output code pointer alias differs',lambda r:changed_section_defaults(r,'code_section_alias',False)),
                      ('section default other output changed prematurely',lambda r:changed_section_defaults(r,'other_sections',[1]+[0]*5)),
                      ('section default list head loses argument alias',lambda r:changed_section_defaults(r,'head_alias',False)),
                      ('section default list tail loses argument alias',lambda r:changed_section_defaults(r,'tail_alias',False)),
                      ('section default pending context loses identity alias',lambda r:changed_section_defaults(r,'context_identity_alias',False)),
                      ('original section default list tail differs',changed_original_list),
                      ('original unmodeled snapshot altered',lambda r:(r/'o/unmodeled-original-data.bin').write_bytes(b'changed')),
                      ('original pending context pointer differs with matching snapshot hash',changed_unmodeled_alias)]
    if 'section_linked'in current_state:
        def changed_linked_section(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['section_linked'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
        def changed_bucket(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['symbol_hash'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
        def original_bucket(root):
            path=root/'o/symbol-hash.bin';data=bytearray(path.read_bytes());data[20]^=4;path.write_bytes(data)
        mutations += [('linked section untouched node bytes differ',lambda r:changed_linked_section(r,'bytes',current_state['section_linked']['bytes'][:-2]+'01')),
                      ('linked section name differs',lambda r:changed_linked_section(r,'name','_DATA')),
                      ('linked section class differs',lambda r:changed_linked_section(r,'class_name','DATA')),
                      ('linked section global-name alias differs',lambda r:changed_linked_section(r,'name_alias',False)),
                      ('linked section pool alias differs',lambda r:changed_linked_section(r,'pool_offset',8)),
                      ('linked section outputs assigned prematurely',lambda r:changed_linked_section(r,'initial_sections',[1]+[0]*6)),
                      ('linked hash wrong bucket selected',lambda r:changed_bucket(r,'section_slot',6)),
                      ('linked hash pointer loses actual section alias',lambda r:changed_bucket(r,'section_alias',False)),
                      ('linked hash untouched slots differ',lambda r:changed_bucket(r,'normalized_sha256','0'*64)),
                      ('original linked bucket pointer differs',original_bucket)]
    if 'section_symbol'in current_state:
        def changed_section_symbol(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['section_symbol'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
        def changed_node_byte(root,index):
            path=root/'o/section-node.bin';data=bytearray(path.read_bytes());data[index]^=1;path.write_bytes(data)
        mutations += [('section symbol local-table selector differs',lambda r:changed_section_symbol(r,'local',1)),
                      ('section symbol untouched tail differs',lambda r:changed_section_symbol(r,'bytes',current_state['section_symbol']['bytes'][:-2]+'01')),
                      ('section symbol name differs',lambda r:changed_section_symbol(r,'name','_DATA')),
                      ('section symbol class differs',lambda r:changed_section_symbol(r,'class_name','DATA')),
                      ('section symbol name global alias differs',lambda r:changed_section_symbol(r,'name_alias',False)),
                      ('section symbol pool allocation alias differs',lambda r:changed_section_symbol(r,'pool_offset',8)),
                      ('section symbol premature output assignment',lambda r:changed_section_symbol(r,'initial_sections',[1]+[0]*6)),
                      ('original section symbol serial differs',lambda r:changed_node_byte(r,40)),
                      ('original section symbol untouched tail differs',lambda r:changed_node_byte(r,85))]
    if 'section_constructor'in current_state:
        def changed_section(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['section_constructor'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
        mutations += [('section constructor name differs',lambda r:changed_section(r,'name','_DATA')),
                      ('section constructor class differs',lambda r:changed_section(r,'class_name','DATA')),
                      ('section constructor serial differs',lambda r:changed_section(r,'serial',2)),
                      ('section constructor name loses global alias',lambda r:changed_section(r,'name_alias',False)),
                      ('section constructor initial output globals differ',lambda r:changed_section(r,'initial_sections',[1]+[0]*6)),
                      ('original section constructor name bytes differ',lambda r:(r/'o/section-name.bin').write_bytes(b'_DATA\0')),
                      ('original section constructor class bytes differ',lambda r:(r/'o/section-class.bin').write_bytes(b'DATA\0')),
                      ('original section constructor output globals differ',lambda r:(r/'o/section-globals.bin').write_bytes(b'\1'+b'\0'*27))]
    if 'intrinsic_table'in current_state:
        def changed_intrinsic(root,index,key,value):
            path=root/'n/state.json';state=json_at(path);state['intrinsic_table'][index][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
        def shortened_intrinsics(root):
            path=root/'n/state.json';state=json_at(path);state['intrinsic_table'].pop();write_json(path,state);raw_changed(root,'n','state.json')
        def wrong_original_intrinsic_tail(root):
            path=root/'o/intrinsic-table.bin';data=bytearray(path.read_bytes());data[-1]=1;path.write_bytes(data)
        def wrong_intrinsic_static_receipt(root):
            path=audit_directory(root)/'data-evidence.json';data=json_at(path);data['intrinsics']['text_identities']=121;write_json(path,data)
            path=audit_path(root);receipt=json_at(path);receipt['data_evidence_sha256']=sha((audit_directory(root)/'data-evidence.json').read_bytes());write_json(path,receipt);bind_audit(root)
        mutations += [('intrinsic result type differs',lambda r:changed_intrinsic(r,3,'result_type',0x4008)),
                      ('intrinsic runtime flags differ',lambda r:changed_intrinsic(r,340,'flags',0)),
                      ('intrinsic argument signature differs',lambda r:changed_intrinsic(r,23,'argument_signature',0)),
                      ('intrinsic command recipe bytes differ',lambda r:changed_intrinsic(r,3,'commands_bytes','4400')),
                      ('intrinsic FF runtime alias separator differs',lambda r:changed_intrinsic(r,23,'runtime_name_bytes','616d6f6400666d6f6400')),
                      ('intrinsic shared runtime address differs',lambda r:changed_intrinsic(r,4,'runtime_name',current_state['intrinsic_table'][4]['runtime_name']+4)),
                      ('intrinsic distinct empty addresses collapsed',lambda r:changed_intrinsic(r,7,'runtime_name',current_state['intrinsic_table'][0]['commands'])),
                      ('intrinsic reserved type field overwritten',lambda r:changed_intrinsic(r,0,'reserved_type',1)),
                      ('intrinsic reserved tail field overwritten',lambda r:changed_intrinsic(r,343,'reserved_flags',1)),
                      ('intrinsic final default row omitted',shortened_intrinsics),
                      ('original intrinsic final reserved byte changed',wrong_original_intrinsic_tail),
                      ('intrinsic model capture changed',lambda r:(r/'n/intrinsic-model.json').write_bytes(b'{}')),
                      ('intrinsic static field/alias receipt differs',wrong_intrinsic_static_receipt)]
    if 'extended_results'in current_state:
        def changed_extended_results(root,change):
            path=root/'n/state.json';state=json_at(path);change(state['extended_results']);write_json(path,state);raw_changed(root,'n','state.json')
            path=root/'n/gdb.stdout';raw=path.read_text()
            for label,values in [('C2-STORE-ABI-ENTRY',state['extended_results']['entries']),('C2-STORE-STEP',state['extended_results']['steps']),('C2-EXTENDED-RESULTS',[state['extended_results']])]:
                rows=iter(values)
                raw=re.sub(r'^'+label+r' .+$',lambda m:label+' '+json.dumps(next(rows),sort_keys=True),raw,flags=re.M)
            path.write_text(raw)
            path=root/'n/receipt.json';receipt=json_at(path);receipt['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write_json(path,receipt)
        def original_pop_missing(root):
            import struct
            path=root/'o/x87-after.bin';data=bytearray(path.read_bytes());struct.pack_into('<I',data,4,0xffff3800);struct.pack_into('<I',data,8,0xffff7fff);path.write_bytes(data)
        def original_kind_changed(root):
            path=root/'o/extended-after.bin';data=bytearray(path.read_bytes());data[10]=0;path.write_bytes(data)
        mutations += [('stored real-one payload differs',lambda r:changed_extended_results(r,lambda v:v['records'].__setitem__(1,'00'*12))),
                      ('stored real-two kind differs',lambda r:changed_extended_results(r,lambda v:v['records'].__setitem__(2,v['records'][2][:20]+'0000'))),
                      ('FSTP writes past ten-byte payload',lambda r:changed_extended_results(r,lambda v:v['steps'][1].__setitem__('payload','00'*12))),
                      ('FSTP fails to pop its x87 input',lambda r:changed_extended_results(r,lambda v:v['steps'][1].__setitem__('tag',0x7fff))),
                      ('naked tracing saves debugger TF',lambda r:changed_extended_results(r,lambda v:v['entries'][0].__setitem__('saved_flags',v['entries'][0]['saved_flags']|0x100))),
                      ('store uses debugger scratch FIP',lambda r:changed_extended_results(r,lambda v:v['steps'][1].__setitem__('instruction',1))),
                      ('tracing changes nonzero incoming raw80 value',lambda r:changed_extended_results(r,lambda v:v['entries'][1].__setitem__('registers',['00'*10]*8))),
                      ('store before/after instruction identity differs',lambda r:changed_extended_results(r,lambda v:v['steps'][1].__setitem__('pc',v['steps'][1]['pc']+1))),
                      ('original FSTP stack-pop evidence differs',original_pop_missing),
                      ('original store changed caller kind',original_kind_changed)]
    if 'extended_store'in current_state:
        def changed_x87(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['extended_store'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
            path=root/'n/gdb.stdout';raw=re.sub(r'^C2-STORE-ABI-STOP .+$','C2-STORE-ABI-STOP '+json.dumps(state['extended_store'],sort_keys=True),path.read_text(),flags=re.M);path.write_text(raw)
            path=root/'n/receipt.json';receipt=json_at(path);receipt['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write_json(path,receipt)
        mutations += [('x87 caller kind bytes differ',lambda r:changed_x87(r,'destination','00'*12)),
                      ('x87 source lost caller local alias',lambda r:changed_x87(r,'source_alias',False)),
                      ('x87 control word differs',lambda r:changed_x87(r,'control',0x37f)),
                      ('x87 stack TOP/status differs',lambda r:changed_x87(r,'status',0)),
                      ('x87 tag marks wrong live register',lambda r:changed_x87(r,'tag',0xffff)),
                      ('x87 raw80 value differs',lambda r:changed_x87(r,'registers',['01'+'00'*9]+['00'*10]*7)),
                      ('x87 FILD instruction identity differs',lambda r:changed_x87(r,'instruction',current_state['extended_store']['instruction']+1)),
                      ('naked tracing changed ESI',lambda r:changed_x87(r,'pointer',current_state['extended_store']['pointer']+4)),
                      ('naked tracing changed x87 source pointer',lambda r:changed_x87(r,'data_pointer',current_state['extended_store']['data_pointer']+4))]
    if 'real_from_int'in current_state:
        def changed_real_input(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['real_from_int'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
        mutations += [('constant initializer integer differs',lambda r:changed_real_input(r,'value',1)),
                      ('constant initializer output identity differs',lambda r:changed_real_input(r,'output_is_zero',False)),
                      ('constant input does not alias caller local',lambda r:changed_real_input(r,'caller_local_alias',False)),
                      ('constant initial real record differs',lambda r:changed_real_input(r,'records',['01'+'00'*11,'00'*12,'00'*12]))]
    if 'pool_result'in current_state:
        def changed_pool_result(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['pool_result'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
        def changed_pool_byte(root,side,index):
            if side=='original':
                path=root/'o/pool-block.bin';data=bytearray(path.read_bytes());data[index]^=1;path.write_bytes(data)
            else:
                path=root/'n/state.json';state=json_at(path);data=bytearray.fromhex(state['pool_result']['payload']);data[index]^=1;state['pool_result']['payload']=data.hex();write_json(path,state);raw_changed(root,'n','state.json')
        mutations += [('pool cursor offset differs',lambda r:changed_pool_result(r,'cursor_offset',76)),
                      ('pool remaining capacity differs',lambda r:changed_pool_result(r,'remaining',4028)),
                      ('node result points at wrong block offset',lambda r:changed_pool_result(r,'types_offset',12)),
                      ('pool block header capacity differs',lambda r:changed_pool_result(r,'capacity',4096)),
                      ('pool tail was not zeroed',lambda r:changed_pool_byte(r,'native',4087)),
                      ('node kind was not stored',lambda r:changed_pool_byte(r,'native',4)),
                      ('original raw pool tail changed',lambda r:changed_pool_byte(r,'original',4095))]
    if 'pool_allocate'in current_state:
        def changed_pool_argument(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['pool_allocate'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
        def changed_pool_count(root):
            path=root/'n/state.json';state=json_at(path);state['globals']['0x491040']=68;write_json(path,state);raw_changed(root,'n','state.json')
        def changed_node_dispatch(root):
            path=audit_directory(root)/'data-evidence.json';data=json_at(path);data['node_size_dispatch'][0]['cases'][0]=68;write_json(path,data)
            path=audit_path(root);receipt=json_at(path);receipt['data_evidence_sha256']=sha((audit_directory(root)/'data-evidence.json').read_bytes());write_json(path,receipt);bind_audit(root)
        mutations += [('pool allocation size differs',lambda r:changed_pool_argument(r,'bytes',68)),
                      ('pool allocator index differs',lambda r:changed_pool_argument(r,'pool',1)),
                      ('pool allocator saved kind differs',lambda r:changed_pool_argument(r,'caller_kind',2)),
                      ('node bytes accounted incorrectly',changed_pool_count),
                      ('node dispatch semantic table differs with rebound hash',changed_node_dispatch)]
    if 'node_size'in current_state:
        def changed_size_argument(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['node_size'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
        def changed_node_counter(root):
            path=root/'n/state.json';state=json_at(path);state['globals']['0x491040']=1;write_json(path,state);raw_changed(root,'n','state.json')
        mutations += [('node size kind differs',lambda r:changed_size_argument(r,'kind',2)),
                      ('node allocator saved kind differs',lambda r:changed_size_argument(r,'caller_kind',2)),
                      ('node allocator saved pool differs',lambda r:changed_size_argument(r,'pool',1)),
                      ('node byte counter changed before sizing',changed_node_counter)]
    if 'allocate_node'in current_state:
        def changed_node_argument(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['allocate_node'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
        def unmasked_tail_target(root):
            from state import load,offset
            _,_,inventory=load();path=root/'o/C2.EXE';data=bytearray(path.read_bytes());at=offset(inventory,0x40d507);assert data[at]==0xcc;data[at]=0x53;path.write_bytes(data)
            path=root/'o/receipt.json';receipt=json_at(path);receipt['executable_sha256']=sha((root/'o/C2.EXE').read_bytes());write_json(path,receipt)
        def falsely_credited_tail_target(root):
            path=audit_directory(root)/'graph-audit.json';graph=json_at(path);graph['functions']['@c2_allocate_node@8']={'edges':[],'imports':[]};graph['checked_functions']+=1;write_json(path,graph)
            path=audit_path(root);receipt=json_at(path);receipt['report_sha256']=sha((audit_directory(root)/'graph-audit.json').read_bytes());write_json(path,receipt);bind_audit(root)
        mutations += [('fastcall node kind differs',lambda r:changed_node_argument(r,'kind',2)),
                      ('fastcall allocation pool differs',lambda r:changed_node_argument(r,'pool',1)),
                      ('extent-known unaccepted application callee was unmasked',unmasked_tail_target),
                      ('extent-known callee falsely credited as reconstructed',falsely_credited_tail_target)]
    if 'binary_snapshot_sha256'in json_at(a.native/'receipt.json'):
        mutations += [('retained native ELF artifact changed',lambda r:(r/'n/native.elf').write_bytes(b'changed ELF')),
                      ('retained Win32 reconstruction changed',lambda r:(r/'o/rebuilt.exe').write_bytes(b'changed PE')),
                      ('retained original-build MAP changed',lambda r:(r/'o/rebuilt.map').write_bytes(b'changed MAP'))]
    if 'symbol_hash'in current_state:
        def wrong_hash_extent(root):
            path=root/'n/state.json';state=json_at(path);state['symbol_hash']['slots']=1023;write_json(path,state);raw_changed(root,'n','state.json')
        def wrong_hash_tail(root):
            path=root/'o/symbol-hash.bin';data=bytearray(path.read_bytes());data[-1]=1;path.write_bytes(data)
        mutations += [('symbol hash reset omits final pointer slot',wrong_hash_extent),
                      ('raw original hash has an uncleared final byte',wrong_hash_tail)]
    if current_state.get('globals',{}).get('0x497df8')==146:
        def wrong_decoded_count(root):
            path=root/'n/state.json';state=json_at(path);state['globals']['0x497df8']=147;write_json(path,state);raw_changed(root,'n','state.json')
        mutations.append(('decoded IL symbol count differs from master header',wrong_decoded_count))
    if 'il_word_result'in current_state:
        mutations.append(('decoded machine differs from original AX result',lambda r:mutate_state(r,'il_word_result',0x2386)))
    if 'valid_buffer'in current_state.get('header_streams',{}):
        def changed_buffer(root):
            path=root/'n/state.json';state=json_at(path);buffer=state['header_streams']['valid_buffer'];state['header_streams']['valid_buffer']='00'+buffer[2:];write_json(path,state);raw_changed(root,'n','state.json')
        def changed_buffer_count(root):
            import struct
            path=root/'n/state.json';state=json_at(path);words=list(struct.unpack('<8I',bytes.fromhex(state['header_streams']['globals'])));words[1]-=1
            state['header_streams']['globals']=struct.pack('<8I',*words).hex();write_json(path,state);raw_changed(root,'n','state.json')
        mutations += [('native valid IL buffer differs from captured input',changed_buffer),
                      ('post-refill count does not match consumed byte',changed_buffer_count),
                      ('raw original valid buffer changed',lambda r:(r/'o/il-buffer.bin').write_bytes(bytes(271)))]
    if current_state.get('header_streams',{}).get('active_index')is not None:
        def wrong_active_il(root):
            path=root/'n/state.json';state=json_at(path);state['header_streams']['active_index']=3;write_json(path,state);raw_changed(root,'n','state.json')
        def wrong_header_flag(root):
            path=root/'n/state.json';state=json_at(path);state['globals']['0x490b7c']=1;write_json(path,state);raw_changed(root,'n','state.json')
        mutations += [('header byte reader selects the wrong IL stream',wrong_active_il),
                      ('header state changed before any bytes are read',wrong_header_flag)]
    if 'header_streams'in current_state:
        def changed_header_stream(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['header_streams'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
        mutations += [('input FILE loses original iob identity',lambda r:changed_header_stream(r,'input_index',4)),
                      ('two IL streams alias one FILE',lambda r:changed_header_stream(r,'distinct',False)),
                      ('new input stream has already consumed data',lambda r:changed_header_stream(r,'input','01'+current_state['header_streams']['input'][2:])),
                      ('changed raw canonical FILE layout',lambda r:(r/'o/header-streams.bin').write_bytes(bytes(64)))]
    if 'intermediate_open'in current_state:
        def changed_open(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['intermediate_open'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
        mutations += [('wrong intermediate suffix argument',lambda r:changed_open(r,'suffix','sy')),
                      ('wrong intermediate stream mode',lambda r:changed_open(r,'mode','w')),
                      ('intermediate base loses environment-pointer alias',lambda r:changed_open(r,'base_alias',False)),
                      ('intermediate opener x87 state differs',lambda r:changed_open(r,'x87_control',0))]
    if 'alignment_option'in current_state:
        def changed_alignment(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['alignment_option'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
        mutations += [('absent alignment is not an empty string',lambda r:changed_alignment(r,'alignment','')),
                      ('final option loses captured environment offset',lambda r:changed_alignment(r,'current_offset',110)),
                      ('completed parser has wrong static argv position',lambda r:changed_alignment(r,'argv_offset',60)),
                      ('last argv value loses current-option pointer alias',lambda r:changed_alignment(r,'argv_value_alias',False))]
    if 'integer_option'in current_state:
        def changed_integer_option(root,key,value):
            path=root/'n/state.json';state=json_at(path);state['integer_option'][key]=value;write_json(path,state);raw_changed(root,'n','state.json')
        mutations += [('integer input loses bound environment offset',lambda r:changed_integer_option(r,'input_offset',43)),
                      ('integer operand loses original pointer alias',lambda r:changed_integer_option(r,'operand_alias',False)),
                      ('copied argument vector has wrong static offset',lambda r:changed_integer_option(r,'argv_offset',28)),
                      ('copied argument does not alias environment token',lambda r:changed_integer_option(r,'argv_value_alias',False))]
    if 'target_environment_sha256'in json_at(a.native/'receipt.json'):
        def changed_target_environment(root):
            path=root/'n/target-environment.bin';path.write_bytes(b'changed=environment\0\0')
            rp=root/'n/receipt.json';receipt=json_at(rp);receipt['target_environment_sha256']=sha(path.read_bytes());write_json(rp,receipt)
        mutations.append(('changed full target environment with matching receipt hash',changed_target_environment))
        def removed_target_environment(root):
            path=root/'n/receipt.json';receipt=json_at(path);del receipt['target_environment_sha256'];write_json(path,receipt)
        mutations.append(('missing required full environment proof',removed_target_environment))
    if json_at(a.original/'receipt.json').get('case_inputs'):
        def changed_ir_input(root):
            path=root/'o/receipt.json';receipt=json_at(path);name=next(iter(receipt['case_inputs']));receipt['case_inputs'][name]='0'*64;write_json(path,receipt)
        mutations.append(('different captured IR input hash',changed_ir_input))
        def removed_input_binding(root):
            path=root/'o/receipt.json';receipt=json_at(path);del receipt['case_inputs'];write_json(path,receipt)
        mutations.append(('missing required IR input binding',removed_input_binding))
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
    reuse=None;reused=[]
    if a.reviewed_baseline:
        assert a.review_note,'baseline reuse requires the concrete reviewed scope'
        baseline=json_at(a.reviewed_baseline)
        result_path=BINARY/baseline['control_result'];assert sha(result_path.read_bytes())==baseline['control_result_sha256']
        prior=json_at(result_path);assert prior['count']==len(prior['rejected'])and not prior['accepted']
        covered=set(prior['rejected'])|set((prior.get('reuse')or{}).get('reused',[]))
        archive=a.reviewed_baseline.parent/baseline['source_archive'];assert sha(archive.read_bytes())==baseline['source_archive_sha256']
        with tarfile.open(archive,'r:gz')as source:
            actual={member.name:sha(source.extractfile(member).read())for member in source.getmembers()}
        assert actual==baseline['checker_contracts'],'baseline checker/contract snapshot changed'
        selected=[]
        for name,change in mutations:
            if name not in covered or (a.affected and re.search(a.affected,name,re.I)):selected.append((name,change))
            else:reused.append(name)
        mutations=selected
        reuse={'baseline':str(a.reviewed_baseline.resolve()),'baseline_sha256':sha(a.reviewed_baseline.read_bytes()),
            'baseline_control_result_sha256':baseline['control_result_sha256'],'baseline_checker_contracts':actual,
            'current_checker_contracts':{name:sha((BINARY/name).read_bytes())for name in actual},
            'baseline_proof_inputs':baseline['proof_inputs'],'review':a.review_note,'reused':reused,'count':len(reused)}
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
    print(json.dumps({'function':a.function,'count':len(passed),'rejected':passed,'control_frontier':control[1]['matched'],'accepted':False,
        'current_proof_inputs':{'original_receipt':sha((a.original/'receipt.json').read_bytes()),'native_receipt':sha((a.native/'receipt.json').read_bytes())},
        'reuse':reuse,'seconds':{'control_and_rejections':time.monotonic()-begin}},indent=2))

if __name__=='__main__':main()
