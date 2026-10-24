#!/usr/bin/env python3
"""Affected scalar context/restore/entry/return/next-branch denials, Linux-only."""
import argparse,copy,importlib.util,json,re,struct,tempfile,time
from pathlib import Path
from language_context_state import check as check_context


def module(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


def main():
    parser=argparse.ArgumentParser();parser.add_argument('original',type=Path);parser.add_argument('native',type=Path);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();o,n=args.original,args.native
    d=module('language_decoder','check-callback.py');h=module('language_history','check-history.py');record={side+'_receipt_sha256':d.s.sha(path/'receipt.json')for side,path in [('original',o),('native',n)]};audit=h.configure(d,o,n,saved_record=record);d.check(o,n,audit_path=audit)
    trial=json.loads((o/'trial.json').read_text());text=(o/'events.txt').read_text();state=json.loads((n/'state.json').read_text());pre=json.loads((n/'pre.json').read_text());phase=json.loads((n/'phase.json').read_text());traps=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$',text,re.M);final='%03d'%int(traps[-1][0]);seed=trial.get('test_seed')
    cases=['original-default','native-pre-depth','native-reset-slot','saved-depth','actual-entry-scalar','actual-entry-formal','missing-return','native-return-stack','native-return-nonvolatile','original-return-stack','original-return-nonvolatile','native-own-neighbor','original-unknown-neighbor','original-slot-write','final-depth','final-slot','return-FP','next-formals','next-caller','next-guard','next-name','original-next-caller','observer-scope','depth-layout','slots-layout']
    if seed:cases+=['seed-plan','missing-initializer-array-seed','missing-helper-array-seed','wrong-restored-depth']
    passed=[];started=time.perf_counter()
    with tempfile.TemporaryDirectory(prefix='language-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx')as tmp:
        original,native=Path(tmp)/'original',Path(tmp)/'native';original.mkdir();native.mkdir()
        for source,destination in [(o,original),(n,native)]:
            for path in source.iterdir():
                if path.is_file()or path.name=='source-content':(destination/path.name).symlink_to(path,target_is_directory=path.is_dir())
        for case in cases:
            astate=copy.deepcopy(state);apre=copy.deepcopy(pre);aphase=copy.deepcopy(phase);atrial=copy.deepcopy(trial);atext=text;changed=[]
            def write(path,blob):
                source=path.resolve();path.unlink();path.write_bytes(blob);changed.append((path,source))
            def raw(path,offset):
                value=bytearray(path.read_bytes());value[offset]^=1;write(path,value)
            def js(name,change):
                path=native/(name+'.json');value=json.loads(path.read_text());change(value);write(path,json.dumps(value).encode())
            if case=='original-default':raw(original/'language-context-000.bin',0)
            elif case=='native-pre-depth':apre['frontend']['language_context_depth']+=1
            elif case=='native-reset-slot':aphase['frontend']['language_contexts'][9]=1
            elif case=='saved-depth':astate['frontend']['saved_language_context_depth']+=1
            elif case=='actual-entry-scalar':js('options-helper-entries',lambda v:v[0]['args'].__setitem__(0,2))
            elif case=='actual-entry-formal':js('options-helper-entries',lambda v:v[0]['formal_inventory'][0].update(type='void *'))
            elif case=='missing-return':js('options-helper-returns',lambda v:v.clear())
            elif case=='native-return-stack':js('options-helper-returns',lambda v:v[0].update(esp=v[0]['esp']+4))
            elif case=='native-return-nonvolatile':js('options-helper-returns',lambda v:v[0].update(ebx=v[0]['ebx']+4))
            elif case=='original-return-stack':atext=re.sub(r'(language-return pc=\w+ esp=)(\w+)',lambda m:m[1]+'%08x'%(int(m[2],16)+4),text)
            elif case=='original-return-nonvolatile':atext=re.sub(r'(language-return .* ebp=)(\w+)',lambda m:m[1]+'%08x'%(int(m[2],16)+4),text)
            elif case=='native-own-neighbor':js('options-helper-returns',lambda v:v[0]['guard'].update(bss='%02x'%(int(v[0]['guard']['bss'][:2],16)^1)+v[0]['guard']['bss'][2:]))
            elif case=='original-unknown-neighbor':raw(original/'language-return-bss.bin',63131)
            elif case=='original-slot-write':raw(original/'language-context-language-return.bin',4+(4 if seed else 0)*4)
            elif case=='final-depth':astate['frontend']['language_context_depth']+=1
            elif case=='final-slot':astate['frontend']['language_contexts'][4 if seed else 0]=2
            elif case=='return-FP':js('options-helper-returns',lambda v:v[0]['fp'].update(cw=0x37f))
            elif case=='next-formals':js('options-next-entries',lambda v:v[0]['formal_inventory'].append(dict(name='invented',type='int')))
            elif case=='next-caller':js('options-next-entries',lambda v:v[0].update(return_pc=v[0]['return_pc']+1))
            elif case=='next-guard':js('options-next-entries',lambda v:v[0]['guard'].update(saved='%02x'%(int(v[0]['guard']['saved'][:2],16)^1)+v[0]['guard']['saved'][2:]))
            elif case=='next-name':astate['symbol']='incorrect_stub'
            elif case=='original-next-caller':atext=re.sub(r'(trap seq=%d .*stack=)(\w+)'%int(final),lambda m:m[1]+'%08x'%(int(m[2],16)+1),text)
            elif case=='observer-scope':atrial['language_context_observers']['slots']=11
            elif case=='depth-layout':astate['frontend']['storage']['language_context_depth'][0]+=4
            elif case=='slots-layout':astate['frontend']['storage']['language_contexts'][1]=36
            elif case=='seed-plan':atrial['language_context_auxiliary']['helper_argument_unchanged']=2
            elif case=='missing-initializer-array-seed':raw(original/'language-context-006.bin',4)
            elif case=='missing-helper-array-seed':
                seq=int(next(v[0]for v in traps if v[1]=='0002b834'));raw(original/('language-context-%03d.bin'%seq),4)
            elif case=='wrong-restored-depth':aphase['frontend']['language_context_depth']=7
            else:raise RuntimeError(case)
            rows=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$',atext,re.M)
            try:
                if case in ('depth-layout','slots-layout'):d.d.field_map(json.loads((native/'state-model.json').read_text()),astate['frontend'])
                else:check_context(d.s,d.d,original,native,atrial,atext,apre,aphase,astate,seed,final,rows)
            except (ValueError,KeyError,AssertionError,struct.error)as error:passed.append(dict(case=case,rejection=str(error)))
            else:raise RuntimeError('mutant accepted: '+case)
            finally:
                for path,source in changed:path.unlink();path.symlink_to(source)
    result=dict(schema=1,kind='fresh affected semantic denials after complete raw/strict positive; raw receipt files never modified',original=str(o),native=str(n),bindings=record,seed=seed,count=len(passed),cases=passed,seconds=time.perf_counter()-started,checker_sha256=d.s.sha(Path(__file__)),semantic_checker_sha256=d.s.sha(Path(__file__).with_name('language_context_state.py')))
    args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(count=len(passed),seconds=result['seconds'])))


if __name__=='__main__':main()
