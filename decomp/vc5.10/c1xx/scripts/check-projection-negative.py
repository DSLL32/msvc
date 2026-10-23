#!/usr/bin/env python3
"""Reject altered typed snapshot/callback/mock proofs without executing a binary."""
import copy
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import tempfile


def load(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module


d=load('projection','check-projection.py')
history=load('projection_history','check-history.py')


def edit(path,change):
    value=json.loads(path.read_text());change(value);path.write_text(json.dumps(value))


def rehash(directory,name):
    path=directory/name
    edit(directory/'receipt.json',lambda row:row['artifacts'].update({name:{'sha256':d.s.sha(path),'size':path.stat().st_size}}))


def rejected(name,run):
    try:run()
    except (ValueError,KeyError,TypeError,struct.error):return
    raise RuntimeError('accepted altered projection proof '+name)


def main():
    packet=json.loads((d.PROJECT/'cases/minimal/initializer-trials.json').read_text())
    records={row['seed']:row for row in packet['trials']}
    passed=[]
    for seed in (None,'restore-nonzero','restore-zero','first-copy-aux'):
        row=records[seed];o,n=Path(row['original']),Path(row['native'])
        audit=history.configure(d,o,n,saved_record=row);d.check(o,n,audit_path=audit)
    state=json.loads((Path(records[None]['native'])/'state.json').read_text())
    model=json.loads((Path(records[None]['native'])/'state-model.json').read_text())
    for name in ('missing-field','original-alias','wrong-original-offset','native-alias','unmodeled-native-field','padding-blob'):
        changed=copy.deepcopy(model);observed=copy.deepcopy(state['frontend'])
        if name=='missing-field':changed['fields'].pop()
        elif name=='original-alias':changed['fields'][1]['original_address']=changed['fields'][2]['original_address']
        elif name=='wrong-original-offset':changed['fields'][0]['original_snapshot_offset']+=4
        elif name=='native-alias':observed['storage']['cleanup_count'][0]=observed['storage']['saved_state'][0]
        elif name=='unmodeled-native-field':observed['declared_fields']['initialized'].append('unknown')
        else:observed['known_sizes']['initialized']=8520
        rejected(name,lambda:d.field_map(changed,observed));passed.append(name)
    cases={None:('wrong-allocation-size','false-native-PC','changed-source','missing-cold-owner'),
           'restore-nonzero':('bad-phase-flag','uncleared-callback','uncleared-count','changed-snapshot-pointer','wrong-restore-mode','bad-original-transfer','wrong-callback-order'),
           'restore-zero':('wrong-zero-mode',),
           'first-copy-aux':('wrong-mock-request','wrong-mock-return','wrong-mock-caller','not-an-INT3','native-guard-corrupt','original-guard-corrupt','old-buffer-modified','new-buffer-not-copied','first-copy-clears-BSS','wrong-original-mock-return')}
    for seed,names in cases.items():
        row=records[seed];original,native=Path(row['original']),Path(row['native'])
        audit=history.configure(d,original,native,saved_record=row)
        for name in names:
            with tempfile.TemporaryDirectory(prefix='projection-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as temporary:
                root=Path(temporary);o=root/'original';n=root/'native'
                shutil.copytree(original,o);shutil.copytree(native,n)
                proof=root/'audit.json';shutil.copy2(audit,proof)
                if name=='missing-cold-owner':
                    edit(proof,lambda j:j['report']['owned_code_chunks']['original']['functions'].update({'_initialize_compiler_pass':[[0x10438550,0x10438610]]}))
                elif name=='changed-source':
                    path=next((n/'source-content').iterdir());path.write_bytes(path.read_bytes()+b'\n')
                elif name in ('original-guard-corrupt','bad-original-transfer'):
                    path=o/('aux-new-guards-phase.bin' if name=='original-guard-corrupt' else 'frontend-data-phase.bin')
                    raw=bytearray(path.read_bytes());raw[0]^=1;path.write_bytes(raw);rehash(o,path.name)
                elif name=='wrong-original-mock-return':
                    path=o/'events.txt';path.write_text(path.read_text().replace('target-byte=cc request=8520','target-byte=cc request=8521'));rehash(o,path.name)
                else:
                    file='phase.json' if name in ('bad-phase-flag','uncleared-callback','uncleared-count','changed-snapshot-pointer','wrong-restore-mode','wrong-zero-mode','native-guard-corrupt','old-buffer-modified','new-buffer-not-copied','first-copy-clears-BSS') else 'aux-call.json' if name in ('wrong-mock-request','not-an-INT3') else 'aux-return.json' if name in ('wrong-mock-return','wrong-mock-caller') else 'state.json'
                    def change(j):
                        if name=='wrong-allocation-size':j['args']['bytes']=8516
                        elif name=='false-native-PC':j['pc']+=1
                        elif name=='bad-phase-flag':j['frontend']['first_invocation']=0
                        elif name=='uncleared-callback':j['frontend']['cleanup_callbacks'][31]=j['frontend']['callback_slots'][1]
                        elif name=='uncleared-count':j['frontend']['cleanup_count']=32
                        elif name=='changed-snapshot-pointer':j['frontend']['saved_state']+=4
                        elif name=='wrong-restore-mode':j['frontend']['restore_mode']=1
                        elif name=='wrong-zero-mode':j['frontend']['restore_mode']=0
                        elif name=='wrong-callback-order':j['frontend']['callback_slots'][1:3]=j['frontend']['callback_slots'][1:3][::-1]
                        elif name=='wrong-mock-request':j['aux_mock_call']['request']=8516
                        elif name=='wrong-mock-return':j['eax']+=4
                        elif name=='wrong-mock-caller':j['pc']+=1
                        elif name=='not-an-INT3':j['aux_mock_call']['actual_opcode']='90'
                        elif name=='native-guard-corrupt':j['aux_owned']['new']['suffix']='00'*32
                        elif name=='old-buffer-modified':j['aux_owned']['old']['known_first']=17
                        elif name=='new-buffer-not-copied':j['aux_owned']['new']['known_first']=303
                        elif name=='first-copy-clears-BSS':j['frontend']['cleanup_count']=0
                        else:raise RuntimeError(name)
                    edit(n/file,change);rehash(n,file)
                rejected(name,lambda:d.check(o,n,audit_path=proof));passed.append(name)
    print(json.dumps({'result':'PASS','rejected':passed,'count':len(passed)},indent=2))


if __name__=='__main__':main()
