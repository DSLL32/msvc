"""Production-preflight checks of the single reached by-value Extended ABI."""
import argparse,contextlib,copy,hashlib,importlib.util,io,json,shutil,sys,tempfile,time
from pathlib import Path
from case_paths import BINARY,CAPTURE_ROOT
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('proof',type=Path);args=parser.parse_args();started=time.perf_counter()
 original=json.loads(args.proof.read_text());assert original['function']=='_save_extended_in_arena@16'
 directory=Path(tempfile.mkdtemp(prefix='extended-value-negatives-',dir=CAPTURE_ROOT));(directory/'functions.json').write_bytes((args.proof.parent/'original-configuration.json').read_bytes());(directory/'build/native').mkdir(parents=True)
 executable=BINARY/'build/native/C1XX3232.EXE';assert sha(executable)==original['native_executable_sha256'];(directory/'build/native/C1XX3232.EXE').symlink_to(executable)
 spec=importlib.util.spec_from_file_location('extended_argument_preflight',BINARY/'scripts/advance-frontier.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);module.BINARY=directory;results=[]
 for name in ['control','wrong-high-word','wrong-format-word','wrong-arena','missing-word-metadata']:
  target=directory/name;shutil.copytree(args.proof.parent,target);proof=copy.deepcopy(original);native=json.loads((target/'native-checkpoint.json').read_text())
  if name=='wrong-high-word':native['value_high']^=1
  if name=='wrong-format-word':native['value_tail']^=0x10000
  if name=='wrong-arena':native['raw_arena']^=1
  if name=='missing-word-metadata':proof['formals'][0]['by_value_words']['original'].pop()
  (target/'native-checkpoint.json').write_text(json.dumps(native,indent=2)+'\n')
  for filename in proof['evidence_sha256']:proof['evidence_sha256'][filename]=sha(target/filename)
  (target/'validation.json').write_text(json.dumps(proof,indent=2)+'\n');sys.argv=['advance-frontier.py','--case',proof['case'],'--candidate',proof['function'],'--argument-proof',str(target/'validation.json'),'--check-arguments-only'];output=io.StringIO()
  try:
   with contextlib.redirect_stdout(output):module.main()
  except AssertionError as error:
   assert name!='control',str(error);results.append(dict(test=name,result='held',reason=str(error)))
  else:
   assert name=='control','corrupted by-value argument passed';results.append(dict(test=name,result='verified',output=output.getvalue().strip()))
 receipt=dict(scope='Read-only actual production preflight and bounded value/high/format/arena word corruption; no original execution or promotion',proof=str(args.proof),proof_sha256=sha(args.proof),checks=results,elapsed_seconds=time.perf_counter()-started);(directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(directory/'receipt.json')
if __name__=='__main__':main()
