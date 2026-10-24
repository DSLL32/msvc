#!/usr/bin/env python3
"""Affected grammar/projection/restore/first-helper denials; Linux-only."""
import argparse,copy,importlib.util,json,os,re,struct,tempfile,time
from pathlib import Path
from options_state import check as check_options


def module(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


def main():
    parser=argparse.ArgumentParser();parser.add_argument('original',type=Path);parser.add_argument('native',type=Path);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    d=module('options_decoder','check-callback.py');history=module('options_history','check-history.py');o,n=args.original,args.native
    record={side+'_receipt_sha256':d.s.sha(path/'receipt.json')for side,path in [('original',o),('native',n)]};audit=history.configure(d,o,n,saved_record=record);d.check(o,n,audit_path=audit)
    trial=json.loads((o/'trial.json').read_text());text=(o/'events.txt').read_text();state=json.loads((n/'state.json').read_text());pre=json.loads((n/'pre.json').read_text());phase=json.loads((n/'phase.json').read_text());traps=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$',text,re.M);final='%03d'%int(traps[-1][0]);seed=trial.get('test_seed')
    cases=['old-rule-literal','native-rule-alias','native-rule-action','grammar-pattern','grammar-terminator','letter-destination','warning-value','grammar-stride','grammar-field-offset','default-initializer','literal-identity','initial-array','final-scalar','saved-scalar','native-BSS','field-inventory','layout-alias','persistent-context','helper-argument','helper-formals','helper-PC','helper-caller','helper-neighbor','helper-error','original-helper-argument','original-before-helper-neighbor']
    if seed:cases+=['restore-input','saved-array','BSS-seed-omitted','persistent-predefault','seed-role']
    if (n/'options-abi-refinement.json').exists():cases+=['refinement-mask','refinement-code','refinement-MAP','refinement-receipt']
    passed=[];started=time.perf_counter()
    with tempfile.TemporaryDirectory(prefix='options-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx')as tmp:
        original,native=Path(tmp)/'original',Path(tmp)/'native';original.mkdir();native.mkdir()
        # Top-level immutable files are referenced, not duplicated. Mutations
        # replace only the temporary symlink; original evidence is never open
        # for writing and raw receipts are deliberately not forged.
        for source,destination in [(o,original),(n,native)]:
            for path in source.iterdir():
                if path.is_file() or path.name=='source-content':(destination/path.name).symlink_to(path,target_is_directory=path.is_dir())
        for case in cases:
            altered_state=copy.deepcopy(state);altered_pre=copy.deepcopy(pre);altered_phase=copy.deepcopy(phase);altered_text=text;changed=[]
            def write(path,raw):
                source=path.resolve();path.unlink();path.write_bytes(raw);changed.append((path,source))
            def raw(path,offset):
                data=bytearray(path.read_bytes());data[offset]^=1;write(path,data)
            def js(name,change):
                path=native/(name+'.json');value=json.loads(path.read_text());change(value);write(path,json.dumps(value).encode())
            if case=='old-rule-literal':raw(original/'options-rules-loaded.bin',0)
            elif case=='native-rule-alias':raw(native/'option_rules-native.bin',4)
            elif case=='native-rule-action':raw(native/'option_rules-native.bin',9)
            elif case=='grammar-pattern':js('options-native-patterns',lambda v:v.__setitem__(0,'wrong'))
            elif case=='grammar-terminator':raw(native/'option_rules-native.bin',1211)
            elif case=='letter-destination':raw(native/'letter_options-native.bin',8)
            elif case=='warning-value':raw(native/'warning_defaults-native.bin',4)
            elif case=='grammar-stride':js('options-grammar-layout',lambda v:v['option_rules'].update(record_size=16))
            elif case=='grammar-field-offset':js('options-grammar-layout',lambda v:v['option_rules']['fields']['action'].__setitem__(0,10))
            elif case=='default-initializer':raw(original/'frontend-data-000.bin',0xcd570-0xcc000)
            elif case=='literal-identity':js('options-native-literals',lambda v:v['database_file'].update(address=v['database_file']['address']+1))
            elif case=='initial-array':altered_pre['frontend']['options_data']['warning_levels'][999]^=1
            elif case=='final-scalar':altered_state['frontend']['options_data']['warning_level']+=1
            elif case=='saved-scalar':altered_phase['frontend']['saved_options_data']['warning_level']+=1
            elif case=='native-BSS':altered_phase['frontend']['options_bss']['option_zn']=1
            elif case=='field-inventory':altered_state['frontend']['options_data'].pop('warning_level')
            elif case=='layout-alias':altered_state['frontend']['storage']['options.warning_level'][0]+=4
            elif case=='persistent-context':altered_state['frontend']['current_pdb_context']^=4
            elif case=='helper-argument':js('options-helper-entries',lambda v:v[0]['args'].__setitem__(0,4))
            elif case=='helper-formals':js('options-helper-entries',lambda v:v[0]['formal_inventory'].clear())
            elif case=='helper-PC':js('options-helper-entries',lambda v:v[0].update(pc=v[0]['pc']+1))
            elif case=='helper-caller':js('options-helper-entries',lambda v:v[0].update(return_pc=v[0]['return_pc']+1))
            elif case=='helper-neighbor':js('options-helper-entries',lambda v:v[0]['guard'].update(data='ff'+v[0]['guard']['data'][2:]))
            elif case=='helper-error':js('options-helper-entries',lambda v:v[0].update(last_error=0))
            elif case=='original-helper-argument':
                altered_text=re.sub(r'(rva=0002b834 .*stack=[0-9a-f]+,)00000000',r'\g<1>00000004',text);assert altered_text!=text
            elif case=='original-before-helper-neighbor':raw(original/('pool-bss-%03d.bin'%(int(final)+1)),63131)
            elif case=='restore-input':altered_pre['frontend']['options_data']['packing_alignment']=0
            elif case=='saved-array':altered_pre['frontend']['saved_options_data']['database_path'][255]=0
            elif case=='BSS-seed-omitted':altered_pre['frontend']['options_bss']['show_includes']=0
            elif case=='persistent-predefault':js('options-before-seed',lambda v:v.update(current_pdb_context=4))
            elif case=='seed-role':altered_text=text.replace('options-seed fields=106','options-seed fields=105')
            elif case=='refinement-mask':js('trial',lambda v:v['ranges'][0].update(start_rva=v['ranges'][0]['start_rva']+1))
            elif case=='refinement-code':
                raw(native/'c1n32_l.dll',1024);js('trial',lambda v:v['bound_inputs'].update({'build/c1n32_l.dll':d.s.sha(native/'c1n32_l.dll')}))
            elif case=='refinement-MAP':
                path=native/'c1xx.map';write(path,path.read_bytes().replace(b'Preferred load address',b'Changed__ load address',1));js('trial',lambda v:v['bound_inputs'].update({'build/c1xx.map':d.s.sha(path)}))
            elif case=='refinement-receipt':js('options-abi-refinement',lambda v:v.update(actual_scalar_argument=4))
            else:raise RuntimeError(case)
            altered_traps=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$',altered_text,re.M)
            try:check_options(d.s,d.d,original,native,trial,altered_text,altered_pre,altered_phase,altered_state,seed,final,altered_traps)
            except (ValueError,KeyError,AssertionError,struct.error)as error:passed.append(dict(case=case,rejection=str(error)))
            else:raise RuntimeError('mutant accepted: '+case)
            finally:
                for path,source in changed:path.unlink();path.symlink_to(source)
    result=dict(schema=1,kind='isolated affected semantic denials after genuine complete raw/strict positive; no receipt mutation',original=str(o),native=str(n),bindings=record,seed=seed,cases=passed,count=len(passed),seconds=time.perf_counter()-started,checker_sha256=d.s.sha(Path(__file__)),semantic_checker_sha256=d.s.sha(Path(__file__).with_name('options_state.py')))
    args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(count=len(passed),seconds=result['seconds'])))


if __name__=='__main__':main()
