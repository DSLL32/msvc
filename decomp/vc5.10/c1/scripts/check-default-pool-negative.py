#!/usr/bin/env python3
"""Focused default-wrapper entry, source order, receiver and untouched state."""
import argparse,copy,importlib.util,json,re,shutil,tempfile,time
from pathlib import Path
import default_pool_state

def main():
    parser=argparse.ArgumentParser();parser.add_argument('original',type=Path);parser.add_argument('native',type=Path);args=parser.parse_args();started=time.perf_counter()
    spec=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));trial=importlib.util.module_from_spec(spec);spec.loader.exec_module(trial);trial.check(args.original,args.native)
    state=json.loads((args.native/'state.json').read_text());candidate=json.loads((args.original/'candidate.json').read_text());fields=json.loads((Path(__file__).resolve().parents[1]/candidate['field_map']).read_text())
    events=(args.original/'events.txt').read_text();module=int(re.findall(r'^module name=c1n32_l.dll actual=([0-9a-f]+)',events,re.M)[0],16);sequence=int(re.findall(r'^trap seq=(\d+)',events,re.M)[-1]);entry,at=default_pool_state.boundary(args.original,args.native,state,sequence,module)
    labels=['missing-physical-entry','entry-receiver','entry-formals','entry-argument','entry-return','state-receiver','state-argument','state-formals','state-formal-type','wrong-caller','caller-argument','pool-write','neighbor-pool-write','arena-write','saved-state-write','initialized-write','original-bss-write','original-saved-write','ninth-receiver-source-order','first-call-source-target','original-receiver','original-argument','omitted-wrapper-entry','wrong-wrapper-invocation','wrong-wrapper-formal']
    with tempfile.TemporaryDirectory(prefix='default-pool-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1')as temporary:
        root=Path(temporary);o=root/'original';n=root/'native';shutil.copytree(args.original,o);shutil.copytree(args.native,n)
        tracked=[n/'pool-arena-entries.json',n/'default-pools-entry.json',o/'events.txt',o/'relocated-text.bin',o/('compiler-bss-%03d.bin'%sequence),o/('compiler-snapshot-%03d.bin'%sequence)];saved={f:f.read_bytes()for f in tracked}
        for label in labels:
            for file,raw in saved.items():file.write_bytes(raw)
            value=copy.deepcopy(state)
            if label.startswith('entry-')or label=='missing-physical-entry':
                file=n/'pool-arena-entries.json';rows=json.loads(file.read_text())
                if label=='missing-physical-entry':rows=[]
                elif label=='entry-receiver':rows[0]['ecx']+=16
                elif label=='entry-formals':rows[0]['formal_inventory'].pop()
                elif label=='entry-argument':rows[0]['arena_index']=1
                else:rows[0]['return']+=1
                file.write_text(json.dumps(rows))
            elif label=='state-receiver':value['args']['this']+=16
            elif label=='state-argument':value['args']['arena_index']=1
            elif label=='state-formals':value['formal_inventory'].pop(0)
            elif label=='state-formal-type':value['formal_inventory'][1]['type']='int'
            elif label=='wrong-caller':value['caller']['symbol']='c1_main'
            elif label=='caller-argument':value['caller']['args']['invented']=0
            elif label=='pool-write':value['compiler_state']['bss']['pools'][8]['arena']=1
            elif label=='neighbor-pool-write':value['compiler_state']['bss']['pools'][7]['cursor']=1
            elif label=='arena-write':value['compiler_state']['bss']['arenas'][0]['reset_value']=1
            elif label=='saved-state-write':value['compiler_state']['snapshot']['compiler_flags']^=1
            elif label=='initialized-write':value['compiler_state']['initialized']['compiler_flags']^=1
            elif label in ('original-bss-write','original-saved-write','ninth-receiver-source-order','first-call-source-target'):
                file=o/('compiler-bss-%03d.bin'%sequence if label=='original-bss-write'else 'compiler-snapshot-%03d.bin'%sequence if label=='original-saved-write'else 'relocated-text.bin');raw=bytearray(file.read_bytes());offset=0 if label.startswith('original-')else (0x12b26 if label.startswith('ninth')else 0x12acb)-0x1000;raw[offset]^=1;file.write_bytes(raw)
            elif label.startswith('original-'):
                file=o/'events.txt';text=file.read_text();match=re.search(r'^trap seq=%d rva=00012b30 .*\n'%sequence,text,re.M);assert match;row=match[0]
                if label=='original-receiver':row=re.sub(r'ecx=([0-9a-f]+)',lambda m:'ecx=%08x'%(int(m[1],16)+16),row)
                else:
                    words=re.search(r'stack=([0-9a-f,]+)',row)[1].split(',');words[1]='00000001';row=re.sub(r'stack=[0-9a-f,]+','stack='+','.join(words),row)
                file.write_text(text[:match.start()]+row+text[match.end():])
            else:
                file=n/'default-pools-entry.json';rows=json.loads(file.read_text())
                if label=='omitted-wrapper-entry':rows=[]
                elif label=='wrong-wrapper-invocation':rows[0]['invocation']['args']['mode']^=1
                else:rows[0]['formal_inventory']=[dict(name='invented',type='int')]
                file.write_text(json.dumps(rows))
            try:
                observed,observed_at=default_pool_state.boundary(o,n,value,sequence,module)
                default_pool_state.validate(o,n,fields,observed,value,observed_at,sequence,module,candidate)
            except(AssertionError,KeyError,ValueError,IndexError):continue
            raise AssertionError('accepted '+label)
    print(json.dumps(dict(status='PASS',fresh_target_controls=len(labels),rejected=labels,seconds=time.perf_counter()-started,scope='Fresh full restore positive plus actual wrapper/member/order/untouched-state validators; independent normal positive separate; earlier unchanged campaigns reused, not rerun.'),indent=2))
if __name__=='__main__':main()
