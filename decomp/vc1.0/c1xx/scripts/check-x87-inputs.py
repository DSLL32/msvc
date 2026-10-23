"""Focused production-preflight negatives for the reached private x87 inputs."""
import argparse,contextlib,copy,hashlib,importlib.util,io,json,shutil,sys,tempfile,time,re
from pathlib import Path
from case_paths import BINARY,CAPTURE_ROOT
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('proof',type=Path);args=parser.parse_args();started=time.perf_counter()
 original=json.loads(args.proof.read_text());assert original['function'] in ('_check_extended_range','_store_extended') and len(original['register_inputs'])==2
 store=original['function']=='_store_extended'
 directory=Path(tempfile.mkdtemp(prefix='x87-input-negatives-',dir=CAPTURE_ROOT));(directory/'functions.json').write_bytes((args.proof.parent/'original-configuration.json').read_bytes());(directory/'build/native').mkdir(parents=True)
 executable=BINARY/'build/native/C1XX3232.EXE';assert sha(executable)==original['native_executable_sha256'];(directory/'build/native/C1XX3232.EXE').symlink_to(executable)
 spec=importlib.util.spec_from_file_location('x87_argument_preflight',BINARY/'scripts/advance-frontier.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);module.BINARY=directory;results=[]
 names=['control','missing-operand','missing-esi','wrong-esi','wrong-caller-owner'] if store else ['control','missing-operand','swapped-raw-operands','short-read','wrong-caller-owner']
 for name in names:
  target=directory/name;shutil.copytree(args.proof.parent,target);proof=copy.deepcopy(original);native=json.loads((target/'native-checkpoint.json').read_text())
  if name=='missing-operand':native['x87_inputs']['inputs'].pop()
  if name=='swapped-raw-operands':
   values=native['x87_inputs']['inputs'];values[0]['raw'],values[1]['raw']=values[1]['raw'],values[0]['raw']
   native['x87_inputs']['raw_gdb']=re.sub(r'^(st([01])\s+.*\(raw 0x)[0-9a-f]{20}(\)\s*)$',lambda match:match[1]+bytes.fromhex(values[int(match[2])]['raw'])[::-1].hex()+match[3],native['x87_inputs']['raw_gdb'],flags=re.M)
  if name=='short-read':native['x87_inputs']['inputs'][1]['read_bytes']=9;native['x87_inputs']['inputs'][1]['read_succeeded']=False
  if name=='wrong-caller-owner':native['caller_locals']['caller_result' if store else 'caller_value']['value']+=4
  if name=='wrong-esi':native['raw_esi']+=4
  if name=='missing-esi':
   receipt=json.loads((target/'original-receipt.json').read_text());receipt['ownership_register_observations']=[];(target/'original-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
  (target/'native-checkpoint.json').write_text(json.dumps(native,indent=2)+'\n')
  for filename in proof['evidence_sha256']:proof['evidence_sha256'][filename]=sha(target/filename)
  (target/'validation.json').write_text(json.dumps(proof,indent=2)+'\n');sys.argv=['advance-frontier.py','--case',proof['case'],'--candidate',proof['function'],'--argument-proof',str(target/'validation.json'),'--check-arguments-only'];output=io.StringIO()
  try:
   with contextlib.redirect_stdout(output):module.main()
  except AssertionError as error:
   assert name!='control',str(error);results.append(dict(test=name,result='held',reason=str(error)))
  else:
   assert name=='control','corrupted x87 input metadata passed';results.append(dict(test=name,result='verified',output=output.getvalue().strip()))
 receipt=dict(scope='Read-only actual production preflight plus bounded synthetic missing/swapped/short-read/wrong-owner corruptions; no original execution or promotion',proof=str(args.proof),proof_sha256=sha(args.proof),checks=results,elapsed_seconds=time.perf_counter()-started);(directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(directory/'receipt.json')
if __name__=='__main__':main()
