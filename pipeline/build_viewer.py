import json, re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
cars={}
for k in ('silvia','civic'):
    folder=ROOT/'output'/k
    before=folder/'assets.before.json'
    cars[k]=json.loads((before if before.exists() else folder/'assets.json').read_text())
    cars[k]['improved']=json.loads((folder/'assets.improved.json').read_text())
    cars[k]['wheels']=json.loads((folder/'assets.wheels.json').read_text())
    detailed=ROOT/'output'/(k+'-detailed')
    cars[k]['detailed']=json.loads((detailed/'assets.json').read_text())
    cars[k]['detailed']['improved']=json.loads((detailed/'assets.improved.json').read_text())
    cars[k]['detailed']['wheels']=json.loads((detailed/'assets.wheels.json').read_text())
template=(ROOT/'pipeline/template.html').read_text(encoding='utf-8')
template=template.replace('Testarossa Relight Test','Silvia S13 / Civic EK9 — Relight Lab')
template=template.replace('so a 1 m ground tile is 28×14 px at the 128 px scale. Glass is tinted black.','128 px sprites at a shared 26 px/m scale. Windows are opaque dark grey; license plates are plain white.')
template=template.replace('<div class="seglabel">View</div>', '<div class="seglabel">Car</div><div class="seg" id="segCar"><button data-v="silvia" aria-pressed="true">Silvia S13</button><button data-v="civic" aria-pressed="false">Civic EK9</button></div></div><div><div class="seglabel">View</div>',1)
template=template.replace('<div><div class="seglabel">View</div>', '<div><div class="seglabel">Cleanup</div><div class="seg" id="segCleanup"><button data-v="before" aria-pressed="false">Before</button><button data-v="after" aria-pressed="true">After</button></div></div><div><div class="seglabel">View</div>',1)
template=template.replace('<div><div class="seglabel">View</div>', '<label class="chk"><input type="checkbox" id="wheelClean" checked> Smooth wheel edges</label><div><div class="seglabel">View</div>',1)
template=template.replace('<div><div class="seglabel">View</div>', '<label class="chk"><input type="checkbox" id="lampDetail" checked> Detailed headlights</label><div><div class="seglabel">View</div>',1)
template=template.replace('<div class="row"><label for="cleanSp">Cleaned sprite</label><input type="checkbox" id="cleanSp" checked><span></span></div>', '<input type="checkbox" id="cleanSp" checked hidden>')
template=template.replace("clean:true };", "clean:true, car:'silvia', cleanup:'after' };")
template=re.sub(r'<div><div class="seglabel">Headlights</div>.*?</div></div>', '<div hidden><div class="seg" id="segState"><button data-v="down" aria-pressed="true">Fixed headlights</button></div></div>',template,count=1)
template=template.replace('Headlights on (popped up)','Headlights on').replace('Headlight beams (when popped up)','Headlight beams')
template=template.replace("P.headon && state.st==='up'",'P.headon')
template=template.replace('value="0.04"','value="0.14"')
template=template.replace('(WH.ang+adv)%72','(WH.ang+adv)%360').replace('Math.floor(WH.ang/9)%8','Math.floor(WH.ang/45)%8').replace('(Math.floor(WH.step)*2)%8','Math.floor(WH.step)%8')
template=template.replace('const ASSETS = __ASSETS__;', 'const CARS = '+json.dumps(cars,separators=(',',':'))+';\nlet ASSETS = CARS.silvia.detailed.wheels;\nfunction selectedAssets(){const car=$(\'lampDetail\').checked?CARS[state.car].detailed:CARS[state.car];return state.cleanup===\'before\'?car:($(\'wheelClean\').checked?car.wheels:car.improved);}')
template=template.replace('const cache = {};','let cache = {};')
template=template.replace('async function setSprite(){\n  const raw = await getSet(state.view, state.st);', 'let spriteRequest=0;\nasync function setSprite(){\n  const request=++spriteRequest;\n  const raw = await getSet(state.view, state.st);\n  if(request!==spriteRequest)return;')
template=template.replace("seg('segView','view');", "$('segCar').addEventListener('click', e=>{ const b=e.target.closest('button'); if(!b)return; [...$('segCar').children].forEach(x=>x.setAttribute('aria-pressed',x===b)); state.car=b.dataset.v; ASSETS=state.cleanup==='after'?CARS[state.car].improved:CARS[state.car]; cache={}; setSprite(); });\n$('segCleanup').addEventListener('click', e=>{ const b=e.target.closest('button'); if(!b)return; [...$('segCleanup').children].forEach(x=>x.setAttribute('aria-pressed',x===b)); state.cleanup=b.dataset.v; ASSETS=state.cleanup==='after'?CARS[state.car].improved:CARS[state.car]; cache={}; setSprite(); });\nseg('segView','view');")
template=template.replace("ASSETS=state.cleanup==='after'?CARS[state.car].improved:CARS[state.car];", "$('wheelClean').disabled=state.cleanup==='before'; ASSETS=selectedAssets();")
template=template.replace("seg('segView','view');", "$('wheelClean').addEventListener('change',()=>{ASSETS=selectedAssets();cache={};setSprite();});\nseg('segView','view');")
template=template.replace("seg('segView','view');", "$('lampDetail').addEventListener('change',()=>{ASSETS=selectedAssets();cache={};setSprite();});\nseg('segView','view');")
template=template.replace('const q8=x=>bands>0?Math.floor(x*bands+0.5)/bands:x;', "const q8=x=>{if(bands<=0)return x; if(state.cleanup==='before')return Math.floor(x*bands+.5)/bands; const n=x*bands,base=Math.floor(n),t=Math.max(0,Math.min(1,(n-base-.35)/.30));return (base+t*t*(3-2*t))/bands;};")
template=template.replace('const sq=x=>x>0.35?(bands>0?Math.round(x*bands)/bands:x):0;', "const sq=x=>{if(state.cleanup==='before')return x>.35?(bands>0?Math.round(x*bands)/bands:x):0; const t=Math.max(0,Math.min(1,(x-.25)/.2));return q8(x)*t*t*(3-2*t);};")
template=template.replace('return new Promise(r => { const i = new Image(); i.onload = () => r(i); i.src = src; });','return new Promise((r,reject) => { const i = new Image(); i.onload = () => r(i); i.onerror=()=>reject(new Error("Sprite image failed to load")); i.src = src; });')
# Remain fully offline, including typography.
template=re.sub(r'<link[^>]+>\s*','',template)
template=template.replace('width="384" height="384"','width="384" height="384" role="img" aria-label="Car with dynamic lighting and shadows"')
(ROOT/'output/relight-lab.html').write_text(template,encoding='utf-8')
print('BUILT',ROOT/'output/relight-lab.html')
preview={k:{v:{'before':cars[k]['data'][v+'_down'][0],'after':cars[k]['improved']['data'][v+'_down'][0],'wheels':cars[k]['wheels']['data'][v+'_down'][0]} for v in ('front','back')} for k in cars}
for k in cars:
    for v in ('front','back'):
        detailed=cars[k]['detailed']
        preview[k][v]['detailed']={'before':detailed['data'][v+'_down'][0],'after':detailed['improved']['data'][v+'_down'][0],'wheels':detailed['wheels']['data'][v+'_down'][0]}
