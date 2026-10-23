"""Read native state at the actual target main entry, without inferior calls."""
import json
from pathlib import Path
import gdb


def capture(directory):
    assert gdb.newest_frame().name()=='main','initial breakpoint is not target main'
    values={name:int(gdb.parse_and_eval(expression))for name,expression in {
        'errno':'(int)errno','doserrno':'recon_doserrno',
        'last_error':'last_error','x87_control_word':'$fctrl'}.items()}
    (Path(directory)/'native-entry-errors.json').write_text(json.dumps(values,indent=2)+'\n')
