#!/usr/bin/env python3
"""Focused genuine SetArena entry/store and Refill frontier controls."""
import argparse,copy,importlib.util,json,re,shutil,tempfile,time
from pathlib import Path
import default_pool_state

def main():
    p=argparse.ArgumentParser();p.add_argument('original',type=Path);p.add_argument('native',type=Path);a=p.parse_args();started=time.perf_counter()
    spec=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));trial=importlib.util.module_from_spec(spec);spec.loader.exec_module(trial);trial.check(a.original,a.native)
    state=json.loads((a.native/'state.json').read_text());candidate=json.loads((a.original/'candidate.json').read_text());fields=json.loads((Path(__file__).resolve().parents[1]/candidate['field_map']).read_text())
    events=(a.original/'events.txt').read_text();module=int(re.findall(r'^module name=c1n32_l.dll actual=([0-9a-f]+)',events,re.M)[0],16);sequence=int(re.findall(r'^trap seq=(\d+)',events,re.M)[-1])
    labels=['missing-member-entry','member-receiver','member-formal','member-argument','member-boundary','member-invocation','missing-refill-formal','refill-formal-type','refill-receiver','refill-size','wrong-caller','caller-index','wrong-arena-owner','neighbor-pool-write','arena-write','snapshot-write','initialized-write','original-wrong-store','original-neighbor-write','original-refill-receiver','original-refill-size','original-code-store']
    with tempfile.TemporaryDirectory(prefix='binding-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1')as temporary:
        root=Path(temporary);o=root/'original';n=root/'native';shutil.copytree(a.original,o);shutil.copytree(a.native,n)
        tracked=[n/'pool-arena-entries.json',o/'events.txt',o/'relocated-text.bin',o/('compiler-bss-%03d.bin'%sequence)];saved={f:f.read_bytes()for f in tracked}
        for label in labels:
            for file,raw in saved.items():file.write_bytes(raw)
            value=copy.deepcopy(state)
            if label.startswith('member-')or label=='missing-member-entry':
                file=n/'pool-arena-entries.json';rows=json.loads(file.read_text())
                if label=='missing-member-entry':rows=[]
                elif label=='member-receiver':rows[0]['ecx']+=16
                elif label=='member-formal':rows[0]['formal_inventory'].pop()
                elif label=='member-argument':rows[0]['args']['arena_index']=1
                elif label=='member-boundary':rows[0]['boundary_rva']+=1
                else:rows[0]['invocation']['args']['mode']^=1
                file.write_text(json.dumps(rows))
            elif label=='missing-refill-formal':value['formal_inventory'].pop()
            elif label=='refill-formal-type':value['formal_inventory'][1]['type']='int'
            elif label=='refill-receiver':value['args']['this']+=16
            elif label=='refill-size':value['args']['minimum_bytes']=2
            elif label=='wrong-caller':value['caller']['symbol']='initialize_default_pools'
            elif label=='caller-index':value['caller']['args']['arena_index']=1
            elif label=='wrong-arena-owner':value['compiler_state']['bss']['pools'][8]['arena']+=32
            elif label=='neighbor-pool-write':value['compiler_state']['bss']['pools'][7]['remaining']=1
            elif label=='arena-write':value['compiler_state']['bss']['arenas'][0]['reset_value']=1
            elif label=='snapshot-write':value['compiler_state']['snapshot']['compiler_flags']^=1
            elif label=='initialized-write':value['compiler_state']['initialized']['compiler_flags']^=1
            elif label in ('original-wrong-store','original-neighbor-write','original-code-store'):
                file=o/('relocated-text.bin'if label=='original-code-store'else 'compiler-bss-%03d.bin'%sequence);raw=bytearray(file.read_bytes());offset=(0x12b40-0x1000 if label=='original-code-store'else fields['pools'][8]['original_object_rva']-0x50000+12 if label=='original-wrong-store'else fields['pools'][7]['original_object_rva']-0x50000+4);raw[offset]^=1;file.write_bytes(raw)
            else:
                file=o/'events.txt';text=file.read_text();match=re.search(r'^trap seq=%d rva=0000aa9c .*\n'%sequence,text,re.M);assert match;row=match[0]
                if label=='original-refill-receiver':row=re.sub(r'ecx=([0-9a-f]+)',lambda m:'ecx=%08x'%(int(m[1],16)+16),row)
                else:
                    words=re.search(r'stack=([0-9a-f,]+)',row)[1].split(',');words[1]='00000002';row=re.sub(r'stack=[0-9a-f,]+','stack='+','.join(words),row)
                file.write_text(text[:match.start()]+row+text[match.end():])
            try:
                observed,at=default_pool_state.boundary(o,n,value,sequence,module)
                default_pool_state.validate_binding(o,n,fields,observed,value,at,sequence,module,candidate)
            except(AssertionError,KeyError,ValueError,IndexError):continue
            raise AssertionError('accepted '+label)
    print(json.dumps(dict(status='PASS',fresh_target_controls=len(labels),rejected=labels,seconds=time.perf_counter()-started,scope='Fresh complete restore positive and affected actual member/store/refill guards; normal positive separate; unchanged campaigns reused.'),indent=2))
if __name__=='__main__':main()
