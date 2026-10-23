#!/usr/bin/env python3
"""Reject new main fields, false entry snapshots and hidden first-helper inputs."""
import importlib.util
import json
from pathlib import Path
import re
import shutil
import struct
import tempfile
import time


def load(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


def main():
    d=load('main_decoder','check-callback.py');h=load('main_negative','check-projection-negative.py');history=load('main_history','check-history.py')
    packet=json.loads((d.s.PROJECT/'cases/minimal/main-trials.json').read_text());started=time.perf_counter();passed=[]
    for row in packet['trials']:
        original,native=Path(row['original']),Path(row['native']);audit=history.configure(d,original,native,saved_record=row);d.check(original,native,audit_path=audit)
        cases=['default-phase','default-C9IL','default-flags','default-SDK-page','default-SDK-granularity','entry-phase','entry-C9IL','entry-flags','entry-SDK-page','entry-SDK-minimum','entry-SDK-revision','entry-command-not-reset','false-entry-PC','false-entry-argc','false-entry-argv','false-entry-return','raw-entry-argc','raw-entry-return','unknown-original-data-change','original-BSS-neighbor','native-neighbor','helper-argument','helper-return-PC','missing-cold-chunk'] if row['seed'] is None else ['live-phase','live-system','saved-system','original-restored-system','original-saved-system','native-restored-system','command-BSS-clear','bad-SDK-layout'] if row['seed']=='restore-nonzero' else ['helper-phase-not-set','raw-helper-phase-not-set']
        for case in cases:
            with tempfile.TemporaryDirectory(prefix='main-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as tmp:
                o,n=Path(tmp)/'original',Path(tmp)/'native';shutil.copytree(original,o);shutil.copytree(native,n);a=Path(tmp)/'audit.json';shutil.copy2(audit,a)
                if case=='missing-cold-chunk':h.edit(a,lambda j:j['report']['owned_code_chunks']['original']['functions'].update({'_main':[[0x10438bfd,0x10438c86]]}))
                elif case.startswith('raw-entry-'):
                    f=o/'events.txt'
                    def change(match):
                        words=match[2].split(',');index=1 if case=='raw-entry-argc' else 0;words[index]='%08x'%(int(words[index],16)+1);return match[1]+','.join(words)
                    f.write_text(re.sub(r'(^trap seq=\d+ rva=00038bfd[^\n]*stack=)([0-9a-f,]+)',change,f.read_text(),flags=re.M));h.rehash(o,f.name)
                elif case.startswith('default-') or case.startswith('original-') or case in ('unknown-original-data-change','raw-helper-phase-not-set'):
                    traps=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]+)',(o/'events.txt').read_text(),re.M);final='%03d'%int(traps[-1][0])
                    if case=='original-BSS-neighbor':fname='main-bss-frontier.bin';index=0
                    elif case=='unknown-original-data-change':fname='frontend-data-'+final+'.bin';index=0
                    elif case=='original-saved-system':fname='frontend-snapshot-phase.bin';index=0x17b8+8
                    elif case=='original-restored-system':fname='main-fields-phase.bin';index=20
                    elif case=='raw-helper-phase-not-set':fname='main-fields-'+final+'.bin';index=0
                    else:fname='main-fields-000.bin';index={'default-phase':0,'default-C9IL':4,'default-flags':8,'default-SDK-page':16,'default-SDK-granularity':40}[case]
                    f=o/fname;blob=bytearray(f.read_bytes());blob[index]^=1;f.write_bytes(blob);h.rehash(o,f.name)
                else:
                    fname='main-entry.json' if case.startswith('entry-') or case.startswith('false-entry-') else 'pre.json' if case in ('live-phase','live-system','saved-system') else 'phase.json' if case in ('native-restored-system','command-BSS-clear') else 'state.json'
                    def change(j):
                        f=j['frontend']
                        if case in ('entry-phase','helper-phase-not-set'):f['compiler_phase']+=1
                        elif case=='entry-C9IL':f['option_c9il']+=1
                        elif case=='entry-flags':f['compiler_flags']+=1
                        elif case.startswith('entry-SDK-'):f['system_info'][{'entry-SDK-page':1,'entry-SDK-minimum':2,'entry-SDK-revision':9}[case]]+=1
                        elif case=='entry-command-not-reset':f['command_argc']=26
                        elif case=='false-entry-PC':j['pc']+=1
                        elif case=='false-entry-argc':j['args']['argc']+=1
                        elif case=='false-entry-argv':j['raw_entry_abi']['stack_arguments'][1]+=4
                        elif case=='false-entry-return':j['raw_entry_abi']['return_pc']=0
                        elif case=='native-neighbor':f['source_signature']=0
                        elif case=='helper-argument':j['args']['unknown']=1
                        elif case=='helper-return-PC':j['caller_return_pc']=0
                        elif case=='live-phase':f['compiler_phase']=0
                        elif case=='live-system':f['system_info'][4]=0
                        elif case=='saved-system':f['saved_system_info'][2]+=4
                        elif case=='native-restored-system':f['system_info'][2]+=4
                        elif case=='command-BSS-clear':f['command_argv']=f['saved_state']+12
                        else:f['system_info_layout']['fields']['wProcessorRevision']=[32,4]
                    h.edit(n/fname,change);h.rehash(n,fname)
                h.rejected(case,lambda:d.check(o,n,audit_path=a));passed.append(case)
    print(json.dumps({'result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'rejected':passed},indent=2))


if __name__=='__main__':main()
