#!/usr/bin/env python3
"""Audit actual collision-helper imports and prune contracts with real ablations."""
import argparse,copy,hashlib,importlib,json,subprocess,time
from pathlib import Path
from binary import PE

BINARY=Path(__file__).resolve().parents[1];ROOT=BINARY.parents[2]


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--config',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    started=time.perf_counter();sha=lambda b:hashlib.sha256(b).hexdigest();checker=importlib.import_module('collision-preservation')
    original=PE(ROOT/'toolchains/5.10/bin/LINK.EXE');rebuilt=PE(BINARY/'build/link.exe');map_path=BINARY/'build/link.map';bound=checker.check(original,rebuilt,map_path.read_text())
    config=json.loads(args.config.read_text());config['accepted']=[checker.OWNER];config_path=args.output/'functions.json';config_path.write_text(json.dumps(config,indent=2)+'\n')
    metadata=json.loads((BINARY/'build/graph.json').read_text());metadata['functions'].setdefault(checker.OWNER,{})['register_imports']={'require_all_register_calls_resolved':True};metadata['register_imports']={'preservation_contracts':checker.contracts()};metadata_path=args.output/'metadata.json';metadata_path.write_text(json.dumps(metadata,indent=2)+'\n')
    tool=ROOT/'tooling/graph-audit/target/release/graph-audit'
    command=[str(tool),'--profile','link','--discovery','cfg','--repo',str(ROOT),'--project',str(BINARY),'--original',str(ROOT/'toolchains/5.10/bin/LINK.EXE'),'--rebuilt',str(BINARY/'build/link.exe'),'--map',str(map_path),'--config',str(config_path),'--runtime',str(BINARY/'runtime.json'),'--rebuilt-runtime',str(BINARY/'build/runtime.json'),'--metadata',str(metadata_path),'--check-ret','--check-references','--check-ownership','--check-extents','--check-callbacks','--check-imports','--resolve-register-imports']
    files=[tool,ROOT/'toolchains/5.10/bin/LINK.EXE',BINARY/'build/link.exe',map_path,args.config,BINARY/'runtime.json',BINARY/'build/runtime.json',BINARY/'build/graph.json',checker.PACKET,BINARY/'scripts/collision-preservation.py',Path(__file__),config_path,metadata_path]
    hashes={str(p):sha(p.read_bytes())for p in files}
    result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.PIPE);(args.output/'positive.json').write_bytes(result.stdout);(args.output/'positive.stderr').write_bytes(result.stderr);assert result.returncode==0,result.stderr.decode()
    inventory=checker.check_resolution(json.loads(result.stdout));ablations=[]
    def run_negative(label,alter):
        altered=copy.deepcopy(metadata);alter(altered['register_imports']['preservation_contracts']);p=args.output/(label+'.json');p.write_text(json.dumps(altered,indent=2)+'\n');cmd=command.copy();cmd[cmd.index('--metadata')+1]=str(p);result=subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE);(args.output/(label+'.stderr')).write_bytes(result.stderr)
        error=result.stderr.decode();assert result.returncode!=0 and'unresolved'in error.lower(),(label,error);ablations.append(dict(label=label,rejected=True,reason=error.strip()))
    for i,(name,bits)in enumerate(checker.MASKS):
        run_negative(name+'-contract-removed',lambda c,i=i:c.pop(i))
        for bit in bits:run_negative(name+'-'+bit+'-removed',lambda c,i=i,b=bit:c[i]['preserves'].remove(b))
    packet=json.loads(checker.PACKET.read_text());controls=[]
    def negative(label,alter):
        changed=copy.deepcopy(packet);alter(changed)
        try:checker.check(original,rebuilt,map_path.read_text(),changed)
        except AssertionError:controls.append(label)
        else:raise AssertionError('finite witness mutant accepted: '+label)
    negative('wrong actual provider export',lambda p:p['imports'][0]['provider_export'].__setitem__('rva',0))
    negative('wrong SDK declaration line',lambda p:p['imports'][1]['declarations'][0].__setitem__('line',145))
    negative('wrong ordinary register mask',lambda p:p['function_contract'].__setitem__('preserves',['edi']))
    negative('missing actual frame restoration',lambda p:p['function_contract']['original_witnesses'][1]['writes']['ebp'].pop())
    negative('wrong helper body identity',lambda p:p['function_contract']['original_witnesses'][1].__setitem__('sha256','00'*32))
    negative('wrong named trap stub return ABI',lambda p:p['function_contract']['rebuilt'].__setitem__('bytes','cc'))
    assert hashes=={str(p):sha(p.read_bytes())for p in files},'audit inputs changed'
    receipt=dict(positive=True,inventory=inventory,witness_binding=bound,contract_ablations=ablations,focused_witness_negatives=controls,negative_count=len(ablations)+len(controls),command=command,input_sha256=hashes,elapsed_seconds=time.perf_counter()-started)
    (args.output/'validation.json').write_text(json.dumps(receipt,indent=2)+'\n');print('Strict collision imports and %d contract/bit ablations plus %d witness controls pass.'%(len(ablations),len(controls)))


if __name__=='__main__':main()
