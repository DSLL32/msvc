#!/usr/bin/env python3
"""Affected filename-map/API-vector semantic controls; no target execution."""
import argparse,copy,importlib.util,json,re,tempfile,time
from pathlib import Path
from path_fold_state import check as check_fold


def load(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('original',type=Path);parser.add_argument('native',type=Path);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();original,native=args.original.resolve(),args.native.resolve()
    decoder=load('fold_negative','check-callback.py');history=load('fold_history','check-history.py');bindings={side+'_receipt_sha256':decoder.s.sha(path/'receipt.json')for side,path in (('original',original),('native',native))};audit=history.configure(decoder,original,native,saved_record=bindings);decoder.check(original,native,audit_path=audit)
    trial=json.loads((native/'trial.json').read_text());state=json.loads((native/'state.json').read_text());pre=json.loads((native/'pre.json').read_text());phase=json.loads((native/'phase.json').read_text());text=(original/'events.txt').read_text();traps=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$',text,re.M)
    cases=('entry-PC','entry-formal','map-alias','input-live','BSS-clear','helper-input','map-output','serialized-map','neighbor','helper-RET','helper-nonvolatile','next-input','next-formal','public-count','public-input','public-formal','public-unsigned-result','public-RET','public-nonvolatile','public-FP','public-state','public-partial-map','private-vector')
    if trial.get('test_seed'):cases=('input-live','BSS-clear','helper-input','map-output')
    start=time.perf_counter();results=[]
    with tempfile.TemporaryDirectory(prefix='path-fold-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx')as tmp:
        work=Path(tmp)
        for p in native.iterdir():(work/p.name).symlink_to(p,target_is_directory=p.is_dir())
        for case in cases:
            st,pr,ph=copy.deepcopy(state),copy.deepcopy(pre),copy.deepcopy(phase);changed=[]
            def write(name,data):
                p=work/name;p.unlink();p.write_bytes(data);changed.append(p)
            def js(name,fn):
                rows=json.loads((work/('path-fold-'+name+'.json')).read_text());fn(rows);write('path-fold-'+name+'.json',json.dumps(rows).encode())
            def flip(row,key,index):
                data=bytearray.fromhex(row[key]);data[index]^=1;row[key]=data.hex()
            if case=='entry-PC':js('entries',lambda r:r[0].update(pc=r[0]['pc']+1))
            elif case=='entry-formal':js('entries',lambda r:r[0]['formal_inventory'].append(dict(name='invented',type='int')))
            elif case=='map-alias':st['frontend']['storage']['filename_case_map'][0]+=2
            elif case=='input-live':pr['frontend']['filename_case_map'][255]^=1
            elif case=='BSS-clear':ph['frontend']['filename_case_map'][255]=1
            elif case=='helper-input':js('entries',lambda r:flip(r[0],'map',511))
            elif case=='map-output':js('returns',lambda r:flip(r[0],'map',511))
            elif case=='serialized-map':st['frontend']['filename_case_map'][255]^=1
            elif case=='neighbor':js('returns',lambda r:flip(r[0]['guard'],'bss',2310))
            elif case=='helper-RET':js('returns',lambda r:r[0].update(esp=r[0]['esp']+4))
            elif case=='helper-nonvolatile':js('returns',lambda r:r[0].update(ebp=r[0]['ebp']^4))
            elif case=='next-input':js('next-entries',lambda r:r[0]['args'].__setitem__(0,1))
            elif case=='next-formal':js('next-entries',lambda r:r[0]['formal_inventory'][0].update(type='int'))
            elif case=='public-count':js('api-entries',lambda r:r.pop())
            elif case=='public-input':js('api-entries',lambda r:r[65]['args'].__setitem__(0,256))
            elif case=='public-formal':js('api-entries',lambda r:r[65]['formal_inventory'][0].update(type='int'))
            elif case=='public-unsigned-result':js('api-returns',lambda r:r[65].update(eax=r[65]['eax']|65536))
            elif case=='public-RET':js('api-returns',lambda r:r[65].update(esp=r[65]['esp']+4))
            elif case=='public-nonvolatile':js('api-returns',lambda r:r[65].update(ebp=r[65]['ebp']^4))
            elif case=='public-FP':js('api-returns',lambda r:r[65]['fp'].update(cw=0x37f))
            elif case=='public-state':js('api-returns',lambda r:r[65]['locale'].update(mb_gate=1))
            elif case=='public-partial-map':js('api-entries',lambda r:flip(r[65],'map',100))
            elif case=='private-vector':
                name='locale-native-fold-api-065-after-CRT-environment.bin';data=bytearray((work/name).read_bytes());data[-2]^=1;write(name,data)
            else:raise RuntimeError(case)
            try:check_fold(decoder.s,decoder.d,original,work,trial,text,pr,ph,st,trial.get('test_seed'),'%03d'%int(traps[-1][0]),traps)
            except(ValueError,AssertionError,KeyError)as error:results.append(dict(case=case,rejection=str(error)))
            else:raise RuntimeError('filename-fold mutant accepted: '+case)
            finally:
                for p in changed:p.unlink();p.symlink_to(native/p.name)
    result=dict(kind='fresh affected filename projection/public-vector semantic controls after whole strict positive; no original/native execution',bindings=bindings,original_directory=str(original),native_directory=str(native),count=len(results),cases=results,seconds=time.perf_counter()-start,checker_sha256=decoder.s.sha(Path(__file__)),semantic_checker_sha256=decoder.s.sha(Path(__file__).with_name('path_fold_state.py')));assert not args.output.exists();args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(count=len(results),seconds=result['seconds'])))
