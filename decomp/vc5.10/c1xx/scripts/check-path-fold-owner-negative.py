#!/usr/bin/env python3
"""Consistent owner-only sentinel mutations across every new fold phase."""
import argparse,copy,importlib.util,json,re,tempfile,time
from pathlib import Path
from path_fold_state import check as check_fold


def load(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path);p.add_argument('native',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args();original,native=a.original.resolve(),a.native.resolve();decoder=load('owner_decoder','check-callback.py');history=load('owner_history','check-history.py');bindings={side+'_receipt_sha256':decoder.s.sha(d/'receipt.json')for side,d in (('original',original),('native',native))};audit=history.configure(decoder,original,native,saved_record=bindings);decoder.check(original,native,audit_path=audit)
    trial=json.loads((native/'trial.json').read_text());state=json.loads((native/'state.json').read_text());pre=json.loads((native/'pre.json').read_text());phase=json.loads((native/'phase.json').read_text());text=(original/'events.txt').read_text();traps=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$',text,re.M)
    names=['path-fold-'+name+'.json'for name in ('entries','returns','api-entries','api-returns','next-entries')];baseline={name:json.loads((native/name).read_text())for name in names};results=[];start=time.perf_counter()
    with tempfile.TemporaryDirectory(prefix='fold-owner-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx')as tmp:
        work=Path(tmp)
        for f in native.iterdir():(work/f.name).symlink_to(f,target_is_directory=f.is_dir())
        for key in ('pctype','old_pctype','static_ctype','result_pointer','CRT-argv','CRT-environment'):
            for name in names:
                rows=copy.deepcopy(baseline[name])
                for row in rows:
                    locale=row['locale']
                    if key.startswith('CRT-'):locale[key]['address']+=4
                    else:
                        locale[key]+=2 if key!='result_pointer' else 1
                        if key=='result_pointer':locale['selection']['result']=locale[key]
                f=work/name;f.unlink();f.write_text(json.dumps(rows))
            try:check_fold(decoder.s,decoder.d,original,work,trial,text,pre,phase,state,trial.get('test_seed'),'%03d'%int(traps[-1][0]),traps)
            except(ValueError,AssertionError)as error:
                rejection=str(error);assert 'crossphase original CRT'in rejection,(key,rejection);results.append(dict(owner=key,rejection=rejection,mutation='same alternate interior owner in every fold phase; prior scanner-next untouched'))
            else:raise RuntimeError('consistent owner substitution accepted '+key)
            finally:
                for name in names:f=work/name;f.unlink();f.symlink_to(native/name)
    result=dict(kind='six fresh crossphase owner-only controls; allfold phases consistently mutated, no raw execution',bindings=bindings,count=len(results),cases=results,seconds=time.perf_counter()-start,checker_sha256=decoder.s.sha(Path(__file__)),semantic_checker_sha256=decoder.s.sha(Path(__file__).with_name('path_fold_state.py')),unchanged_base_controls='cases/minimal/evidence/path-fold-normal-controls-1.json');assert not a.output.exists();a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(count=len(results),seconds=result['seconds'])))
