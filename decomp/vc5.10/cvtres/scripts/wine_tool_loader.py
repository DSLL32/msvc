"""Only original capture tools may import this module."""
import importlib.util
from pathlib import Path
spec=importlib.util.spec_from_file_location('wine_tool',Path(__file__).with_name('wine-tool.py'))
tool=importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)
