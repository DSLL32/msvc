"""Native capture paths and content identity, without original transport imports."""
from pathlib import Path
from provenance import sha
PROJECT=Path(__file__).resolve().parents[1]
ROOT=PROJECT.parents[2]
SCRATCH=Path.home()/'.ai-tmp/msvc2/vc5.10/c1'
