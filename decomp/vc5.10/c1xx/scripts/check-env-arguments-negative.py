#!/usr/bin/env python3
"""Bounded new parser/allocator observation mutations, never execution."""
import argparse,copy,importlib.util,json,re,tempfile,time
from pathlib import Path
from env_arguments_state import check as check_env


def module(name,path):
    spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path);p.add_argument('native',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    script=Path(__file__).resolve();decoder=module('env_negative_decoder',script.with_name('check-callback.py'));history=module('env_negative_history',script.with_name('check-history.py'))
    o,n=a.original.resolve(),a.native.resolve();bindings={key+'_receipt_sha256':decoder.s.sha(d/'receipt.json')for key,d in [('original',o),('native',n)]}
    start=time.perf_counter();audit=history.configure(decoder,o,n,saved_record=bindings);positive=decoder.check(o,n,audit_path=audit);positive_seconds=time.perf_counter()-start
    trial=json.loads((n/'trial.json').read_text());state=json.loads((n/'state.json').read_text());pre=json.loads((n/'pre.json').read_text());phase=json.loads((n/'phase.json').read_text());text=(o/'events.txt').read_text();traps=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$',text,re.M)
    cases=['NULL-input','source-formal','return-stack','return-register','return-target-byte','return-saved-byte','next-capacity','next-arena','next-formal','next-PC','next-caller','warning-extra-byte','return-owner','return-FP','private-vector','original-return-error']
    result=[];start=time.perf_counter()
    with tempfile.TemporaryDirectory(prefix='env-arguments-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx')as tmp:
        work=Path(tmp)
        for f in n.iterdir():(work/f.name).symlink_to(f,target_is_directory=f.is_dir())
        for case in cases:
            changed=[];mutant_text=text
            def write(name,data):
                f=work/name;f.unlink();f.write_bytes(data);changed.append(f)
            def js(stem,fn):
                name='env-arguments-'+stem+'.json';rows=json.loads((n/name).read_text());fn(rows[0]);write(name,json.dumps(rows).encode())
            def flip(row,key,offset):
                raw=bytearray.fromhex(row[key]);raw[offset]^=1;row[key]=raw.hex()
            if case=='NULL-input':js('entries',lambda r:r['args'].__setitem__(0,1))
            elif case=='source-formal':js('entries',lambda r:r['formal_inventory'][0].update(type='int'))
            elif case=='return-stack':js('returns',lambda r:r.update(esp=r['esp']+4))
            elif case=='return-register':js('returns',lambda r:r.update(ebx=r['ebx']^4))
            elif case=='return-target-byte':js('returns',lambda r:flip(r['guard'],'bss',0))
            elif case=='return-saved-byte':js('returns',lambda r:flip(r['guard'],'saved',0))
            elif case=='next-capacity':js('next',lambda r:r['args'].__setitem__(0,41))
            elif case=='next-arena':js('next',lambda r:r['args'].__setitem__(1,0))
            elif case=='next-formal':js('next',lambda r:r['formal_inventory'][0].update(type='int'))
            elif case=='next-PC':js('next',lambda r:r.update(pc=r['pc']+1))
            elif case=='next-caller':js('next',lambda r:r.update(return_pc=r['return_pc']+1))
            elif case=='warning-extra-byte':js('next',lambda r:flip(r['guard'],'data',0))
            elif case=='return-owner':js('returns',lambda r:r['locale'].update(pctype=r['locale']['pctype']+2))
            elif case=='return-FP':js('returns',lambda r:r['fp'].update(sw=r['fp']['sw']^1))
            elif case=='private-vector':
                name='locale-native-env-return-CRT-environment.bin';raw=bytearray((n/name).read_bytes());raw[-2]^=1;write(name,raw)
            elif case=='original-return-error':
                mutant_text=re.sub(r'(^env-arguments-return .*error=)203$',r'\g<1>204',text,flags=re.M)
                if mutant_text==text:raise ValueError('missing original return mutation premise')
            else:raise ValueError(case)
            try:check_env(decoder.s,decoder.d,o,work,trial,mutant_text,pre,phase,state,trial.get('test_seed'),'%03d'%int(traps[-1][0]),traps)
            except(ValueError,AssertionError,KeyError)as e:result.append(dict(case=case,rejection=str(e)))
            else:raise ValueError('mutation accepted: '+case)
            finally:
                for f in changed:f.unlink();f.symlink_to(n/f.name)
    out=dict(kind='fresh affected parser+next allocator controls; existing controller campaigns reused unchanged',original=str(o),native=str(n),bindings=bindings,positive=positive,positive_seconds=positive_seconds,count=len(result),cases=result,seconds=time.perf_counter()-start,checker_sha256=decoder.s.sha(script),semantic_checker_sha256=decoder.s.sha(script.with_name('env_arguments_state.py')))
    if a.output.exists():raise ValueError('refuse overwrite')
    a.output.write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(dict(count=len(result),seconds=out['seconds'],positive_seconds=positive_seconds)))


if __name__=='__main__':main()
