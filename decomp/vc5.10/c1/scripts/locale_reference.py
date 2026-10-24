"""Authenticate own original API closure before native host selection binding."""
import importlib.util,json
from pathlib import Path
from provenance import sha

def bind(original,output):
 path=Path(__file__).with_name('check-isolated-locale.py')
 spec=importlib.util.spec_from_file_location('own_locale_reference',path);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
 result=module.check(original)
 assert result['kind']=='own original-only canonical setlocale plus _setmbcp witness; no native API success or app acceptance'
 result['binding_role']='native host input authorization only; does not perform locale or MB transition'
 (output/'isolated-locale-binding.json').write_text(json.dumps(result,indent=2)+'\n')
 return sha(original/'receipt.json')
