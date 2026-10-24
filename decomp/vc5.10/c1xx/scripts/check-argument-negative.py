#!/usr/bin/env python3
"""Focused iterator/borrowed-pointer/matcher controls; no executable runs."""
import argparse,copy,importlib.util,json,struct,tempfile,time
from pathlib import Path
from argument_state import check


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path);p.add_argument('native',type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();start=time.perf_counter()
    sp=importlib.util.spec_from_file_location('argument_decoder',Path(__file__).with_name('check-callback.py'));m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
    o,n=a.original.resolve(),a.native.resolve();trial=json.loads((n/'trial.json').read_text());text=(o/'events.txt').read_text();state=json.loads((n/'state.json').read_text());entry=json.loads((n/'argument-entry-state.json').read_text())
    positive=check(m.s,m.d,o,n,trial,text,state,entry,{})
    cases=['input-count','input-root','return-stack','return-register','return-pointer','return-string','return-neighbor','return-owner','return-FP','matcher-rules','matcher-argument','matcher-callback','matcher-copy','matcher-formal','matcher-PC','matcher-caller','matcher-owner','final-count','final-root','final-private-vector','original-return-pointer','original-return-neighbor','original-final-formal','candidate-range']
    results=[]
    with tempfile.TemporaryDirectory(prefix='argument-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx')as tmp:
        root=Path(tmp);no,nn=root/'original',root/'native';no.mkdir();nn.mkdir()
        for directory,source in((no,o),(nn,n)):
            for f in source.iterdir():(directory/f.name).symlink_to(f,target_is_directory=f.is_dir())
        for case in cases:
            changed=[];st=copy.deepcopy(state);en=copy.deepcopy(entry);tr=copy.deepcopy(trial);tx=text
            def write(directory,name,raw):
                f=directory/name;f.unlink();f.write_bytes(raw);changed.append((f,(o if directory==no else n)/name))
            def js(name,fn):
                rows=json.loads((n/name).read_text());fn(rows[0]);write(nn,name,json.dumps(rows).encode())
            def flip_guard(r):
                b=bytearray.fromhex(r['guard']['bss']);b[0]^=1;r['guard']['bss']=b.hex()
            if case=='input-count':en['frontend']['command_argc']=25
            elif case=='input-root':en['frontend']['command_argv']+=4
            elif case=='return-stack':js('argument-returns.json',lambda r:r.update(esp=r['esp']+4))
            elif case=='return-register':js('argument-returns.json',lambda r:r.update(esi=r['esi']^4))
            elif case=='return-pointer':js('argument-returns.json',lambda r:r.update(eax=r['eax']+1))
            elif case=='return-string':js('argument-returns.json',lambda r:r.update(result='2d696d00'))
            elif case=='return-neighbor':js('argument-returns.json',flip_guard)
            elif case=='return-owner':js('argument-returns.json',lambda r:r['locale'].update(pctype=r['locale']['pctype']+2))
            elif case=='return-FP':js('argument-returns.json',lambda r:r['fp'].update(sw=r['fp']['sw']^1))
            elif case.startswith('matcher-'):
                field=case[8:]
                if field in('rules','argument','callback','copy'):js('argument-matchers.json',lambda r:r['args'].__setitem__(['rules','argument','callback','copy'].index(field),r['args'][['rules','argument','callback','copy'].index(field)]^1))
                elif field=='formal':js('argument-matchers.json',lambda r:r['formal_inventory'][3].update(type='void *'))
                elif field=='PC':js('argument-matchers.json',lambda r:r.update(pc=r['pc']+1))
                elif field=='caller':js('argument-matchers.json',lambda r:r.update(return_pc=r['return_pc']+1))
                elif field=='owner':js('argument-matchers.json',lambda r:r['locale'].update(result_pointer=r['locale']['result_pointer']+1))
                else:raise ValueError(field)
            elif case=='final-count':st['frontend']['command_argc']=24
            elif case=='final-root':st['frontend']['command_argv']+=4
            elif case=='final-private-vector':
                name='locale-native-argument-return-CRT-environment.bin';b=bytearray((n/name).read_bytes());b[-2]^=1;write(nn,name,b)
            elif case=='original-return-pointer':
                name='argument-return-registers.bin';b=bytearray((o/name).read_bytes());struct.pack_into('<I',b,12,struct.unpack_from('<I',b,12)[0]+1);write(no,name,b)
            elif case=='original-return-neighbor':
                name='locale-argument-return-target-BSS.bin';b=bytearray((o/name).read_bytes());b[0]^=1;write(no,name,b)
            elif case=='original-final-formal':
                needle='104390be,104d6160,';assert needle in tx;tx=tx.replace(needle,'104390be,104d6164,')
            elif case=='candidate-range':tr['ranges'][-1]['end_rva']-=1
            else:raise ValueError(case)
            try:check(m.s,m.d,no,nn,tr,tx,st,en,{})
            except(ValueError,AssertionError,KeyError)as e:results.append(dict(case=case,rejection=str(e)))
            else:raise ValueError('mutation accepted: '+case)
            finally:
                for f,source in changed:f.unlink();f.symlink_to(source)
    result=dict(kind='restore-only iterator/next matcher affected controls; unchanged prefix/protocol campaigns reused with their immutable vector receipts',original=str(o),native=str(n),original_receipt_sha256=m.s.sha(o/'receipt.json'),native_receipt_sha256=m.s.sha(n/'receipt.json'),checker_sha256=m.s.sha(Path(__file__)),semantic_checker_sha256=m.s.sha(Path(__file__).with_name('argument_state.py')),positive=positive,count=len(results),cases=results,seconds=time.perf_counter()-start)
    if a.output.exists():raise ValueError('refuse overwrite')
    a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({'count':len(results),'seconds':result['seconds']}))
if __name__=='__main__':main()
