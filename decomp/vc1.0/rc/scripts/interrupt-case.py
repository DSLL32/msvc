"""Use the immutable original-backed, real-console interrupt analysis runner."""
import importlib.machinery
import json
from pathlib import Path
from binary import PE, imports

capture = importlib.machinery.SourceFileLoader('interrupt_capture',
    str(Path('../fixtures/rc-interrupt/capture.py').resolve())).load_module()


def run(executable, arguments, work, stem, original=False):
    pe = PE(executable)
    thunk = next(r['start']-pe.base for r in imports(pe) if r['name']=='import:SetConsoleCtrlHandler')
    if original:
        functions = {r['name']:r['start']-pe.base for r in json.loads(Path('functions.json').read_text())['driver']}
    else:
        import compare
        functions = {n:a-pe.base for n,a in compare.publics().items()}
    return capture.execute(executable,arguments,work,stem,
        [thunk,functions['_initialize_lexer@4'],functions['_interrupt_resource@4']])
