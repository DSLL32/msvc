#!/usr/bin/env python3
"""Acceptance controls for real completion, ordering, identity and cleanup."""
import argparse
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import tempfile

spec=importlib.util.spec_from_file_location('decoder',Path(__file__).with_name('check-completion.py'))
d=importlib.util.module_from_spec(spec)
spec.loader.exec_module(d)


def edit(path,change):
    value=json.loads(path.read_text());change(value);path.write_text(json.dumps(value))


def rehash(directory,name):
    path=directory/name
    edit(directory/'receipt.json',lambda j:j['artifacts'].update({name:{'sha256':d.s.sha(path),'size':path.stat().st_size}}))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('original',type=Path);parser.add_argument('native',type=Path)
    parser.add_argument('--historical',action='store_true',help='revalidate immutable execution snapshots; cannot promote')
    args=parser.parse_args()
    historical_audit=None
    if args.historical:
        spec=importlib.util.spec_from_file_location('history',Path(__file__).with_name('check-history.py'))
        history=importlib.util.module_from_spec(spec);spec.loader.exec_module(history)
        historical_audit=history.configure(d,args.original,args.native)
    d.check(args.original,args.native,audit_path=historical_audit)
    cases=('missing-return','wrong-return','false-original-completion','export-body-entered','original-module',
           'original-dangling-end','original-callback','original-x87','wrong-provider','native-return',
           'native-dangling-end','native-module','native-x87','native-last-error','native-notification',
           'raw-native-state-mismatch','native-exit','native-trace','compiled-source-content','audit-missing-cold')
    for case in cases:
        with tempfile.TemporaryDirectory(prefix='completion-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as temp:
            root=Path(temp);o=root/'original';n=root/'native'
            shutil.copytree(args.original,o);shutil.copytree(args.native,n)
            audit=root/'audit.json';shutil.copy2(historical_audit or d.s.PROJECT/'build/audit-proof.json',audit)
            if case in ('missing-return','wrong-return','false-original-completion','export-body-entered','original-x87','wrong-provider'):
                path=o/'events.txt';text=path.read_text()
                if case=='missing-return':text='\n'.join(row for row in text.splitlines() if not row.startswith('crt-return seq=8'))+'\n'
                elif case=='wrong-return':text=text.replace('reason=0 status=1','reason=0 status=0')
                elif case=='false-original-completion':text=text.replace('exit=0 target=1','exit=1 target=1')
                elif case=='export-body-entered':text=text.replace('rva=0002e52a','rva=000903ad',1)
                elif case=='original-x87':text=text.replace('control-word=ffff027f','control-word=ffff037f',1)
                else:text=text.replace('resolved=78001715','resolved=78001716')
                path.write_text(text);rehash(o,path.name)
            elif case.startswith('original-'):
                path=o/('state-004.bin' if case=='original-module' else 'state-008.bin')
                raw=bytearray(path.read_bytes())
                offset={'original-module':0x28,'original-dangling-end':0x20,'original-callback':0x24}[case]
                struct.pack_into('<I',raw,offset,0 if case!='original-callback' else 1)
                path.write_bytes(raw);rehash(o,path.name)
            elif case in ('native-return','native-dangling-end','native-module','native-x87','native-last-error','native-notification'):
                records=[json.loads(line) for line in (n/'lifecycle-state.jsonl').read_text().splitlines()]
                key={'native-return':'status','native-dangling-end':'onexit_end','native-module':'dll_module',
                     'native-x87':'floating_control','native-last-error':'last_error','native-notification':'thread_calls_disabled'}[case]
                records[1][key]=0
                (n/'lifecycle-state.jsonl').write_text(''.join(json.dumps(row)+'\n' for row in records))
                edit(n/'state.json',lambda j:j.update(checkpoints=records))
                rehash(n,'lifecycle-state.jsonl');rehash(n,'state.json')
            elif case=='raw-native-state-mismatch':
                edit(n/'state.json',lambda j:j['checkpoints'][0].update(status=0));rehash(n,'state.json')
            elif case=='native-exit':edit(n/'receipt.json',lambda j:j.update(exit=1))
            elif case=='native-trace':(n/'trace.txt').write_text('');rehash(n,'trace.txt')
            elif case=='compiled-source-content':
                path=next((n/'source-content').iterdir());path.write_bytes(path.read_bytes()+b'\n')
            else:edit(audit,lambda j:j['report']['owned_code_chunks']['original']['functions'].update({'__CRT_INIT@12':[[0x1042e46a,0x1042e52a]]}))
            try:d.check(o,n,audit_path=audit)
            except (ValueError,KeyError,TypeError):continue
            raise RuntimeError('accepted altered completion '+case)
    print(json.dumps({'count':len(cases),'rejected':list(cases)}))


if __name__=='__main__':main()
