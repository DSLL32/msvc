#!/usr/bin/env python3
"""Mutate only finite record/class tables and ownership; no execution/discovery."""
import argparse
import copy
import json
from pathlib import Path
import shutil
import struct
import tempfile
import time
import record_data
from state import BINARY,TARGET,sha


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();start=time.monotonic()
    config=json.loads((BINARY/'build/trial.json').read_text());positive=record_data.verify(BINARY,config);passed=[]
    with tempfile.TemporaryDirectory(prefix='record-dispatch-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        folder=Path(folder);base=folder/'base';(base/'build').mkdir(parents=True);(base/'src').mkdir();(base/'cases/analysis').mkdir(parents=True)
        for name in ('c2.exe','c2.map'):shutil.copyfile(BINARY/'build'/name,base/'build'/name)
        shutil.copyfile(BINARY/'src/code-records.c',base/'src/code-records.c');shutil.copyfile(BINARY/'cases/analysis/prepare-records-tables.json',base/'cases/analysis/prepare-records-tables.json');shutil.copyfile(TARGET,base/'original.exe')
        def row(c):return next(r for r in c['driver']if r['name']=='@prepare_function_records@4')
        def pe_change(root,original,at,value):
            path=root/'original.exe'if original else root/'build/c2.exe';data=bytearray(path.read_bytes())
            # PE file offset from its own exact sectionheader, not an image dump.
            header=struct.unpack_from('<I',data,60)[0];optional=header+24;base=struct.unpack_from('<I',data,optional+28)[0];count=struct.unpack_from('<H',data,header+6)[0];sections=optional+struct.unpack_from('<H',data,header+20)[0];found=[]
            for i in range(count):
                _,rva,size,file=struct.unpack_from('<IIII',data,sections+i*40+8)
                if base+rva<=at and at+len(value)<=base+rva+size:found.append(file+at-base-rva)
            assert len(found)==1;point=found[0];assert data[point:point+len(value)]!=value;data[point:point+len(value)]=value;path.write_bytes(data)
            if original:
                p=root/'cases/analysis/prepare-records-tables.json';v=json.loads(p.read_text());v['target_sha256']=sha(data);p.write_text(json.dumps(v,indent=2)+'\n')
        record=next(r for r in positive['sides']['rebuilt']['intervals']);rebuilt=record[0];classes=positive['sides']['rebuilt']['intervals'][1][0];expressions=positive['sides']['rebuilt']['intervals'][2][0]
        cases=[('rebuilt record target content differs',lambda r,c:pe_change(r,False,rebuilt,struct.pack('<I',0x400000))),
            ('rebuilt class content differs',lambda r,c:pe_change(r,False,classes,b'\x01')),
            ('rebuilt expression alias differs',lambda r,c:pe_change(r,False,expressions,struct.pack('<I',positive['sides']['rebuilt']['expression_targets'][2]))),
            ('original record target differs with hash rebound',lambda r,c:pe_change(r,True,0x41f7a0,struct.pack('<I',0x41f64e))),
            ('original class content differs with hash rebound',lambda r,c:pe_change(r,True,0x41f7b0,b'\x01')),
            ('original expression target differs with hash rebound',lambda r,c:pe_change(r,True,0x41f7c0,struct.pack('<I',0x41f6ee))),
            ('original shared data owner omitted',lambda r,c:row(c).__setitem__('shared',[])),
            ('original shared classmap owner shortened',lambda r,c:row(c)['shared'][1].__setitem__(1,0x41f7be)),
            ('original shared gap envelope invented',lambda r,c:row(c).__setitem__('shared',[[0x41f793,0x41f7d4]])),
            ('text data classmap missing',lambda r,c:c.__setitem__('text_data',[v for v in c['text_data']if v['name']!='record-kind-classes'])),
            ('rebuilt data owner interval shortened',lambda r,c:row(c)['rebuilt_record_dispatch_offsets'][1].__setitem__(1,0x340)),
            ('rebuilt source byte identity differs',lambda r,c:(r/'src/code-records.c').write_text((r/'src/code-records.c').read_text()+'\n'))]
        for i,(name,change)in enumerate(cases):
            root=folder/str(i);shutil.copytree(base,root);c=copy.deepcopy(config);before=(json.dumps(c,sort_keys=True),sha((root/'original.exe').read_bytes()),sha((root/'build/c2.exe').read_bytes()),sha((root/'src/code-records.c').read_bytes()));change(root,c);after=(json.dumps(c,sort_keys=True),sha((root/'original.exe').read_bytes()),sha((root/'build/c2.exe').read_bytes()),sha((root/'src/code-records.c').read_bytes()));assert before!=after,'control premise unchanged: '+name;record_data.TARGET=root/'original.exe'
            try:record_data.verify(root,c)
            except(AssertionError,KeyError,ValueError)as error:passed.append({'control':name,'reason':str(error)})
            else:raise AssertionError('dispatch mutation accepted: '+name)
        record_data.TARGET=TARGET
    a.output.write_text(json.dumps({'positive':positive,'count':len(passed),'rejected':passed,'seconds':time.monotonic()-start,'checker_sha256':sha(Path(__file__).read_bytes()),'validator_sha256':sha((BINARY/'scripts/record_data.py').read_bytes()),'configuration_sha256':sha((BINARY/'build/trial.json').read_bytes()),'scope':'Exact encountered51B structured data and disjoint ownership only. Original hash is deliberately rebound in owned temporary controls to reach actual content checks; repository original never mutated. Commonstrictgraph audit separately handles all instruction/extent ownership.'},indent=2)+'\n')

if __name__=='__main__':main()
