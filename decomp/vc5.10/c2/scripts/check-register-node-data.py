#!/usr/bin/env python3
"""Actual factory table/field validator rejects targeted original/rebuilt changes."""
import argparse,copy,json,re,shutil,struct,tempfile,time
from pathlib import Path
import register_node_data as data
from state import BINARY,TARGET,sha

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();start=time.monotonic();base=json.loads((BINARY/'build/trial.json').read_text());original_root,original_target=data.BINARY,data.TARGET;passed=[]
    def offset(raw,va):
        h=struct.unpack_from('<I',raw,60)[0];op=h+24;image=struct.unpack_from('<I',raw,op+28)[0];count=struct.unpack_from('<H',raw,h+6)[0];sections=op+struct.unpack_from('<H',raw,h+20)[0]
        for i in range(count):
            virtual,rva,size,file=struct.unpack_from('<IIII',raw,sections+40*i+8)
            if image+rva<=va<image+rva+size:return file+va-image-rva
        raise AssertionError('unbacked mutation')
    with tempfile.TemporaryDirectory(prefix='node-data-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as temp:
        root=Path(temp)
        names=['src/function-records.c','src/function-records.h','cases/analysis/register-node-analysis.json','build/c2.map','build/c2.exe']
        originals={name:(BINARY/name).read_bytes()for name in names};target_bytes=TARGET.read_bytes()
        for name,raw in originals.items():q=root/name;q.parent.mkdir(parents=True,exist_ok=True);q.write_bytes(raw)
        target=root/'original.exe';target.write_bytes(target_bytes);data.BINARY=root;data.TARGET=target
        try:
            assert data.verify(base)
            maps={m[1]:int(m[2],16)for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',(root/'build/c2.map').read_text(),re.M)};entry=maps['@new_function_register@4']
            cases=[('original kind4 class','original',0x401e2d),('original jump target','original',0x401e10),('rebuilt kind4 class','rebuilt',entry+0x25c),('rebuilt jump target','rebuilt',entry+0x243)]
            for label,side,at in cases:
                config=copy.deepcopy(base);path=target if side=='original'else root/'build/c2.exe';initial=path.read_bytes();raw=bytearray(initial);index=offset(raw,at);raw[index]^=1;assert raw!=initial;path.write_bytes(raw)
                # Rebind only transport identity so this reaches content checks.
                if side=='original':
                    model=json.loads(originals['cases/analysis/register-node-analysis.json']);model['target_sha256']=sha(raw);content=(json.dumps(model,indent=2)+'\n').encode();(root/'cases/analysis/register-node-analysis.json').write_bytes(content);next(r for r in config['driver']if r['name']=='@new_function_register@4')['register_node_model_sha256']=sha(content)
                try:data.verify(config)
                except AssertionError as e:passed.append({'control':label,'reason':str(e)})
                else:raise AssertionError('table mutation accepted '+label)
                path.write_bytes(initial);(root/'cases/analysis/register-node-analysis.json').write_bytes(originals['cases/analysis/register-node-analysis.json'])
            for name,before,after in [('field offset','src/function-records.h','    struct C2FunctionRegister *next_free;')]:
                path=root/before;raw=path.read_text();changed=raw.replace(after,'    unsigned wrong_width;\n'+after);assert changed!=raw;path.write_text(changed)
                try:data.verify(base)
                except AssertionError as e:passed.append({'control':name,'reason':str(e)})
                else:raise AssertionError('field source mutation accepted')
                path.write_text(raw)
        finally:data.BINARY=original_root;data.TARGET=original_target
    a.output.write_text(json.dumps({'count':len(passed),'controls':passed,'seconds':time.monotonic()-start,'checker_sha256':sha(Path(__file__).read_bytes()),'validator_sha256':sha((BINARY/'scripts/register_node_data.py').read_bytes()),'scope':'Actual source/data validator on isolated exact PE/MAP/source/model copies. Original transport identity rebound only to reach content checks; actual target/raw untouched. Case meanings and code identities remain independently bound.'},indent=2)+'\n')
if __name__=='__main__':main()
