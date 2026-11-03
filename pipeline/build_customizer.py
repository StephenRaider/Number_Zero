import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
cars={k:json.loads((ROOT/'output'/(k+'-modular')/'assets.json').read_text())['data'] for k in ['silvia','civic']}
pool=[];lookup={}
def intern(x):
    if isinstance(x,dict):return {k:intern(v) for k,v in x.items()}
    if x not in lookup:lookup[x]=len(pool);pool.append(x)
    return lookup[x]
cars=intern(cars)
old=(ROOT/'pipeline/lighting-preview.html').read_text(encoding='utf-8')
draw=old[old.index('  function draw()'):old.index('  for(const [key,e]')]
draw=draw.replace('if(car&&s.lamps&&s.lit)','if(car)')
draw=draw.replace('front=map.e[j]/255,rear=map.e[j+1]/255','front=s.headlights?map.e[j]/255:0,rear=(s.brake?1.8:s.taillights?.65:0)*map.e[j+1]/255')
fragment=(ROOT/'pipeline/customization-controls.html').read_text(encoding='utf-8')+'\n<script>\n'+(ROOT/'pipeline/customization.js').read_text(encoding='utf-8').replace('__DRAW_FUNCTION__',draw).replace('__PREVIEW_DATA__',json.dumps(cars,separators=(',',':'))).replace('__PREVIEW_IMAGES__',json.dumps(pool,separators=(',',':')))+'\n</script>'
dest=Path('C:/Users/jyoti/.codex/visualizations/2026/11/02/01a19b50-a458-76c3-94ca-fe4b51dc33ea/car-customization.html')
dest.write_text(fragment,encoding='utf-8')
assert len(fragment.encode())<1000000,len(fragment.encode())
wrapper='''<!doctype html><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Car Customization Lab</title><style>:root{--font-size-base:14px;--background:#151820;--foreground:#eee;--muted:#272d39;--border:#414858;color-scheme:dark}body{background:var(--background);color:var(--foreground);font:14px system-ui;max-width:850px;margin:24px auto;padding:0 16px}.viz-controls,.viz-row{display:flex;gap:12px;flex-wrap:wrap;align-items:center}.form-label{display:grid;gap:5px;flex:1;min-width:135px}.form-select,.form-control{background:#272d39;color:#eee;border:1px solid #555d70;border-radius:5px;padding:7px;min-height:34px}.form-control-color{width:100%;height:35px;padding:2px}.form-check{display:flex;gap:5px;align-items:center;min-height:34px}.sr-only{position:absolute;width:1px;height:1px;overflow:hidden}</style>'''
for name in ['customization-lab.html','customization-qa.html']:(ROOT/'output'/name).write_text(wrapper+fragment,encoding='utf-8')
print('CUSTOMIZER',len(fragment.encode()),'bytes',len(pool),'unique maps')
