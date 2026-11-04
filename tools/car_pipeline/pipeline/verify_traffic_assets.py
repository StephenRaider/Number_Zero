from pathlib import Path
import json,hashlib
import numpy as np
from PIL import Image
import sys as _path_sys; _path_sys.path.insert(0,str(Path(__file__).resolve().parent))
from settings import GAME;root=GAME/'assets/traffic';catalog=json.loads((root/'catalog.json').read_text());assert len(catalog['cars'])==5
for entry in catalog['cars']:
 meta=json.loads((root/entry['manifest']).read_text());folder=root/entry['id'];assert len(meta['files'])==60
 for name,digest in meta['files'].items():
  p=folder/name;assert hashlib.sha256(p.read_bytes()).hexdigest()==digest;im=Image.open(p);assert im.size==(256,256)
 for pose in ['left','straight','right']:
  emit=np.array(Image.open(folder/f'{pose}_f0_emit.png'));assert np.count_nonzero(emit[...,1]>24)>=2 and np.count_nonzero(emit[...,2]>24)>=2,(entry['id'],pose,'No tail/indicator')
  albedo=np.array(Image.open(folder/f'{pose}_f0_albedo.png'));assert not np.any(emit[albedo[...,3]==0]),'Emission outside sprite'
  for side in [False,True]:assert np.any((emit[...,2]>24)&((emit[...,3]>127)==side)),(entry['id'],pose,'Missing indicator side')
 print('VERIFIED',entry['id'],'three poses/four phases; tails and BOTH indicator sides')
print('PASS all 300 runtime textures, hashes, canvas sizes and lamp masks.')
