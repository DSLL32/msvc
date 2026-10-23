#!/usr/bin/env python3
"""Promote exactly a reviewed trial with fresh raw-derived state and CFG proof."""
import argparse
import json
import shutil
from pathlib import Path
from protocol import HERE,entry_trial_pair,crt_init_pair,dll_main_pair,rc_entry_pair,rc_main_pair,rc_initialize_pair,rc_allocate_pair,rc_is_option_pair,rc_emit_pair,rc_message_pair,rc_format_pair,sha,validation_versions
from audit import run
from rich_protocol import release_trial,prepare_trial,write_trial,detect_trial,unicode_trial,line_trial,prefix_trial,notify_trial,preprocess_trial,widen_trial,preprocessor_main_trial,next_argument_trial,scan_options_trial,apply_option_trial,option_number_trial,open_preprocessor_trial,character_token_trial,character_class_trial,macro_hash_trial,define_macro_trial,find_macro_trial,copy_macro_trial,open_source_trial,refill_source_trial,duplicate_source_name_trial,emit_source_line_trial,write_preprocessed_trial,quote_source_filename_trial,character_token_lookup_trial,list_macro_trial,scan_line_start_trial,handle_directive_trial,read_nonspace_trial,read_identifier_trial,character_class_lookup_trial,directive_code_trial,next_token_trial,read_number_trial,logical_character_trial,integer_value_trial,digit_value_trial
p=argparse.ArgumentParser();p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--check-only',action='store_true');p.add_argument('--case',choices=['help','resources-rich','resources-rich-trailing-space'],default='help')
a=p.parse_args()
config=json.loads((HERE/'functions.json').read_text())
contracts={'_DllMainCRTStartup':entry_trial_pair,'_CRT_INIT':crt_init_pair,'DllMain':dll_main_pair,'RC':rc_entry_pair,'main':rc_main_pair,'rc_initialize':rc_initialize_pair,'rc_allocate':rc_allocate_pair,'rc_is_option':rc_is_option_pair,'rc_emit':rc_emit_pair,'rc_message':rc_message_pair,'rc_format':rc_format_pair}
if a.case in ['resources-rich','resources-rich-trailing-space']:contracts={'rc_release':release_trial,'rc_prepare_input':prepare_trial,'rc_write_bytes':write_trial,'rc_detect_encoding':detect_trial,'rc_is_text_unicode':unicode_trial,'rc_read_wide_line':line_trial,'rc_prefix_equal':prefix_trial,'rc_notify_progress':notify_trial,'rc_preprocess':preprocess_trial,'rc_widen_arguments':widen_trial,'rc_preprocessor_main':preprocessor_main_trial,'rc_next_argument':next_argument_trial,'rc_scan_options':scan_options_trial,'rc_apply_option':apply_option_trial,'rc_option_number':option_number_trial,'rc_open_preprocessor':open_preprocessor_trial,'rc_set_character_token':character_token_trial,'rc_set_character_class':character_class_trial,'rc_hash_macro_name':macro_hash_trial,'rc_define_macro':define_macro_trial,'rc_find_macro':find_macro_trial,'rc_copy_macro_text':copy_macro_trial,'rc_open_source':open_source_trial,'rc_refill_source':refill_source_trial,'rc_duplicate_source_name':duplicate_source_name_trial,'rc_emit_source_line':emit_source_line_trial,'rc_write_preprocessed':write_preprocessed_trial,'rc_quote_source_filename':quote_source_filename_trial,'rc_character_token':character_token_lookup_trial,'rc_list_macro':list_macro_trial,'rc_scan_line_start':scan_line_start_trial,'rc_handle_directive':handle_directive_trial,'rc_read_nonspace':read_nonspace_trial,'rc_read_identifier':read_identifier_trial,'rc_character_class':character_class_lookup_trial,'rc_directive_code':directive_code_trial,'rc_next_token':next_token_trial,'rc_read_number':read_number_trial,'rc_get_logical_character':logical_character_trial,'rc_parse_integer_value':integer_value_trial,'rc_digit_value':digit_value_trial}
assert config.get('trial_function') in contracts,'no implemented typed contract for this trial'
proof=contracts[config['trial_function']](a.original,a.native)
report=run()
assert report['status']==0 and report['functions']==config['accepted']+[config['trial_function']]
proof['graph_audit_sha256']=sha(HERE/'build/graph-audit-receipt.json')
proof['validation_versions']=validation_versions()
proof['original_receipt_sha256']=sha(a.original/'receipt.json');proof['native_receipt_sha256']=sha(a.native/'receipt.json')
print(json.dumps(proof,indent=2))
if not a.check_only:
    review=json.loads((HERE/('cases/'+a.case+'/review.json')).read_text())
    assert config['trial_function'] in review['trial_promotions'] or review.get('autonomous_scope')==('rc-export-help'if a.case=='help'else a.case),'candidate promotion is held outside the approved first help case'
    history=HERE/(('cases/'+a.case+'/acceptance-%04d.json')%(len(config['accepted'])+1))
    assert not history.exists()
    audit_history=history.with_name(history.name.replace('acceptance-','audit-'))
    assert not audit_history.exists();shutil.copyfile(HERE/'build/graph-audit-receipt.json',audit_history)
    proof['graph_audit_record']=str(audit_history.relative_to(HERE))
    history.write_text(json.dumps(dict(configuration=config,proof=proof,review=review,original=str(a.original),native=str(a.native)),indent=2)+'\n')
    if config['trial_function']=='rc_prepare_input':
        for support in config.get('support_compiler',[]):
            if support.get('trial'):support['accepted']=True;support.pop('trial')
    config['accepted'].append(config.pop('trial_function'))
    config['status']=a.case+' case in progress'
    plan=json.loads((HERE/'cases/plan.json').read_text());plan['accepted_functions']=config['accepted'];(HERE/'cases/plan.json').write_text(json.dumps(plan,indent=2)+'\n')
    (HERE/'functions.json').write_text(json.dumps(config,indent=2)+'\n')
