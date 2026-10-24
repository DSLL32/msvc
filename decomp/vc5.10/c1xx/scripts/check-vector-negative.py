#!/usr/bin/env python3
"""Affected vector/arena/public-entry semantic controls; no execution."""
import argparse,copy,importlib.util,json,re,tempfile,time
from pathlib import Path
from vector_state import check as check_vector


def module(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path);p.add_argument('native',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args();o,n=a.original.resolve(),a.native.resolve();decoder=module('vector_negative','check-callback.py');history=module('vector_negative_history','check-history.py');bindings={side+'_receipt_sha256':decoder.s.sha(d/'receipt.json')for side,d in [('original',o),('native',n)]}
    start=time.perf_counter();audit=history.configure(decoder,o,n,saved_record=bindings);positive=decoder.check(o,n,audit_path=audit);positive_seconds=time.perf_counter()-start
    trial=json.loads((n/'trial.json').read_text());state=json.loads((n/'state.json').read_text());prefix=json.loads((n/'vector-prefix-state.json').read_text());text=(o/'events.txt').read_text()
    cases=['call-count','capacity-formal','layout','returned-owner','header-count','pointer-array','committed-tail','member-this','member-RET','API-input','API-result','API-cleanup','owned-reserve','commit-zero','final-neighbor','public-PC','public-count','public-seed','public-input','public-formal','final-raw-ABI','unknown-target-suffix','query-state','all-vector-owner']
    if trial.get('test_seed'):cases=['final-neighbor','next-formal','restore-discriminator','derived-tail','next-caller']
    start=time.perf_counter();results=[]
    with tempfile.TemporaryDirectory(prefix='vector-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx')as tmp:
        work=Path(tmp)
        for f in n.iterdir():(work/f.name).symlink_to(f,target_is_directory=f.is_dir())
        for case in cases:
            changed=[];st,pr=copy.deepcopy(state),copy.deepcopy(prefix);t=text
            def write(name,data):
                f=work/name;f.unlink();f.write_bytes(data);changed.append(f)
            def js(name,fn):
                file='vector-'+name+'.json';rows=json.loads((n/file).read_text());fn(rows);write(file,json.dumps(rows).encode())
            def flip(row,key,index):
                raw=bytearray.fromhex(row[key]);raw[index]^=1;row[key]=raw.hex()
            if case=='call-count':js('entries',lambda r:r.pop())
            elif case=='capacity-formal':js('entries',lambda r:r[0]['formal_inventory'][0].update(type='int'))
            elif case=='layout':js('entries',lambda r:r[0]['layout']['fields']['capacity'].__setitem__(2,'unsigned short'))
            elif case=='returned-owner':js('returns',lambda r:r[1].update(eax=r[1]['eax']+4))
            elif case=='header-count':js('returns',lambda r:r[1]['header'].__setitem__(0,1))
            elif case=='pointer-array':js('returns',lambda r:flip(r[1],'items',159))
            elif case in ('committed-tail','commit-zero'):
                name='vector-2-committed.bin'if case=='committed-tail'else'vector-commit-zero.bin';raw=bytearray((n/name).read_bytes());raw[-1]^=1;write(name,raw)
            elif case=='member-this':js('member-entries',lambda r:r[0].update(ecx=r[0]['ecx']-32))
            elif case=='member-RET':js('member-returns',lambda r:r[0].update(esp=r[0]['esp']+4))
            elif case=='API-input':js('api-entries',lambda r:r[0]['args'].__setitem__(1,65536))
            elif case=='API-result':js('api-returns',lambda r:r[0].update(eax=r[0]['eax']+4))
            elif case=='API-cleanup':js('api-returns',lambda r:r[0].update(esp=r[0]['esp']-16))
            elif case=='owned-reserve':js('api-returns',lambda r:r[0].update(owned_reservations=[]))
            elif case=='final-neighbor':st['frontend']['filename_case_map'][255]^=1
            elif case=='public-PC':js('public-entries',lambda r:r[0].update(pc=r[0]['pc']+1))
            elif case=='public-count':js('public-entries',lambda r:r[0]['args'].__setitem__(1,1))
            elif case=='public-seed':js('public-entries',lambda r:r[0]['args'].__setitem__(2,0))
            elif case=='public-input':js('public-entries',lambda r:flip(r[0],'input',2))
            elif case=='public-formal':js('public-entries',lambda r:r[0]['formal_inventory'][1].update(type='unsigned long'))
            elif case=='final-raw-ABI':st['raw_entry_abi']['stack_arguments'][1]=1
            elif case=='unknown-target-suffix':t+='trap seq=201 rva=00039428 extra=1\n'
            elif case=='query-state':
                t=t.replace('bytes=00008000 state=00001000','bytes=00008000 state=00002000');assert t!=text
            elif case=='all-vector-owner':
                for stem in ['entries','returns','member-entries','member-returns','api-entries','api-returns','public-entries']:
                    js(stem,lambda rows:[r['locale'].update(pctype=r['locale']['pctype']+2)for r in rows])
            elif case=='next-formal':js('next-entries',lambda r:r[0]['formal_inventory'].append(dict(name='invented',type='int')))
            elif case=='restore-discriminator':pr['frontend']['option_c9il']=0
            elif case=='derived-tail':
                # The original last next-stub return must be the actual caller.
                t=re.sub(r'(^trap seq=\d+ rva=00039428 .*stack=)104390b3',r'\g<1>104390b4',text,flags=re.M);assert t!=text
            elif case=='next-caller':js('next-entries',lambda r:r[0].update(return_pc=r[0]['return_pc']+1))
            else:raise ValueError(case)
            try:check_vector(decoder.s,decoder.d,o,work,trial,t,st,pr)
            except(ValueError,AssertionError,KeyError)as e:results.append(dict(case=case,rejection=str(e)))
            else:raise ValueError('vector mutant accepted: '+case)
            finally:
                for f in changed:f.unlink();f.symlink_to(n/f.name)
    result=dict(kind='fresh affected vector/arena/provider entry controls; reused prior raw controller campaigns',original=str(o),native=str(n),bindings=bindings,positive=positive,positive_seconds=positive_seconds,count=len(results),cases=results,seconds=time.perf_counter()-start,checker_sha256=decoder.s.sha(Path(__file__)),semantic_checker_sha256=decoder.s.sha(Path(__file__).with_name('vector_state.py')))
    if a.output.exists():raise ValueError('refuse overwrite')
    a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(count=len(results),seconds=result['seconds'],positive_seconds=positive_seconds)))


if __name__=='__main__':main()
