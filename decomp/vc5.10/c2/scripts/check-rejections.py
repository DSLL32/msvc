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
