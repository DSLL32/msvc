"""Share the structurally checked COFF/archive/PE parser with the debug fixture."""
from importlib.machinery import SourceFileLoader
from pathlib import Path
_module = SourceFileLoader('link_debug_artifacts', str(Path(__file__).resolve().parent.parent / 'link-debug-dll/artifacts.py')).load_module()
timestamp_fields = _module.timestamp_fields
normalize_artifact = _module.normalize_artifact
