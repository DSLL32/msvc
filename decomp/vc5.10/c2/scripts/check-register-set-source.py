#!/usr/bin/env python3
"""Finite source ownership/index/escape checks on isolated exact current files."""
import argparse,json,shutil,tempfile,time
from pathlib import Path
import register_set_data as data
from state import BINARY,sha

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();start=time.monotonic();config=json.loads((BINARY/'build/trial.json').read_text());original=data.BINARY;passed=[]
    with tempfile.TemporaryDirectory(prefix='set-source-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as temp:
        root=Path(temp)
        for name in ['src/register-sets.c','src/register-sets.h','src/function-records.c','src/function-records.h','src/stubs.c','cases/analysis/register-set-analysis.json','build/c2.map','build/c2.exe']:
            target=root/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(BINARY/name,target)
        data.BINARY=root
        def reject(name,fn):
            try:fn()
            except AssertionError as e:passed.append({'control':name,'reason':str(e)})
            else:raise AssertionError('unmodeled use accepted '+name)
        try:
            assert data.verify(config)
            for expression in ['C2RegisterSet *escape;','void *f(void){return free_register_sets;}','void *f(void){return free_register_sets[0];}','unsigned f(C2RegisterSet *s){return s->pending05[0];}','void f(void){new_register_set(12);}']:
                path=root/'src/unknown.c';path.write_text(expression);reject(expression,lambda:data.verify(config));path.unlink()
            path=root/'src/stubs.c';text=path.read_text();path.write_text(text+'\nvoid *escape(void){return free_register_sets;}\n');reject('table escape in pending-stub source',lambda:data.verify(config));path.write_text(text)
            path=root/'src/function-records.c';text=path.read_text()
            for index in ['0','2','12','pool']:
                changed=text.replace('new_register_set(1)','new_register_set('+index+')',1);assert changed!=text;path.write_text(changed);reject('caller index '+index,lambda:data.verify(config));path.write_text(text)
            path=root/'src/register-sets.h';text=path.read_text();changed=text.replace('(pool) == 1 ? 1 :','(pool) == 12 ? 12 :');assert changed!=text;path.write_text(changed);reject('guard changed',lambda:data.verify(config));path.write_text(text)
            path=root/'src/register-sets.c';text=path.read_text();changed=text.replace('free_register_sets[C2_REGISTER_SET_POOL(pool)]','free_register_sets[pool]',1);assert changed!=text;path.write_text(changed);reject('actual lookup unguarded',lambda:data.verify(config))
        finally:data.BINARY=original
    a.output.write_text(json.dumps({'count':len(passed),'controls':passed,'seconds':time.monotonic()-start,'checker_sha256':sha(Path(__file__).read_bytes()),'scope':'Actual production source ownership/caller/guard validator on isolated copies. No mutable target/raw files, no unknownslot credit.'},indent=2)+'\n')
if __name__=='__main__':main()
