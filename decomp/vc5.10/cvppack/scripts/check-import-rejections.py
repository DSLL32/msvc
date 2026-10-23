#!/usr/bin/env python3
"""Current import/report and reused-original sidecar guards; Linux only."""
import argparse
import json
from pathlib import Path
import shutil
import tempfile
import time
from accept import validate_pair
from state import BINARY, sha


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original',type=Path,required=True)
    parser.add_argument('--native',type=Path,required=True)
    parser.add_argument('--audit',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    config=args.original/'configuration.json'
    start=time.perf_counter()
    baseline=validate_pair(config,args.original,args.native,args.audit)
    tests=[]
    reasons={
        'stale-native-sidecar':'stale/swapped reused-capture sidecar',
        'swapped-original-sidecar':'stale/swapped reused-capture sidecar',
        'reuse-flag-alone-does-not-authorize-refresh':'stale/swapped reused-capture sidecar',
        'stale-historical-audit':'stale/swapped reused-capture sidecar',
        'stale-historical-metadata':'stale/swapped reused-capture sidecar',
        'omitted-qualified-import-check':'refresh omitted qualified import checks',
        'forged-current-metadata-with-refreshed-listed-hashes':'forged current metadata',
        'forged-current-coff-proof-with-refreshed-listed-hash':'forged current data proof',
        'aggregate-graph-pass-cannot-hide-four-unknown-free-sites':'main cached free site inventory differs',
        'gpa-callback-must-not-be-guessed-as-free':'main cached free site inventory differs',
        'extra-rebuilt-opaque-call':'unexpected rebuilt opaque callback',
        'stale-common-report-input':'stale common import report PE inputs',
    }
    def reject(name,change):
        with tempfile.TemporaryDirectory(prefix='cvpack-import-control-',dir=Path.home()/'.ai-tmp') as temporary:
            directory=Path(temporary)
            shutil.copytree(args.audit.parent,directory/'audit')
            path=directory/'audit'/args.audit.name
            audit=json.loads(path.read_text())
            change(audit,path.parent)
            path.write_text(json.dumps(audit))
            try:
                validate_pair(config,args.original,args.native,path)
            except (AssertionError,KeyError,ValueError) as error:
                assert str(error)==reasons[name], ('wrong rejection guard',name,str(error))
                tests.append(dict(name=name,rejected=True,diagnostic=str(error)))
            else:
                raise AssertionError('mutation accepted: '+name)
    reject('stale-native-sidecar',lambda a,d:a['reused_capture_binding'].update(native_receipt_sha256='0'*64))
    reject('swapped-original-sidecar',lambda a,d:a['reused_capture_binding'].update(original_receipt_sha256='0'*64))
    reject('reuse-flag-alone-does-not-authorize-refresh',lambda a,d:a.pop('reused_capture_binding'))
    reject('stale-historical-audit',lambda a,d:a['reused_capture_binding'].update(historical_audit_sha256='0'*64))
    reject('stale-historical-metadata',lambda a,d:a['reused_capture_binding'].update(historical_metadata_sha256='0'*64))
    reject('omitted-qualified-import-check',lambda a,d:a['graph_command'].remove('--check-imports'))
    def metadata(a,d):
        path=d/'graph-metadata.json';data=json.loads(path.read_text())
        data['functions']['_pack_module']['rebuilt']['text_data'][0][1]+=1
        path.write_text(json.dumps(data))
        a['graph_metadata_sha256']=sha(path)
        a['graph']['register_import_inputs']['metadata_sha256']=sha(path)
    reject('forged-current-metadata-with-refreshed-listed-hashes',metadata)
    def proof(a,d):
        path=d/'graph-metadata-proof.json';data=json.loads(path.read_text())
        data['tables'][0]['rebuilt_targets'][0]+=4
        path.write_text(json.dumps(data));a['graph_metadata_proof_sha256']=sha(path)
    reject('forged-current-coff-proof-with-refreshed-listed-hash',proof)
    def missing_free(a,d):
        original=a['graph']['functions']['_main']['register_imports']['original']
        sites=sorted(map(int,original['resolved_register_calls']))[1:]
        for site in sites:original['resolved_register_calls'].pop(str(site))
        original['unresolved_register_calls']=sorted(original['unresolved_register_calls']+sites)
        # Updating the repeated summary still cannot make the report satisfy the
        # independently captured exact five-site inventory.
        a['cached_import_sites']['original_free_sites']=sorted(map(int,original['resolved_register_calls']))
    reject('aggregate-graph-pass-cannot-hide-four-unknown-free-sites',missing_free)
    def guessed_callback(a,d):
        original=a['graph']['functions']['_main']['register_imports']['original']
        site=original['unresolved_register_calls'].pop()
        original['resolved_register_calls'][str(site)]={'dll':'msvcrt.dll','name':'free'}
    reject('gpa-callback-must-not-be-guessed-as-free',guessed_callback)
    reject('extra-rebuilt-opaque-call',lambda a,d:a['graph']['functions']['_main']['register_imports']['rebuilt']['other_unresolved_calls'].append(0x403000))
    reject('stale-common-report-input',lambda a,d:a['graph']['register_import_inputs'].update(rebuilt_sha256='0'*64))
    report=dict(schema=1,native_only=True,original_receipt_sha256=sha(args.original/'receipt.json'),
        native_receipt_sha256=sha(args.native/'receipt.json'),audit_sha256=sha(args.audit),
        checker_sha256=sha(Path(__file__)),acceptance_checker_sha256=sha(BINARY/'scripts/accept.py'),
        baseline=baseline,count=len(tests),tests=tests,seconds=time.perf_counter()-start)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(rejected=len(tests),seconds=report['seconds']),indent=2))


if __name__=='__main__':main()
