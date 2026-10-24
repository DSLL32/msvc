#!/usr/bin/env python3
"""One coherent later return-site substitution after immutable-ELF validation."""
import argparse,importlib.util,json,re,tempfile,time
from pathlib import Path
from path_fold_state import check as check_fold


def load(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path);p.add_argument('native',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args();original,native=a.original.resolve(),a.native.resolve();d=load('callsite_decoder','check-callback.py');h=load('callsite_history','check-history.py');binding={side+'_receipt_sha256':d.s.sha(f/'receipt.json')for side,f in (('original',original),('native',native))};audit=h.configure(d,original,native,saved_record=binding);d.check(original,native,audit_path=audit)
    read=lambda name:json.loads((native/name).read_text());trial,state,pre,phase=(read(name)for name in ('trial.json','state.json','pre.json','phase.json'));text=(original/'events.txt').read_text();traps=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$',text,re.M);start=time.perf_counter()
    with tempfile.TemporaryDirectory(prefix='fold-callsite-control-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx')as tmp:
        work=Path(tmp)
        for f in native.iterdir():(work/f.name).symlink_to(f,target_is_directory=f.is_dir())
        for name,key in (('path-fold-api-entries.json','return_pc'),('path-fold-api-returns.json','pc')):
            data=read(name);data[65][key]+=1;f=work/name;f.unlink();f.write_text(json.dumps(data))
        try:check_fold(d.s,d.d,original,work,trial,text,pre,phase,state,trial.get('test_seed'),'%03d'%int(traps[-1][0]),traps)
        except(ValueError,AssertionError)as error:
            rejection=str(error);assert 'actual public callsite differs'in rejection,rejection
        else:raise RuntimeError('coherent wrong later callsite accepted')
    result=dict(kind='one coherent later call/return-PC substitution; unchanged immutable ELF and all per-row ABI/state values',bindings=binding,count=1,rejection=rejection,seconds=time.perf_counter()-start,checker_sha256=d.s.sha(Path(__file__)),semantic_checker_sha256=d.s.sha(Path(__file__).with_name('path_fold_state.py')));assert not a.output.exists();a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(count=1,seconds=result['seconds'])))
