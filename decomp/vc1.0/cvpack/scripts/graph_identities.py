"""Explicit strict relocation identity preparation and fail-closed cache loading."""
import argparse,json,hashlib
from pathlib import Path
from state import BINARY,ROOT,load,sha
from runtime import identify

def provenance(rebuilt):
    config,_=load()
    return dict(original_sha256=sha(ROOT/config['target']['path']),rebuilt_sha256=sha(rebuilt),
        archive_sha256=sha(ROOT/'toolchains/1.0-nt/lib/LIBC.LIB'),functions_sha256=sha(BINARY/'functions.json'),
        map_sha256=sha(BINARY/'build/cvpack.map'),runtime_anchors_sha256=sha(BINARY/'runtime-anchors.json'),
        runtime_index_sha256=sha(BINARY/'runtime.json'),
        identifier_sources={name:sha(BINARY/'scripts'/name)for name in ('runtime.py','binary.py','state.py','audit.py','graph_identities.py')})

def payload_sha(record):
    payload={key:record[key]for key in ('provenance','original_runtime','rebuilt_runtime')}
    return hashlib.sha256(json.dumps(payload,sort_keys=True,separators=(',',':')).encode()).hexdigest()

def prepare(output,rebuilt):
    from audit import publics
    config,_=load();before=provenance(rebuilt)
    original=identify(ROOT/config['target']['path'],{'_main':config['target']['application_main']})
    assert original==json.loads((BINARY/'runtime.json').read_text()),'strict original runtime identities changed'
    canonical=identify(rebuilt,{'_main':publics()['_main']})
    assert provenance(rebuilt)==before,'identity inputs changed during preparation'
    record=dict(schema=1,provenance=before,original_runtime=original,rebuilt_runtime=canonical)
    record['identity_payload_sha256']=payload_sha(record)
    output.write_text(json.dumps(record,indent=2)+'\n')

def read(path,rebuilt):
    record=json.loads(path.read_text())
    assert record['identity_payload_sha256']==payload_sha(record),'changed strict identity payload'
    assert record['schema']==1 and record['provenance']==provenance(rebuilt),'stale strict graph identity cache; explicitly run make graph-identities'
    assert record['original_runtime']==json.loads((BINARY/'runtime.json').read_text()),'original identity cache differs'
    return record['original_runtime'],record['rebuilt_runtime']

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepare',action='store_true',required=True)
    parser.add_argument('--output',type=Path,default=BINARY/'build/graph-identities.json')
    parser.add_argument('--rebuilt',type=Path,default=BINARY/'build/CVPACK.EXE')
    args=parser.parse_args();prepare(args.output,args.rebuilt)
    print('Prepared hash-bound strict original/compiler relocation identities')
if __name__=='__main__':main()
