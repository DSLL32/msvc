#!/usr/bin/env python3
"""Focused unsigned page-count and owned allocator out-pointer controls."""
import argparse,copy,importlib.util,json,re,shutil,tempfile,time
from pathlib import Path
import default_pool_state

def main():
    p=argparse.ArgumentParser();p.add_argument('original',type=Path);p.add_argument('native',type=Path);a=p.parse_args();started=time.perf_counter()
    spec=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));trial=importlib.util.module_from_spec(spec);spec.loader.exec_module(trial);trial.check(a.original,a.native)
    state=json.loads((a.native/'state.json').read_text());candidate=json.loads((a.original/'candidate.json').read_text());fields=json.loads((Path(__file__).resolve().parents[1]/candidate['field_map']).read_text())
    events=(a.original/'events.txt').read_text();module=int(re.findall(r'^module name=c1n32_l.dll actual=([0-9a-f]+)',events,re.M)[0],16);sequence=int(re.findall(r'^trap seq=(\d+)',events,re.M)[-1])
    labels=['missing-refill-entry','refill-pc','refill-receiver','refill-size','refill-boundary','refill-formal','missing-allocator-formal','allocator-formal-type','allocator-receiver','allocator-pages','allocator-out-pointer','wrong-caller','caller-size','premature-cursor-write','premature-count-write','neighbor-arena-write','snapshot-write','original-bss-write','original-receiver','original-pages','original-out-pointer','original-cold-body','original-page-size-load']
    with tempfile.TemporaryDirectory(prefix='refill-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1')as temporary:
        root=Path(temporary);o=root/'original';n=root/'native';shutil.copytree(a.original,o);shutil.copytree(a.native,n)
        tracked=[n/'pool-refill-entries.json',o/'events.txt',o/'relocated-text.bin',o/('compiler-bss-%03d.bin'%sequence)];saved={f:f.read_bytes()for f in tracked}
        for label in labels:
            for file,raw in saved.items():file.write_bytes(raw)
            value=copy.deepcopy(state)
            if label.startswith('refill-')or label=='missing-refill-entry':
                file=n/'pool-refill-entries.json';rows=json.loads(file.read_text())
                if label=='missing-refill-entry':rows=[]
                elif label=='refill-pc':rows[0]['pc']+=1
                elif label=='refill-receiver':rows[0]['ecx']+=16
                elif label=='refill-size':rows[0]['minimum_bytes']=2
                elif label=='refill-boundary':rows[0]['boundary_rva']+=1
                else:rows[0]['formal_inventory'].pop()
                file.write_text(json.dumps(rows))
            elif label=='missing-allocator-formal':value['formal_inventory'].pop()
            elif label=='allocator-formal-type':value['formal_inventory'][2]['type']='void *'
            elif label=='allocator-receiver':value['args']['this']+=32
            elif label=='allocator-pages':value['args']['pages']=2
            elif label=='allocator-out-pointer':value['args']['available_bytes']+=4
            elif label=='wrong-caller':value['caller']['symbol']='CompilerPool::SetArena'
            elif label=='caller-size':value['caller']['args']['minimum_bytes']=2
            elif label=='premature-cursor-write':value['compiler_state']['bss']['pools'][8]['cursor']=1
            elif label=='premature-count-write':value['compiler_state']['bss']['pools'][8]['remaining']=4096
            elif label=='neighbor-arena-write':value['compiler_state']['bss']['arenas'][1]['reset_value']=1
            elif label=='snapshot-write':value['compiler_state']['snapshot']['memory_percent']+=1
            elif label in ('original-bss-write','original-cold-body','original-page-size-load'):
                file=o/('compiler-bss-%03d.bin'%sequence if label=='original-bss-write'else 'relocated-text.bin');raw=bytearray(file.read_bytes());offset=0 if label=='original-bss-write'else (0x2c3f7 if label=='original-cold-body'else 0xaaa6)-0x1000;raw[offset]^=1;file.write_bytes(raw)
            else:
                file=o/'events.txt';text=file.read_text();match=re.search(r'^trap seq=%d rva=0000aacb .*\n'%sequence,text,re.M);assert match;row=match[0]
                if label=='original-receiver':row=re.sub(r'ecx=([0-9a-f]+)',lambda m:'ecx=%08x'%(int(m[1],16)+32),row)
                else:
                    words=re.search(r'stack=([0-9a-f,]+)',row)[1].split(',');index=1 if label=='original-pages'else 2;words[index]='%08x'%(int(words[index],16)+1);row=re.sub(r'stack=[0-9a-f,]+','stack='+','.join(words),row)
                file.write_text(text[:match.start()]+row+text[match.end():])
            try:
                observed,at=default_pool_state.boundary(o,n,value,sequence,module)
                default_pool_state.validate_refill(o,n,fields,observed,value,at,sequence,module,candidate)
            except(AssertionError,KeyError,ValueError,IndexError):continue
            raise AssertionError('accepted '+label)
    print(json.dumps(dict(status='PASS',fresh_target_controls=len(labels),rejected=labels,seconds=time.perf_counter()-started,scope='Fresh complete restore positive and affected actual refill/page/out-pointer guards; independent normal positive; unchanged campaigns reused.'),indent=2))
if __name__=='__main__':main()
