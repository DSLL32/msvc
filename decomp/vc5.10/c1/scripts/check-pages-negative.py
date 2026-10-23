#!/usr/bin/env python3
"""Focused page allocation output/count/cursor and genuine commit boundary."""
import argparse,copy,importlib.util,json,re,shutil,tempfile,time
from pathlib import Path
import default_pool_state

def main():
    p=argparse.ArgumentParser();p.add_argument('original',type=Path);p.add_argument('native',type=Path);a=p.parse_args();started=time.perf_counter()
    spec=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));trial=importlib.util.module_from_spec(spec);spec.loader.exec_module(trial);trial.check(a.original,a.native)
    state=json.loads((a.native/'state.json').read_text());candidate=json.loads((a.original/'candidate.json').read_text());fields=json.loads((Path(__file__).resolve().parents[1]/candidate['field_map']).read_text())
    events=(a.original/'events.txt').read_text();module=int(re.findall(r'^module name=c1n32_l.dll actual=([0-9a-f]+)',events,re.M)[0],16);sequence=int(re.findall(r'^trap seq=(\d+)',events,re.M)[-1])
    labels=['missing-page-entry','page-pc','page-receiver','page-count','page-output','page-boundary','page-formal','missing-commit-formal','invented-commit-formal','commit-receiver','wrong-caller','caller-count','caller-output','wrong-count-store','wrong-cursor-store','premature-pool-cursor','premature-commit-limit','premature-commit-count','neighbor-pool-write','snapshot-write','original-count-store','original-cursor-store','original-neighbor-write','original-receiver','original-return','original-call-target']
    with tempfile.TemporaryDirectory(prefix='pages-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1')as temporary:
        root=Path(temporary);o=root/'original';n=root/'native';shutil.copytree(a.original,o);shutil.copytree(a.native,n)
        tracked=[n/'arena-page-entries.json',o/'events.txt',o/'relocated-text.bin',o/('compiler-bss-%03d.bin'%sequence)];saved={f:f.read_bytes()for f in tracked}
        for label in labels:
            for file,raw in saved.items():file.write_bytes(raw)
            value=copy.deepcopy(state)
            if label.startswith('page-')or label=='missing-page-entry':
                file=n/'arena-page-entries.json';rows=json.loads(file.read_text())
                if label=='missing-page-entry':rows=[]
                elif label=='page-pc':rows[0]['pc']+=1
                elif label=='page-receiver':rows[0]['ecx']+=32
                elif label=='page-count':rows[0]['pages']=2
                elif label=='page-output':rows[0]['available_bytes']+=4
                elif label=='page-boundary':rows[0]['boundary_rva']+=1
                else:rows[0]['formal_inventory'].pop()
                file.write_text(json.dumps(rows))
            elif label=='missing-commit-formal':value['formal_inventory']=[]
            elif label=='invented-commit-formal':value['formal_inventory'].append(dict(name='hidden_eax',type='unsigned int'))
            elif label=='commit-receiver':value['args']['this']+=32
            elif label=='wrong-caller':value['caller']['symbol']='CompilerPool::Refill'
            elif label=='caller-count':value['caller']['args']['pages']=2
            elif label=='caller-output':value['caller']['args']['available_bytes']+=4
            elif label=='wrong-count-store':value['compiler_state']['bss']['pools'][8]['remaining']+=1
            elif label=='wrong-cursor-store':value['compiler_state']['bss']['arenas'][0]['cursor']+=1
            elif label=='premature-pool-cursor':value['compiler_state']['bss']['pools'][8]['cursor']=1
            elif label=='premature-commit-limit':value['compiler_state']['bss']['arenas'][0]['committed_limit']+=4096
            elif label=='premature-commit-count':value['compiler_state']['bss']['arenas'][0]['committed_bytes']=4096
            elif label=='neighbor-pool-write':value['compiler_state']['bss']['pools'][7]['remaining']=4096
            elif label=='snapshot-write':value['compiler_state']['snapshot']['memory_percent']+=1
            elif label in ('original-count-store','original-cursor-store','original-neighbor-write','original-call-target'):
                file=o/('relocated-text.bin'if label=='original-call-target'else 'compiler-bss-%03d.bin'%sequence);raw=bytearray(file.read_bytes());offset=(0xab0e-0x1000 if label=='original-call-target'else fields['pools'][8]['original_object_rva']-0x50000+4 if label=='original-count-store'else fields['arenas']['original_rva']-0x50000 if label=='original-cursor-store'else fields['pools'][7]['original_object_rva']-0x50000+4);raw[offset]^=1;file.write_bytes(raw)
            else:
                file=o/'events.txt';text=file.read_text();match=re.search(r'^trap seq=%d rva=00009883 .*\n'%sequence,text,re.M);assert match;row=match[0]
                if label=='original-receiver':row=re.sub(r'ecx=([0-9a-f]+)',lambda m:'ecx=%08x'%(int(m[1],16)+32),row)
                else:
                    words=re.search(r'stack=([0-9a-f,]+)',row)[1].split(',');words[0]='%08x'%(int(words[0],16)+1);row=re.sub(r'stack=[0-9a-f,]+','stack='+','.join(words),row)
                file.write_text(text[:match.start()]+row+text[match.end():])
            try:
                observed,at=default_pool_state.boundary(o,n,value,sequence,module)
                default_pool_state.validate_allocation(o,n,fields,observed,value,at,sequence,module,candidate)
            except(AssertionError,KeyError,ValueError,IndexError):continue
            raise AssertionError('accepted '+label)
    print(json.dumps(dict(status='PASS',fresh_target_controls=len(labels),rejected=labels,seconds=time.perf_counter()-started,scope='Fresh complete restore positive and affected real page count/cursor/commit guards; normal positive separate; unchanged campaigns reused.'),indent=2))
if __name__=='__main__':main()
