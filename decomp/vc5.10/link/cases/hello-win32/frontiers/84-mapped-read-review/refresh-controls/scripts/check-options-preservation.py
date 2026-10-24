#!/usr/bin/env python3
"""Exercise binding failures in the reviewed ABI packet, without running Wine."""
import argparse
import copy
import importlib
import json
from pathlib import Path
import time
from binary import PE

module = importlib.import_module('options-preservation')
project = Path(__file__).resolve().parents[1]
original = PE(project.parents[2]/'toolchains/5.10/bin/LINK.EXE')
rebuilt = PE(project/'build/link.exe')
packet = json.loads(module.PACKET.read_text())
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--register-report',type=Path)
args = parser.parse_args()
started = time.perf_counter()
positive = module.check(original,packet)
mutations = {
    'wrong original': lambda p:p.update(original_sha256='0'*64),
    'wrong provider': lambda p:p['canonical_provider'].update(sha256='0'*64),
    'changed actual module receipt': lambda p:p['canonical_provider'].update(actual_loaded_receipt_sha256='0'*64),
    'wrong owned provider copy': lambda p:p['canonical_provider']['owned_hfile_copies'][0].update(sha256='0'*64),
    'wrong required import': lambda p:p['imports'][0].update(identity='msvcrt.dll!fclose'),
    'wrong provider ordinal': lambda p:p['imports'][0]['provider_export'].update(ordinal=0),
    'wrong provider entry': lambda p:p['imports'][0].update(entry_bytes='00'*16),
    'changed SDK header': lambda p:p['imports'][0]['declarations'][0].update(sha256='0'*64),
    'wrong SDK declaration': lambda p:p['imports'][0]['declarations'][0].update(declaration='int __cdecl unrelated(void);'),
    'missing preservation contract': lambda p:p['imports'].pop(),
}

rejected = []
for name, mutate in mutations.items():
    altered = copy.deepcopy(packet)
    mutate(altered)
    try:
        module.check(original,altered)
    except (AssertionError,KeyError,ValueError):
        rejected.append(name)
    else:
        raise AssertionError('accepted altered ABI packet: '+name)
resolution = None
if args.register_report:
    report = json.loads(args.register_report.read_text())
    resolution = module.check_resolution(report,rebuilt)
    for name in ['drop resolved site','wrong qualified import','unresolved call','wrong report PE','wrong report witness']:
        altered = copy.deepcopy(report)
        sites = altered['functions']['_configure_link_options@12']['register_imports']['original']
        site = next(iter(sites['resolved_register_calls']))
        if name == 'drop resolved site': sites['resolved_register_calls'].pop(site)
        elif name == 'wrong qualified import': sites['resolved_register_calls'][site]['name']='fclose'
        elif name == 'unresolved call': sites['unresolved_register_calls']=[int(site)]
        elif name == 'wrong report PE': altered['register_import_inputs']['original_sha256']='0'*64
        else: altered['register_import_inputs']['preservation_contracts'][0]['evidence']['sha256']='0'*64
        try:
            module.check_resolution(altered,rebuilt)
        except AssertionError:
            rejected.append(name)
        else:
            raise AssertionError('accepted altered common site report: '+name)
print(json.dumps(dict(positive=positive,resolution=resolution,rejected=rejected,elapsed_seconds=time.perf_counter()-started),indent=2))
