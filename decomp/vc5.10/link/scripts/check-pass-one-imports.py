#!/usr/bin/env python3
"""Strict real PassOne import audit, two contract ablations and scoped controls."""
import argparse
import copy
import hashlib
import importlib
import json
from pathlib import Path
import subprocess
import time
from binary import PE

BINARY=Path(__file__).resolve().parents[1]
ROOT=BINARY.parents[2]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    started=time.perf_counter();sha=lambda b:hashlib.sha256(b).hexdigest()
    witness=importlib.import_module('pass-one-preservation')
    original=PE(ROOT/'toolchains/5.10/bin/LINK.EXE');rebuilt=PE(BINARY/'build/link.exe')
    map_path=BINARY/'build/link.map';map_text=map_path.read_text()
    bound=witness.check(original,rebuilt,map_text)
    configuration=json.loads(args.config.read_text());configuration['accepted']=[witness.OWNER]
    config_path=args.output/'functions.json';config_path.write_text(json.dumps(configuration,indent=2)+'\n')
    metadata=json.loads((BINARY/'build/graph.json').read_text())
    metadata['functions'].setdefault(witness.OWNER,{})['register_imports']={'require_all_register_calls_resolved':True}
    evidence=dict(path='cases/hello-win32/pass-one-import-preservation.json',sha256=sha(witness.PACKET.read_bytes()))
    metadata['register_imports']={'preservation_contracts':[
        dict(target=dict(function='_link_message'),preserves=['esi'],evidence=evidence),
        dict(target={'import':dict(dll='msvcrt.dll',name='fputc')},preserves=['esi'],evidence=evidence)]}
    metadata_path=args.output/'metadata.json';metadata_path.write_text(json.dumps(metadata,indent=2)+'\n')
    tool=ROOT/'tooling/graph-audit/target/release/graph-audit'
    command=[str(tool),'--profile','link','--discovery','cfg','--repo',str(ROOT),'--project',str(BINARY),
        '--original',str(ROOT/'toolchains/5.10/bin/LINK.EXE'),'--rebuilt',str(BINARY/'build/link.exe'),
        '--map',str(map_path),'--config',str(config_path),'--runtime',str(BINARY/'runtime.json'),
        '--rebuilt-runtime',str(BINARY/'build/runtime.json'),'--metadata',str(metadata_path),
        '--check-ret','--check-references','--check-ownership','--check-extents','--check-callbacks',
        '--check-imports','--resolve-register-imports']
    files=[tool,ROOT/'toolchains/5.10/bin/LINK.EXE',BINARY/'build/link.exe',map_path,args.config,
        BINARY/'runtime.json',BINARY/'build/runtime.json',BINARY/'build/graph.json',witness.PACKET,
        BINARY/'scripts/pass-one-preservation.py',Path(__file__),config_path,metadata_path]
    hashes={str(p):sha(p.read_bytes())for p in files}
    result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    (args.output/'positive.json').write_bytes(result.stdout);(args.output/'positive.stderr').write_bytes(result.stderr)
    assert result.returncode==0,result.stderr.decode()
    inventory=witness.check_resolution(json.loads(result.stdout))
    ablations=[]
    for index,label in enumerate(['message-function','fputc-provider']):
        altered=copy.deepcopy(metadata);altered['register_imports']['preservation_contracts'].pop(index)
        path=args.output/(label+'-removed.json');path.write_text(json.dumps(altered,indent=2)+'\n')
        altered_command=command.copy();altered_command[altered_command.index('--metadata')+1]=str(path)
        negative=subprocess.run(altered_command,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        (args.output/(label+'-removed.stderr')).write_bytes(negative.stderr)
        error=negative.stderr.decode()
        assert negative.returncode!=0 and'unresolved'in error.lower(),error
        assert '44bcab'in error.lower()or str(0x44bcab)in error,error
        ablations.append(dict(contract=label,rejected=True,reason=error.strip()))
    packet=json.loads(witness.PACKET.read_text());controls=[]
    def negative(name,change):
        altered=copy.deepcopy(packet);change(altered)
        try:witness.check(original,rebuilt,map_text,altered)
        except AssertionError:controls.append(name)
        else:raise AssertionError('ABI witness control accepted: '+name)
    negative('wrong actual fputc provider entry',lambda p:p['imports'][0].__setitem__('entry_bytes','00'*16))
    negative('wrong shipped declaration line',lambda p:p['imports'][0]['declarations'][0].__setitem__('line',344))
    negative('unreviewed ordinary register mask',lambda p:p['function_contract'].__setitem__('preserves',['edi']))
    negative('missing suppression-filter restore',lambda p:p['function_contract']['original_witnesses'][2]['esi_writes'].pop())
    negative('wrong finite diagnostic body hash',lambda p:p['function_contract']['original_witnesses'][1].__setitem__('sha256','00'*32))
    negative('truncated actual named-stub ABI',lambda p:p['function_contract']['rebuilt'].__setitem__('bytes',p['function_contract']['rebuilt']['bytes'][:-2]))
    assert hashes=={str(p):sha(p.read_bytes())for p in files},'import audit inputs changed'
    receipt=dict(positive=True,inventory=inventory,witness_binding=bound,contract_ablations=ablations,
        focused_witness_negatives=controls,negative_count=len(ablations)+len(controls),command=command,
        input_sha256=hashes,elapsed_seconds=time.perf_counter()-started,
        qualification='Common tool discovers every CFG/call/import/RET. Controls bind exactly two necessary ESI contracts; no original/native execution or new library semantics.')
    (args.output/'validation.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print('PassOne strict imports pass; two real contract ablations and six finite-witness controls reject.')


if __name__=='__main__':main()
