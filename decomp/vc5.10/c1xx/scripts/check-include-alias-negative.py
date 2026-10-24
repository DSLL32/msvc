#!/usr/bin/env python3
"""Affected alias-root identity/cursor/neighbor/ABI denials; no execution."""
import argparse,copy,importlib.util,json,re,struct,tempfile,time
from pathlib import Path
from include_alias_state import check as check_alias


def load(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


def main():
    parser=argparse.ArgumentParser();parser.add_argument('original',type=Path);parser.add_argument('native',type=Path);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();o,n=args.original,args.native
    d=load('alias_decoder','check-callback.py');h=load('alias_history','check-history.py');record={side+'_receipt_sha256':d.s.sha(path/'receipt.json')for side,path in [('original',o),('native',n)]};audit=h.configure(d,o,n,saved_record=record);d.check(o,n,audit_path=audit)
    trial=json.loads((o/'trial.json').read_text());state=json.loads((n/'state.json').read_text());text=(o/'events.txt').read_text();traps=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$',text,re.M);final='%03d'%int(traps[-1][0]);seq=int(next(v[0]for v in traps if v[1]=='00039282'));cases=['request','entry-formal','entry-PC','native-root-prestate','original-root-prestate','native-out-cursor','native-out-root','native-unknown-neighbor','original-unknown-BSS','original-unknown-data','saved-snapshot','native-RET','native-nonvolatile','original-RET','original-nonvolatile','FP','next-formal','next-PC','next-caller','next-name','next-args','serialized-cursor','serialized-root','allocation-contents','original-caller'];passed=[];started=time.perf_counter()
    with tempfile.TemporaryDirectory(prefix='alias-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx')as tmp:
        original,native=Path(tmp)/'original',Path(tmp)/'native';original.mkdir();native.mkdir()
        for source,destination in [(o,original),(n,native)]:
            for path in source.iterdir():
                if path.is_file()or path.name=='source-content':(destination/path.name).symlink_to(path,target_is_directory=path.is_dir())
        for case in cases:
            astate=copy.deepcopy(state);atrial=copy.deepcopy(trial);atext=text;changed=[]
            def write(path,blob):
                source=path.resolve();path.unlink();path.write_bytes(blob);changed.append((path,source))
            def raw(path,offset):
                value=bytearray(path.read_bytes());value[offset]^=1;write(path,value)
            def js(name,change):
                path=native/(name+'.json');value=json.loads(path.read_text());change(value);write(path,json.dumps(value).encode())
            def guard_byte(rows,index,member,offset):
                value=bytearray.fromhex(rows[index]['guard'][member]);value[offset]^=1;rows[index]['guard'][member]=value.hex()
            if case=='request':atrial['alias_root_observers']['request_bytes']=8
            elif case=='entry-formal':js('options-next-entries',lambda v:v[0]['formal_inventory'].append(dict(name='invented',type='int')))
            elif case=='entry-PC':js('options-next-entries',lambda v:v[0].update(pc=v[0]['pc']+1))
            elif case=='native-root-prestate':js('options-next-entries',lambda v:guard_byte(v,0,'data',208))
            elif case=='original-root-prestate':raw(original/('frontend-data-%03d.bin'%seq),0x444)
            elif case=='native-out-cursor':js('options-alias-returns',lambda v:guard_byte(v,0,'bss',state['frontend']['storage']['arenas'][0]-state['frontend']['bss_address']))
            elif case=='native-out-root':js('options-alias-returns',lambda v:guard_byte(v,0,'data',208))
            elif case=='native-unknown-neighbor':js('options-alias-returns',lambda v:guard_byte(v,0,'data',0))
            elif case=='original-unknown-BSS':raw(original/'alias-return-bss.bin',63131)
            elif case=='original-unknown-data':raw(original/'frontend-data-alias-return.bin',0)
            elif case=='saved-snapshot':raw(original/'frontend-snapshot-alias-return.bin',8519)
            elif case=='native-RET':js('options-alias-returns',lambda v:v[0].update(esp=v[0]['esp']+4))
            elif case=='native-nonvolatile':js('options-alias-returns',lambda v:v[0].update(esi=v[0]['esi']+4))
            elif case=='original-RET':atext=re.sub(r'(alias-return pc=\w+ esp=)(\w+)',lambda m:m[1]+'%08x'%(int(m[2],16)+4),text)
            elif case=='original-nonvolatile':atext=re.sub(r'(alias-return .* esi=)(\w+)',lambda m:m[1]+'%08x'%(int(m[2],16)+4),text)
            elif case=='FP':js('options-alias-returns',lambda v:v[0]['fp'].update(cw=0x37f))
            elif case=='next-formal':js('options-next-entries',lambda v:v[1]['formal_inventory'].append(dict(name='invented',type='int')))
            elif case=='next-PC':js('options-next-entries',lambda v:v[1].update(pc=v[1]['pc']+1))
            elif case=='next-caller':js('options-next-entries',lambda v:v[1].update(return_pc=v[1]['return_pc']+1))
            elif case=='next-name':astate['symbol']='wrong_stub'
            elif case=='next-args':astate['args']['invented']=1
            elif case=='serialized-cursor':astate['frontend']['arenas'][0]+=4
            elif case=='serialized-root':astate['frontend']['options_data']['include_aliases']+=4
            elif case=='allocation-contents':raw(native/'pool-final-committed.bin',40960)
            elif case=='original-caller':atext=re.sub(r'(trap seq=%d .*stack=)(\w+)'%seq,lambda m:m[1]+'%08x'%(int(m[2],16)+1),text)
            else:raise RuntimeError(case)
            rows=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$',atext,re.M)
            try:check_alias(d.s,d.d,original,native,atrial,atext,astate,final,rows)
            except (ValueError,KeyError,AssertionError,struct.error)as error:passed.append(dict(case=case,rejection=str(error)))
            else:raise RuntimeError('mutant accepted: '+case)
            finally:
                for path,source in changed:path.unlink();path.symlink_to(source)
    result=dict(schema=1,kind='fresh affected semantics after complete paired/strict positive; no raw receipt mutation',original=str(o),native=str(n),bindings=record,count=len(passed),cases=passed,seconds=time.perf_counter()-started,checker_sha256=d.s.sha(Path(__file__)),semantic_checker_sha256=d.s.sha(Path(__file__).with_name('include_alias_state.py')));args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(count=len(passed),seconds=result['seconds'])))


if __name__=='__main__':main()
