#!/usr/bin/env python3
"""Check every encountered record field and rank copy/reset/result controls."""
import importlib.util
import json
from pathlib import Path
import re
import shutil
import struct
import tempfile
import time


def load(name, file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    return module


def main():
    d=load('rank_helper_decoder','check-callback.py')
    h=load('rank_helper_negative','check-projection-negative.py')
    history=load('rank_helper_history','check-history.py')
    packet=json.loads((d.s.PROJECT/'cases/minimal/rank-helper-trials.json').read_text())
    started=time.perf_counter();passed=[]
    for row in packet['trials']:
        original,native=Path(row['original']),Path(row['native'])
        audit=history.configure(d,original,native,saved_record=row);d.check(original,native,audit_path=audit)
        if row['seed'] is None:
            cases=[('record',name,i,field) for name,count in [('primitive_type_ranks',8),('complex_type_ranks',9)] for i in range(count) for field in (0,1)]
            cases += [('original-default',name,0,0) for name in ('primitive_type_ranks','complex_type_ranks')]
            cases += [('result',name,0,0) for name in ('primitive_type_rank','complex_type_rank')]
        elif row['seed']=='restore-nonzero':
            cases=[(kind,name,0,0) for kind in ('prestate','saved-input','BSS-clear','native-ready') for name in ('primitive_type_ranks','complex_type_ranks')]
        else:
            cases=[(kind,name,0,0) for kind in ('original-result','original-clear','wrong-domain','wrong-stride') for name in ('primitive_type_ranks','complex_type_ranks')]
        for kind,name,index,field in cases:
            case='%s:%s[%d].%s'%(kind,name,index,'rank' if field else 'code')
            with tempfile.TemporaryDirectory(prefix='rank-helper-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as tmp:
                o,n=Path(tmp)/'original',Path(tmp)/'native';shutil.copytree(original,o);shutil.copytree(native,n)
                if kind=='original-default':
                    f=o/'frontend-data-000.bin';raw=bytearray(f.read_bytes());offset=0x1708 if name.startswith('primitive') else 0x1748;struct.pack_into('<i',raw,offset,99);f.write_bytes(raw);h.rehash(o,f.name)
                elif kind in ('original-result','original-clear'):
                    final='%03d'%int(re.findall(r'^trap seq=(\d+) rva=',(o/'events.txt').read_text(),re.M)[-1]);suffix=final if kind=='original-result' else 'phase'
                    fname=('primitive-type-rank' if name.startswith('primitive') else 'complex-type-rank')+'-'+suffix+'.bin';f=o/fname;raw=bytearray(f.read_bytes());raw[0]=255;f.write_bytes(raw);h.rehash(o,fname)
                else:
                    fname='pre.json' if kind in ('prestate','saved-input') else 'phase.json' if kind in ('record','BSS-clear') else 'state-model.json' if kind=='wrong-domain' else 'state.json'
                    def change(j):
                        if kind=='wrong-domain':j['type_rank_projection']['primitive_output_bytes' if name.startswith('primitive') else 'complex_output_bytes']+=1;return
                        f=j['frontend']
                        if kind=='record':f[name][index][field]+=1
                        elif kind=='prestate':f[name][0][0]=0
                        elif kind=='saved-input':f['saved_'+name][0][1]=0
                        elif kind=='BSS-clear':f['primitive_type_rank' if name.startswith('primitive') else 'complex_type_rank'][0]=181
                        elif kind=='native-ready':f['type_ranks_initialized']=0
                        elif kind=='wrong-stride':f['storage'][name+'[1].code'][0]+=4
                        elif kind=='result':f[name][0]=255
                        else:raise RuntimeError(case)
                    h.edit(n/fname,change);h.rehash(n,fname)
                h.rejected(case,lambda:d.check(o,n,audit_path=audit));passed.append(case)
    print(json.dumps({'result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'rejected':passed},indent=2))


if __name__=='__main__':main()
