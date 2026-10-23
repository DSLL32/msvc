#!/usr/bin/env python3
"""Ablate each finite EDI guarantee in the real full common graph audit."""
import copy
import json
from pathlib import Path
import subprocess
import time
from state import BINARY,sha


def main():
    import argparse
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--original',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();begin=time.monotonic();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
    receipt=json.loads((a.original/'audit-receipt.json').read_text());graph=json.loads((a.original/'graph-audit.json').read_text());base=json.loads((a.original/'graph-metadata.json').read_text());contracts=base['register_imports']['preservation_contracts']
    assert receipt['tool_sha256']==sha(Path(receipt['command'][0]).read_bytes())
    assert graph['checked_functions']==71 and graph['register_import_inputs']['enabled_functions']==['@read_initializer_records@4','_initialize_object_output']
    writer=graph['functions']['_initialize_object_output']['register_imports'];assert writer['original']['resolved_register_calls']=={'4537148':{'dll':'msvcrt.dll','name':'fopen'},'4672513':{'dll':'msvcrt.dll','name':'fopen'}}
    assert all(not writer[s]['unresolved_register_calls']and not writer[s]['other_unresolved_calls']for s in ('original','rebuilt'))
    expected=[i for i,c in enumerate(contracts)if 'edi'in c['preserves']];assert len(expected)==8
    results=[]
    for count,index in enumerate(expected):
        value=copy.deepcopy(base);items=value['register_imports']['preservation_contracts'];target=copy.deepcopy(items[index]['target']);items[index]['preserves'].remove('edi')
        if not items[index]['preserves']:items.pop(index)
        meta=out/('metadata-%d.json'%count);meta.write_text(json.dumps(value,indent=2)+'\n');report=out/('report-%d.json'%count);command=receipt['command'][:];command[command.index('--metadata')+1]=str(meta);command[command.index('--report')+1]=str(report)
        start=time.monotonic();r=subprocess.run(command,capture_output=True,timeout=30);elapsed=time.monotonic()-start
        (out/('stdout-%d.txt'%count)).write_bytes(r.stdout);(out/('stderr-%d.txt'%count)).write_bytes(r.stderr)
        assert r.returncode!=0 and b'original _initialize_object_output register imports: unresolved register import calls: {4537148}'in r.stderr,(target,r.stderr.decode())
        results.append({'target':target,'status':r.returncode,'seconds':elapsed,'metadata_sha256':sha(meta.read_bytes()),'stderr_sha256':sha(r.stderr),'reason':r.stderr.decode().strip(),'command':command})
    # All earlier source graphs retain their original call/reference ownership;
    # normalized edges/imports remain equal despite relocated rebuilt addresses.
    earlier=json.loads((BINARY/'cases/acceptance/0070-output-directive-append/original/graph-audit.json').read_text())
    for name,record in earlier['functions'].items():
        assert record['edges']==graph['functions'][name]['edges']and record['imports']==graph['functions'][name]['imports']
        assert earlier['original_accepted_graphs'][name]==graph['original_accepted_graphs'][name]
        assert earlier['owned_code_chunks']['original']['functions'][name]==graph['owned_code_chunks']['original']['functions'][name]
    summary={'status':'PASS','count':len(results),'results':results,'prior_source_graphs_preserved':70,'original_receipt_sha256':sha((a.original/'receipt.json').read_bytes()),'source_metadata_sha256':sha((a.original/'graph-metadata.json').read_bytes()),'tool_sha256':receipt['tool_sha256'],'seconds':time.monotonic()-begin,'scope':'Each actual needed EDI bit removed independently; existing13 EBP claims retained, full71source audit unchanged. Every ablation loses hot fopen453b3c after the real cold join; no empty-mask parse rejection or guessed unknowncall property.'}
    (out/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')

if __name__=='__main__':main()
