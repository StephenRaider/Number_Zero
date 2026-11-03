"""Freeze the version shown before the cleanup experiment for an honest A/B."""
import json,base64
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
viewer=(ROOT/'output/relight-lab.html').read_text(encoding='utf-8')
cars,_=json.JSONDecoder().raw_decode(viewer.split('const CARS = ',1)[1])
for key in ('silvia','civic'):
    path=ROOT/'output'/key/'assets.before.json'
    if not path.exists():
        baseline=cars[key]
        baseline.pop('improved',None)
        path.write_text(json.dumps(baseline,separators=(',',':')))
        folder=path.parent/'sprites-before';folder.mkdir(exist_ok=True)
        for state,frames in baseline['data'].items():
            for fi,frame in enumerate(frames):
                for kind,url in frame.items():
                    (folder/f'{state}_f{fi}_{kind}.png').write_bytes(base64.b64decode(url.split(',')[1]))
        print('CAPTURED',key)
