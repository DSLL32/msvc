from pathlib import Path
import json,hashlib,shutil,sys
B=Path('/root/msvc2/decomp/vc1.0/c1');S=Path('/root/.ai-tmp/msvc2/vc1.0/c1');C=B/'cases/asm-direct-callbacks';m=json.loads((C/'manifest.json').read_text());rows=json.loads((S/'asm-direct-matrix-v2.json').read_text());cfg=json.loads((B/'functions.json').read_text());assert len(rows)==15 and len(cfg['accepted'])==997
sys.path.insert(0,str(B/'scripts'));from images import imports;from binary import PE
original=PE(B.parents[2]/m['phases']['c1']['target']);drivers=cfg['driver'];addresses={r['start']for r in drivers};records=[r for r in json.loads((B/'runtime.json').read_text())if r['start']not in addresses]+imports(original)+drivers;by={r['name']:r for r in drivers};out=[]
for i,row in enumerate(rows):
 d=Path(row['original']);record=by[row['input']['function']];expected=bytearray(original.data)
 for section in original.sections:
  if section['flags']&0x20:expected[section['off']:section['off']+section['size']]=b'\xcc'*section['size']
 for r in records:
  if r['name']not in cfg['accepted']and r['name']!=record['name']:continue
  for a,b in [[r['start'],r['end']]]+r.get('shared',[]):
   lo,hi=original.offset(a),original.offset(b-1)+1;expected[lo:hi]=original.data[lo:hi]
 expected[original.offset(by[m['direct_marker']]['start'])]=0xcc
 assert bytes(expected)==(d/'original-masked.exe').read_bytes(),'original candidate ownership differs'
 target=C/'expected'/('%02d'%i);target.mkdir(parents=True)
 for name in ['events','stdout','stderr','receipt.json']:
  shutil.copy2(d/name,target/name)
 shutil.copytree(d/'work',target/'work')
 out.append(dict(input=row['input'],directory=str(target.relative_to(C)),files={str(p.relative_to(target)):hashlib.sha256(p.read_bytes()).hexdigest()for p in target.rglob('*')if p.is_file()},candidate_ownership=dict(mask_recipe_matches=True,accepted_prefix=997,candidate_range=[record['start'],record['end']],candidate_shared=record.get('shared',[]),masked_image_sha256=hashlib.sha256(expected).hexdigest())))
m['direct_oracles']=out;m['direct_definedness']='evidence/definedness-and-caller-proof.json';m['direct_termination']='Both debugger hosts succeed only after matching selected entry or complete direct return checkpoint. Original debugger then deliberately terminates child99; native GDB kills its stopped inferior. These are debugger retirement policies, not ordinary C1 compilation statuses. Phase expected_exit5 describes the unchanged full initialization source, not the direct call.';(C/'manifest.json').write_text(json.dumps(m,indent=2)+'\n')
cfg['accepted']+=['_report_asm_expected_token@8','_asm_recovery_value@4'];cfg['direct_cases']={'asm-direct-callbacks':dict(accepted_from=997,accepted_count=999,public_reachability=False,units=15,manifest='cases/asm-direct-callbacks/manifest.json')};(B/'functions.json').write_text(json.dumps(cfg,indent=2)+'\n');print('Frozen15 directunits; candidate full masked-image recipes verified; accepted999')
