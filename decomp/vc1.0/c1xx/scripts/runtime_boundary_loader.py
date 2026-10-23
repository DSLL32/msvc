"""Load the shared local offline runtime identity helper."""
import importlib.util
from pathlib import Path


def load_boundary():
    spec=importlib.util.spec_from_file_location('c1xx_runtime_boundary',Path(__file__).with_name('runtime-boundary.py'))
    module=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module
