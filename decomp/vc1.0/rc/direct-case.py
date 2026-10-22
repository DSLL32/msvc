"""Invoke the two explicitly labelled direct helpers; never fabricate CLI use."""
import importlib.machinery
import shutil
from pathlib import Path
from binary import PE

fixture=importlib.machinery.SourceFileLoader('direct_capture',
    str(Path('cases/direct-helpers/capture.py').resolve())).load_module()


def run(label,executable,mode,work,stem,names):
    helper=fixture.HELPERS[mode]
    rva=helper['rva'] if label=='original' else names[helper['name']]-PE(executable).base
    return fixture.run(executable,mode,rva,work,stem)


def retain(label,mode,work,stem):
    destination=Path('build/direct-helper-results')/mode/label
    destination.mkdir(parents=True,exist_ok=True)
    for suffix in ('stdout','stderr','events','events.direct'):
        shutil.copyfile(Path(str(stem)+'.'+suffix),destination/suffix)
    shutil.copyfile(work/'results.bin',destination/'results.bin')
