import json,copy,sys,re,hashlib
from pathlib import Path
sys.path.insert(0,'scripts');from parser_context_owner import validate,STACK_BASE
p=Path('cases/argument-observations/state_machine_debug/1108-resolve_member_pointer_class-8')
proof=json.loads((p/'validation.json').read_text());receipt=json.loads((p/'original-receipt.json').read_text());native=json.loads((p/'native-checkpoint.json').read_text());events=(p/'original-events.txt').read_text();digest=hashlib.sha256((p/'original-events.txt').read_bytes()).hexdigest();config=json.loads((p/'original-configuration.json').read_text());words={int(i):int(v,16) for i,v in re.findall(r'^argument (\d+) ([0-9a-f]+)$',events,re.M)};formal=proof['formals'][1];anchors={r['name']:r for r in receipt['diagnostic_observations']}
validate(proof['function'],formal,anchors,native,words,receipt,events,config,digest)
results=[]
for name in ('missing-input-anchor','wrong-slot','short-read','unreadable-word','wrong-input-owner','original-right-outside-stack','native-right-outside-stack','wrong-native-owner','missing-output-anchor','wrong-output-local','invented-output-prestate'):
 a=copy.deepcopy(anchors);n=copy.deepcopy(native);r=copy.deepcopy(receipt);raw=events;d=digest
 if name=='missing-input-anchor':r['ownership_register_observations']=[x for x in r['ownership_register_observations'] if x['register']!='esi']
 elif name=='wrong-slot':a['parser_context']['original']+=8
 elif name=='short-read':a['parser_context']['read_bytes']=2
 elif name=='unreadable-word':a['parser_context']['read_succeeded']=False
 elif name=='wrong-input-owner':a['parser_context']['value']+=4
 elif name=='original-right-outside-stack':
  for x in r['ownership_register_observations']:
   if x['register']=='esi':x['value']=STACK_BASE-8
  raw=re.sub(r'^ownership-anchor esi [0-9a-f]{8}$','ownership-anchor esi %08x'%(STACK_BASE-8),raw,flags=re.M);d=hashlib.sha256(raw.encode()).hexdigest();r['ownership_entry_projection']['events_sha256']=d
 elif name=='native-right-outside-stack':n['caller_locals']['caller_right']['value']=n['caller_locals']['caller_name']['stack_base']-8
 elif name=='wrong-native-owner':n['caller_locals']['caller_name']['local']='other'
 elif name=='missing-output-anchor':r['ownership_register_observations']=[x for x in r['ownership_register_observations'] if x['register']!='eax']
 elif name=='wrong-output-local':n['caller_locals']['caller_result']['address']+=4
 elif name=='invented-output-prestate':n['caller_locals']['caller_result']['value']=0
 try:validate(proof['function'],formal,a,n,words,r,raw,config,d)
 except (AssertionError,KeyError) as e:results.append(dict(test=name,result='held',reason=str(e)))
 else:raise AssertionError('negative passed '+name)
out=Path('evidence/parser-member-pointer-owner');out.mkdir(exist_ok=True);(out/'negatives.json').write_text(json.dumps(dict(control='pass',proof_sha256=hashlib.sha256((p/'validation.json').read_bytes()).hexdigest(),negatives=results),indent=2)+'\n')
(out/'analysis.txt').write_text('The81B whole helper ignores output prestate and consumes qualified name pointer from accepted parse_translation_unit right[-1].context. Raw2792a MOV EAX,[ESI-8]/PUSH2792d, thenLEAoutput EAX2792e/PUSH27932/CALL27933 return27938. Original parser_stacks94e44 plus440B state prefix gives semanticbase94ffc; this proof is bounded to normal allocated220-entry prefix, excluding existing boundary-cell aliases. Native actualglobal parser_stacks.data.semantic.values has the same bounds/layout. Original diagnostic read is statically restricted to this allocated stack region before capture; freshESI bounds/alignment/slot-8/readsuccess bind it to actualarg1. Native indexedread is boundschecked before dereference; only current right[-1].context, not an array dump. EAX outputidentity is actual native caller member_class address with valueNone, not initialization or comparison of unwritten bytes. Eleven focused metadata/ownership/read/bounds negatives hold; no raw accepted records modified.\n')
print('eleven parser ownership negatives held')
