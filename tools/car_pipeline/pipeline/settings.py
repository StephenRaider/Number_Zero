"""Explicit locations: versioned code in the game repo, bulky work elsewhere."""
import os
from pathlib import Path
CODE=Path(__file__).resolve().parent
GAME=Path(os.environ.get('NZ_CAR_GAME',str(CODE.parents[2]))).resolve()
value=os.environ.get('NZ_CAR_WORKSPACE')
if not value:raise RuntimeError('Use tools/car_pipeline/run.py with --workspace <external experiment directory>')
WORKSPACE=Path(value).expanduser().resolve()
SOURCES=Path(os.environ.get('NZ_CAR_SOURCES',str(WORKSPACE/'sources'))).expanduser().resolve()
if WORKSPACE==GAME or GAME in WORKSPACE.parents:raise RuntimeError('Asset workspace must be outside the game checkout')