# Deduplicate embedded map images shared by cleanup variants and light modes.
image_pool=[];image_ids={}
def intern(value):
    if isinstance(value,dict): return {k:intern(v) for k,v in value.items()}
    if isinstance(value,str) and value.startswith('data:image/'):
        if value not in image_ids: image_ids[value]=len(image_pool);image_pool.append(value)
        return image_ids[value]
    return value
preview=intern(preview)
fragment=(ROOT/'pipeline/lighting-preview.html').read_text(encoding='utf-8').replace('__PREVIEW_DATA__',json.dumps(preview,separators=(',',':'))).replace('__PREVIEW_IMAGES__',json.dumps(image_pool,separators=(',',':')))
preview_dir=Path('C:/Users/jyoti/.codex/visualizations/2026/11/02/01a19b50-a458-76c3-94ca-fe4b51dc33ea')
(preview_dir/'car-relighting.html').write_text(fragment,encoding='utf-8')
assert len(fragment.encode())<1000000
# A local QA wrapper supplies the theme tokens and base layout of the chat surface.
wrapper='<!doctype html><meta charset="utf-8"><style>:root{--font-size-base:14px;--background:#151820;--foreground:#eeeeee;--muted:#272d39;--border:#414858}body{background:var(--background);color:var(--foreground);font:14px system-ui;max-width:736px;margin:24px auto}.viz-controls,.viz-row{display:flex;gap:16px;flex-wrap:wrap}.form-label{display:grid;gap:4px;flex:1;min-width:140px}.sr-only{position:absolute;width:1px;height:1px;overflow:hidden}</style>'
(ROOT/'output/preview-qa.html').write_text(wrapper+fragment,encoding='utf-8')
print('PREVIEW',preview_dir/'car-relighting.html')
