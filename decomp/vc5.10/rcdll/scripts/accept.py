#!/usr/bin/env python3
"""Promote exactly a reviewed trial with fresh raw-derived state and CFG proof."""
import argparse
import json
from pathlib import Path
from protocol import HERE,entry_trial_pair,crt_init_pair,dll_main_pair,rc_entry_pair,rc_main_pair,rc_initialize_pair,rc_allocate_pair,rc_is_option_pair,rc_emit_pair,rc_message_pair,rc_format_pair,sha,validation_versions
from audit import run
p=argparse.ArgumentParser();p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--check-only',action='store_true')
a=p.parse_args()
config=json.loads((HERE/'functions.json').read_text())
contracts={'recon_dll_entry':entry_trial_pair,'rcdll_crt_init':crt_init_pair,'rcdll_main':dll_main_pair,'RC':rc_entry_pair,'rc_main':rc_main_pair,'rc_initialize':rc_initialize_pair,'rc_allocate':rc_allocate_pair,'rc_is_option':rc_is_option_pair,'rc_emit':rc_emit_pair,'rc_message':rc_message_pair,'rc_format':rc_format_pair}
assert config.get('trial_function') in contracts,'no implemented typed contract for this trial'
proof=contracts[config['trial_function']](a.original,a.native)
report=run()
assert report['status']==0 and report['functions']==config['accepted']+[config['trial_function']]
proof['graph_audit_sha256']=sha(HERE/'build/graph-audit-receipt.json')
proof['validation_versions']=validation_versions()
proof['original_receipt_sha256']=sha(a.original/'receipt.json');proof['native_receipt_sha256']=sha(a.native/'receipt.json')
print(json.dumps(proof,indent=2))
if not a.check_only:
    review=json.loads((HERE/'cases/help/review.json').read_text())
    assert config['trial_function'] in review['trial_promotions'] or review.get('autonomous_scope')=='rc-export-help','candidate promotion is held outside the approved first help case'
    history=HERE/('cases/help/acceptance-%04d.json'%(len(config['accepted'])+1))
    assert not history.exists()
    history.write_text(json.dumps(dict(configuration=config,proof=proof,review=review,original=str(a.original),native=str(a.native)),indent=2)+'\n')
    config['accepted'].append(config.pop('trial_function'))
    config['status']='help case in progress'
    (HERE/'functions.json').write_text(json.dumps(config,indent=2)+'\n')
