"""Compatibility import; the shared implementation lives in recon-tool."""
import importlib.util as _util
from pathlib import Path as _Path
import sys as _sys
_directory=_Path(__file__).resolve().parent.parent/'recon-tool'/'watcom'
_sys.path.insert(0,str(_directory))
_spec=_util.spec_from_file_location(__name__,_directory/_Path(__file__).name)
_module=_util.module_from_spec(_spec)
_sys.modules[__name__]=_module
_spec.loader.exec_module(_module)
globals().update(_module.__dict__)
