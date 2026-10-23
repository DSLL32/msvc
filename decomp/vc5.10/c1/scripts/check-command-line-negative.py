#!/usr/bin/env python3
"""Affected option storage, table identities, reset and real helper controls."""
import argparse,copy,importlib.util,json,re,time
from pathlib import Path
import command_line_state as command

def main():
 p=argparse.ArgumentParser();p.add_argument('original',type=Path);p.add_argument('native',type=Path);a=p.parse_args();started=time.perf_counter()
 spec=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));trial=importlib.util.module_from_spec(spec);spec.loader.exec_module(trial)
 trial.check(a.original,a.native)
 project=Path(__file__).resolve().parents[1]
 state=json.loads((a.native/'state.json').read_text());candidate=json.loads((a.original/'candidate.json').read_text());fields=json.loads((project/candidate['field_map']).read_text())
 events=(a.original/'events.txt').read_text();module=int(re.findall(r'^module name=c1n32_l.dll actual=([0-9a-f]+)',events,re.M)[0],16);sequence=int(re.findall(r'^trap seq=(\d+)',events,re.M)[-1]);probe=json.loads((a.original/'receipt.json').read_text())['probe']
 entry,at=command.boundary(a.original,a.native,state,sequence,module)
 labels=['lost-alias-root','warning-array-tail','database-path-tail','pch-path-tail','warning-bound','packing-signedness','pointer-role','missing-option','snapshot-loss','bss-vector-not-cleared','bss-source-not-cleared','persistent-pdb-cleared','layout-size','layout-offset','field-alias','field-width','wrong-default','missing-rule','rule-pattern','rule-action','rule-result','rule-target','z-rule-target','co-mutated-target-address','helper-formal','helper-caller','helper-caller-argument','helper-state-mutation','helper-error','helper-fpu-status','helper-fpu-tag']
 for label in labels:
  value=copy.deepcopy(state);mapping=copy.deepcopy(fields);options=value['compiler_state']['option_state']
  if label=='lost-alias-root':options['initialized']['include_aliases']=1
  elif label=='warning-array-tail':options['initialized']['warning_levels'][-1]^=1
  elif label=='database-path-tail':options['initialized']['database_path'][-1]^=1
  elif label=='pch-path-tail':options['initialized']['pch_link_path'][-1]^=1
  elif label=='warning-bound':options['initialized']['lowest_warning']+=1
  elif label=='packing-signedness':options['initialized']['packing_alignment']^=0x80000000
  elif label=='pointer-role':options['initialized']['database_file']+=4
  elif label=='missing-option':options['initialized'].pop('language_extensions')
  elif label=='snapshot-loss':options['snapshot']['move_pch']+=1
  elif label=='bss-vector-not-cleared':options['bss']['defines']=1
  elif label=='bss-source-not-cleared':options['bss']['source_file']=1
  elif label=='persistent-pdb-cleared':options['current_pdb']=0 if options['current_pdb']else 1
  elif label=='layout-size':options['layout']['initialized_size']+=4
  elif label=='layout-offset':options['layout']['initialized']['packing_stack']+=4
  elif label in ('field-alias','field-width','wrong-default'):
   row=next(row for row in mapping['modeled_initialized']if row['name']=='options.packing_stack')
   row['native_offset'if label=='field-alias'else'bytes'if label=='field-width'else'default']+=4
  elif label=='missing-rule':options['tables']['rules'].pop()
  elif label=='rule-pattern':options['tables']['rules'][1]['pattern']='-notnologo'
  elif label=='rule-action':options['tables']['rules'][1]['action']=5
  elif label=='rule-result':options['tables']['rules'][1]['result']=0
  elif label=='rule-target':options['tables']['rules'][1]['target']+=4
  elif label=='z-rule-target':options['tables']['z_rules'][0][2]+=4
  elif label=='co-mutated-target-address':
   old=options['tables']['targets'][str(0x550ec)];options['tables']['targets'][str(0x550ec)]=old+4
   options['tables']['rules'][1]['target']=old+4
  elif label=='helper-formal':value['formal_inventory'].append(dict(name='invented',type='int'))
  elif label=='helper-caller':value['caller']['symbol']='c1_main'
  elif label=='helper-caller-argument':value['caller']['args']['argc']+=1
  elif label=='helper-state-mutation':value['compiler_state']['initialized']['first_call']=1
  elif label=='helper-error':value['win32_last_error']+=1
  elif label=='helper-fpu-status':value['command_line_fp']['sw']^=0x20
  else:value['command_line_fp']['tag']^=3
  try:
   command.validate_projection(a.original,a.native,mapping,value,sequence,module,candidate,probe)
   command.validate_parser(a.original,a.native,entry,value,at,sequence,module,candidate)
  except(AssertionError,KeyError,ValueError,IndexError):continue
  raise AssertionError('accepted '+label)
 print(json.dumps(dict(status='PASS',fresh_target_controls=len(labels),rejected=labels,seconds=time.perf_counter()-started,scope='Fresh complete positive plus affected parser field/default/table/reset/ABI/x87 controls; unchanged transport campaigns reused.'),indent=2))
if __name__=='__main__':main()
