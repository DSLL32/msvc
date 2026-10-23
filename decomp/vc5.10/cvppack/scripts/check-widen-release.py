#!/usr/bin/env python3
"""Focused four-lifetime Release, buffer ownership and strict-RET controls."""
import copy
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import tempfile
import time
from state import BINARY,sha

start=time.perf_counter()
pair=BINARY/'cases/hello-debug-dll/evidence/0095-release-widen-retained'
original_dir=pair/'original';native_dir=pair/'native-sequence';audit_dir=pair/'hardened-audit'
config=json.loads((original_dir/'configuration.json').read_text());data=json.loads((original_dir/'data.json').read_text())
events=(original_dir/'events').read_text();native=json.loads((native_dir/'state.json').read_text())
def module(name):
    spec=importlib.util.spec_from_file_location(name.replace('-','_'),BINARY/'scripts'/(name+'.py'))
    value=importlib.util.module_from_spec(spec);spec.loader.exec_module(value);return value
decoder=module('raw-state');release=module('release-tail')
artifacts={'hello.dll':(original_dir/'artifact-hello.dll').read_bytes()}
views={side:{p.name:p.read_bytes() for p in directory.glob(pattern)} for side,directory,pattern in [('original',original_dir,'events-view-*.bin'),('native',native_dir,'native-view-*.bin')]}
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);decoded=decoder.decode(events)
assert len(native['widen_release_calls'])==4 and native['widen_release_calls'][-1]['exit_cursor']==4
rejected=[]
def reject(name,mutate,reason,raw=events):
    changed=copy.deepcopy(native);mutate(changed)
    try:compare(changed,raw)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),('wrong rejection',name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
reject('missing-completed-lifetime',lambda s:s.update(widen_release_calls=s['widen_release_calls'][:-1]),'Release complete lifetime inventory differs')
reject('missing-original-release',lambda s:None,'Widen actual lifetime sequence differs',events.replace('widen destroy-return ','unrecognized-release-return ',1))
reject('wrong-native-event-order',lambda s:s['widen_sequence'].reverse(),'Widen actual lifetime sequence differs')
for field,value,reason in [('object',0,'Release opaque instance/object differs'),('entry_cursor',3,'Release native lifetime transition differs'),('exit_cursor',0,'Release native lifetime transition differs'),('entry_stage',2,'Release native lifetime transition differs'),('exit_stage',3,'Release native lifetime transition differs'),('entry_ecx',0,'Release native actual ECX differs'),('exit_esp',0,'Release native RET0 differs'),('exit_last_error',123,'Release actual LastError differs'),('copied_size',0,'Release caller copied output differs'),('copied_address',0,'Release copied output source-buffer alias differs')]:
    reject('changed-release-'+field,lambda s,f=field,v=value:s['widen_release_calls'][2].update({f:v}),reason)
def changed_byte(state,key,index,field,offset):
    row=state[key][index];raw=bytearray.fromhex(row[field]);raw[offset]^=1;row[field]=raw.hex()
reject('changed-earlier-copied-output',lambda s:changed_byte(s,'widen_release_calls',0,'copied',0),'Release caller copied output differs')
reject('changed-earlier-emit-prefix',lambda s:changed_byte(s,'widen_emit_calls',0,'output_after',0),'emit initialized output prefix differs')
reject('changed-earlier-emit-own-tail',lambda s:changed_byte(s,'widen_emit_calls',1,'output_after',59),'emit changed own unwritten capacity tail')
reject('changed-earlier-query-input-post',lambda s:changed_byte(s,'widen_query_calls',1,'input_after',0),'query changed initialized input')
reject('wrong-call-time-source-module',lambda s:s['image_allocations'][-1]['source'].update(module=2),'symbol rewrite actual call-time operands differ')
# Find the first rewrite by its actual retained source provenance, not array
# adjacency to other allocator classes.
old_native=next(row['address'] for row in native['image_allocations'] if row.get('owner')=='rewrite_module_symbols')
reject('changed-retained-earlier-symbol-output',lambda s:s['memory'].update({str(old_native):'ff'+s['memory'][str(old_native)][2:]}),'directory rewritten symbol contents differ')
# Actual original return-register/stack memory move together; this deliberately
# passes raw-memory lookup and reaches the intended ABI cleanup guard.
marker=events.index('widen destroy-return ')
begin=events.rfind('registers ',0,marker);end=events.index('\n',begin)
parts=events[begin:end].split();old_esp=int(parts[-1],16);parts[-1]='%08x'%(old_esp+4)
raw=events[:begin]+' '.join(parts)+events[end:marker].replace('memory %08x '%old_esp,'memory %08x '%(old_esp+4),1)+events[marker:]
reject('original-wrong-release-cleanup',lambda s:None,'Release actual RET0/caller cleanup evidence differs',raw)
# Own unwritten tails are deliberately unconstrained across processes.
changed=copy.deepcopy(native)
for row in changed['widen_emit_calls']:
    written=int.from_bytes(bytes.fromhex(row['description_after'])[:4],'little')
    for field in ('output_before','output_after'):
        count=len(bytes.fromhex(row[field]))-written;row[field]=row[field][:written*2]+'c7'*count
last=changed['widen_emit_calls'][-1];changed['memory'][str(last['output_address'])]=last['output_after'];compare(changed)
# Strict return coverage remains all other functions. These controls refresh
# hashes on mutated projections/reports to test semantics, not only stale bytes.
audit=json.loads((audit_dir/'audit.json').read_text());release.verify_projection(audit,audit_dir)
with tempfile.TemporaryDirectory(prefix='cvpack-release-ret-',dir=Path.home()/'.ai-tmp') as temporary:
    directory=Path(temporary)
    for name in ('graph-config.json','ret-projection-config.json','ret-projection-report.json'):(directory/name).write_bytes((audit_dir/name).read_bytes())
    def projection_reject(name,mutate,reason):
        current=copy.deepcopy(audit)
        for filename in ('ret-projection-config.json','ret-projection-report.json'):(directory/filename).write_bytes((audit_dir/filename).read_bytes())
        current['external_release_tail']['command'][current['external_release_tail']['command'].index('--config')+1]=str(directory/'ret-projection-config.json')
        mutate(current)
        try:release.verify_projection(current,directory)
        except AssertionError as error:
            assert reason in str(error),(name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
        else:raise AssertionError('accepted '+name)
    def remove_body(current):
        path=directory/'ret-projection-config.json';value=json.loads(path.read_text());value['accepted'].pop();path.write_text(json.dumps(value));current['external_release_tail']['config_sha256']=sha(path)
    projection_reject('extra-body-RET-exclusion-with-refreshed-hash',remove_body,'RET projection excludes more than Release')
    def remove_check(current):
        path=directory/'ret-projection-report.json';value=json.loads(path.read_text());value['checks'].remove('ret_cleanup');path.write_text(json.dumps(value));current['external_release_tail'].update(report=value,report_sha256=sha(path))
    projection_reject('missing-strict-RET-check-refreshed-report',remove_check,'RET projection changed common check set')
    projection_reject('omitted-strict-RET-command',lambda a:a['external_release_tail']['command'].remove('--check-ret'),'RET projection omitted strict cleanup')
    projection_reject('omitted-other-graph-check',lambda a:a['external_release_tail']['command'].remove('--check-imports'),'RET projection omitted or changed common checks')
    def swap_report(current,key,value):
        path=directory/'ret-projection-report.json';report=json.loads(path.read_text());report[key]=value
        path.write_text(json.dumps(report));current['external_release_tail'].update(report=report,report_sha256=sha(path))
    projection_reject('swapped-PE-input-refreshed-report',lambda a:swap_report(a,'register_import_inputs',dict(a['external_release_tail']['report']['register_import_inputs'],rebuilt_sha256='00'*32)), 'RET projection changed graph input')
    projection_reject('swapped-profile-refreshed-report',lambda a:swap_report(a,'profile','link'), 'RET projection changed graph input')
    projection_reject('swapped-import-catalog-refreshed-report',lambda a:swap_report(a,'import_catalogs',{}), 'RET projection changed graph input')
    def swap_body(current):
        reports=copy.deepcopy(current['external_release_tail']['report']['functions'])
        name=next(iter(reports));reports[name]['original']={'forged':'same name, different graph'}
        swap_report(current,'functions',reports)
    projection_reject('swapped-body-facts-refreshed-report',swap_body,'RET projection changed common body facts')

# Independent machine-code proof: the native genuine-thiscall callback returns
# without popping explicit arguments. Canonical Win32 caller identity is checked
# by release.prove(), independently compiled with the provided VC5 compiler.
disassembly=subprocess.check_output(['objdump','-d','-Mintel','--disassemble=destroy',str(BINARY/'build/native/CVPACK')],text=True)
assert len(re.findall(r'\bret\b',disassembly))==1 and not re.findall(r'\bret\s+0x',disassembly)
(pair/'native-release-callback-disassembly.txt').write_text(disassembly)
proof=release.prove(config)
report=dict(schema=1,native_only=True,count=len(rejected),rejected=rejected,lifetimes=4,full_graph_functions=95,common_strict_ret_functions=94,
    exact_external_tail_functions=1,original_release_cleanup=[row['exit_esp']-row['entry_esp'] for row in decoded['widen_release_calls']],
    native_release_cleanup=[row['exit_esp']-row['entry_esp'] for row in native['widen_release_calls']],own_tail_arbitrary_native_positive=True,
    abi_proof=proof,original_receipt_sha256=sha(original_dir/'receipt.json'),native_receipt_sha256=sha(native_dir/'receipt.json'),
    audit_sha256=sha(audit_dir/'audit.json'),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),observer_sha256=sha(BINARY/'scripts/gdb-state.py'),
    checker_sha256=sha(Path(__file__)),seconds=time.perf_counter()-start)
(pair/'release-checks-v2.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(rejected=len(rejected),seconds=report['seconds'])))
