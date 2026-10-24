#!/usr/bin/env python3
"""Finite source consumer/escape guard controls with no worktree changes."""
import argparse,json,shutil,tempfile,time
from pathlib import Path
import function_register_data as data
from state import BINARY,sha

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();start=time.monotonic();config=json.loads((BINARY/'functions.json').read_text());original=data.BINARY;passed=[]
    with tempfile.TemporaryDirectory(prefix='block-source-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as temp:
        root=Path(temp)
        for name in ['src/function-records.c','src/function-records.h','src/stubs.c','cases/analysis/register-block-analysis.json','cases/analysis/function-records-analysis.json']:
            target=root/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(BINARY/name,target)
        data.BINARY=root
        try:
            assert data.verify(config)
            for expression in ['int f(void){return tracked_register_block->records[0].pending05[0];}','void *f(void){return first_function_register_block;}','void f(void){empty_function_register->pending32[0]=1;}','C2FunctionRegister *escaped;']:
                path=root/'src/unknown.c';path.write_text(expression)
                try:data.verify(config)
                except AssertionError as e:passed.append({'control':expression,'reason':str(e)})
                else:raise AssertionError('unknown source use accepted')
                path.unlink()
            path=root/'src/function-records.c';text=path.read_text();changed=text.replace('reg->number = next_function_register_number++;','{ reg->number = next_function_register_number++; reg->kind=0; }',1);assert changed!=text;path.write_text(changed)
            try:data.verify(config)
            except AssertionError as e:passed.append({'control':'known source field access changed','reason':str(e)})
            else:raise AssertionError('unreviewed source field changed')
        finally:data.BINARY=original
    a.output.write_text(json.dumps({'count':len(passed),'controls':passed,'seconds':time.monotonic()-start,'checker_sha256':sha(Path(__file__).read_bytes()),'scope':'Actual finite production source/escape guard, isolated copies only. No mutable worktree/PE/raw evidence; actual pending named stub signature remains allowed, no callable/body credit.'},indent=2)+'\n')
if __name__=='__main__':main()
