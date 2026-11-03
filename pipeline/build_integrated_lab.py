"""Extend the original renderer with modular customization; never replace its shader."""
import json,re,runpy
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
# Build the proven full-shading viewer first, then replace its asset input only.
runpy.run_path(str(ROOT/'pipeline/build_viewer.py'))
path=ROOT/'output/relight-lab.html';html=path.read_text(encoding='utf-8')
cars={k:json.loads((ROOT/'output'/(k+'-modular')/'assets.json').read_text()) for k in ['silvia','civic']}
definition='const MODULAR = '+json.dumps(cars,separators=(',',':'))+";\nlet ASSETS = {meta:MODULAR.silvia.meta};\nfunction selectedAssets(){return {meta:MODULAR[state.car].meta};}"
html,count=re.subn(r'const CARS = .*?;\nlet ASSETS = .*?;\nfunction selectedAssets\(\)\{[^\n]+\}',lambda _:definition,html,flags=re.S);assert count==1
controls='''<div class="panel"><h2>Customization</h2><div class="ctl">
<div class="row"><label for="bodyPaint">Body paint</label><input type="color" id="bodyPaint" value="#ffeaba"></div>
<label class="chk" id="twoToneRow"><input type="checkbox" id="twoTone" checked> Two-tone Silvia</label>
<div class="row" id="lowerPaintRow"><label for="lowerPaint">Lower body paint</label><input type="color" id="lowerPaint" value="#aeaeae"></div>
<label class="chk"><input type="checkbox" id="blackHood"> Black hood</label>
<label class="chk"><input type="checkbox" id="carbonHood"> Carbon pattern</label>
<div class="row"><label for="rideHeight">Ride height</label><select id="rideHeight"><option value="0">Stock</option><option value="1">Low - 1 px</option><option value="2">Lowest - 2 px</option></select></div>
<div class="row"><label for="pose">Steering</label><select id="pose"><option value="left">Left</option><option value="straight" selected>Straight</option><option value="right">Right</option></select></div>
<div class="row"><label for="partView">Show</label><select id="partView"><option value="complete">Complete car</option><option value="body">Body only</option><option value="wheels">Wheels only</option></select></div>
</div></div>'''
html=html.replace('<h2>Car feel</h2>', '<h2>Car feel</h2>',1)
html=html.replace('      <div class="panel">\n        <h2>Car feel</h2>',controls+'\n      <div class="panel">\n        <h2>Car feel</h2>',1)
# Comparison controls no longer choose a different renderer or dataset.
html=re.sub(r'<div><div class="seglabel">Cleanup</div>.*?</div></div>','<div hidden><div id="segCleanup"><button data-v="after" aria-pressed="true">After</button></div></div>',html,count=1)
html=html.replace('id="wheelClean" checked','id="wheelClean" checked hidden').replace('id="lampDetail" checked','id="lampDetail" checked hidden')
html=html.replace('> Smooth wheel edges</label>','></label>').replace('> Detailed headlights</label>','></label>')
html=html.replace('id="headon" checked','id="headon"')
html=html.replace('<label class="chk"><input type="checkbox" id="brake">','<label class="chk"><input type="checkbox" id="tailon"> Tail lights</label><label class="chk"><input type="checkbox" id="brake">')
html=html.replace("P.brake=$('brake').checked;","P.tailon=$('tailon').checked; P.brake=$('brake').checked;")
html=html.replace('emR = P.brake ? 3.2 : 1.5','emR = P.brake ? 3.2 : P.tailon ? 1.5 : 0')
html=html.replace('if (P.carlights){\n    const rc=', 'if (P.carlights && (P.tailon || P.brake)){\n    const rc=')
# Front spill must remain independent of the rear switch.
html=html.replace('    if (headsOn){ const fc=', '  }\n  if (P.carlights){\n    if (headsOn){ const fc=',1)
html=html.replace('#carlights,#brake,#hazard','#carlights,#tailon,#brake,#hazard')
start=html.index('async function getSet(view, st){');end=html.index('const toLin =',start)
html=html[:start]+html[end:]
module=(ROOT/'pipeline/relight-customization.js').read_text(encoding='utf-8')
html=html.replace('setSprite();\n</script>',module+'\nsetSprite();\n</script>')
assert 'const modularDecodeCache=' in html
html=html.replace('height:100vh','min-height:100vh')
html=html.replace('</style>','[hidden]{display:none!important}select{min-width:0;width:100%;background:var(--panel);color:var(--text);border:1px solid var(--line);padding:5px}</style>',1)
path.write_text(html,encoding='utf-8')
print('INTEGRATED full Relight Lab',len(html.encode()),'bytes')
# Inline version uses the SAME full shader and controls, with one wheel-spin
# phase to keep it below 1 MB. The full browser version retains all nine phases.
static={k:{'meta':v['meta'],'data':v['data'],'animation':{label:[layers['wheels']] for label,layers in v['data'].items()}} for k,v in cars.items()}
pool=[];lookup={}
def intern(value):
    if isinstance(value,dict):return {k:intern(v) for k,v in value.items()}
    if isinstance(value,list):return [intern(v) for v in value]
    if isinstance(value,str) and value.startswith('data:image/'):
        if value not in lookup:lookup[value]=len(pool);pool.append(value)
        return {'imageIndex':lookup[value]}
    return value
compact=intern(static)
replacement='const IMAGE_POOL='+json.dumps(pool,separators=(',',':'))+';\nconst MODULAR='+json.dumps(compact,separators=(',',':'))+';\nfunction restoreImages(value){if(value&&typeof value==="object"){if("imageIndex" in value)return IMAGE_POOL[value.imageIndex];for(const key of Object.keys(value))value[key]=restoreImages(value[key]);}return value;}restoreImages(MODULAR);'
inline=re.sub(r'const MODULAR = .*?;\nlet ASSETS',lambda _:replacement+'\nlet ASSETS',html,count=1,flags=re.S)
inline=inline.replace('const $ = id => document.getElementById(id);','const labRoot=document.getElementById("integrated-relight");\nconst $ = id => labRoot.querySelector("#"+id);')
inline=inline.replace('document.querySelectorAll(', 'labRoot.querySelectorAll(').replace("document.addEventListener('input',saveLab)","labRoot.addEventListener('input',saveLab)")
style=re.search(r'<style>([\s\S]*?)</style>',inline).group(1)
body=re.search(r'<body>([\s\S]*?)</body>',inline).group(1)
style=style.replace(':root{','#integrated-relight{').replace('html,body{','#integrated-relight{')
# Scope every rule, including selectors nested in responsive media queries.
def scope(css):
    css=re.sub(r'/\*[\s\S]*?\*/','',css);result='';pos=0
    while pos<len(css):
        start=css.find('{',pos)
        if start<0:break
        head=css[pos:start].strip();depth=1;end=start+1
        while end<len(css) and depth:
            depth+=(css[end]=='{')-(css[end]=='}');end+=1
        content=css[start+1:end-1]
        if head.startswith('@'):result+=head+'{'+scope(content)+'}'
        else:result+=','.join(s.strip() if s.strip().startswith('#integrated-relight') else '#integrated-relight '+s.strip() for s in head.split(','))+'{'+content+'}'
        pos=end
    return result
style=scope(style)
style+='\n#integrated-relight .grid{grid-template-columns:1fr}#integrated-relight .stagebox{max-width:500px}#integrated-relight{background:transparent}#integrated-relight .wrap{padding:0}#integrated-relight [hidden]{display:none!important}'
style+='\n@media(min-width:900px){#integrated-relight .grid{grid-template-columns:minmax(0,1fr) 320px}}'
body=body.replace('Shake and wheel spin follow the highway speed. Wheels use 8 pre-rendered spin frames plus a blurred set. Lights glow from an emission map.','The browser viewer includes animated wheel spin. Lights use the same emission and spill renderer here.')
fragment='<div id="integrated-relight">\n<style>'+style+'</style>\n'+body+'\n</div>'
dest=Path('C:/Users/jyoti/.codex/visualizations/2026/11/02/01a19b50-a458-76c3-94ca-fe4b51dc33ea/car-relighting.html')
assert len(fragment.encode())<1000000,len(fragment.encode())
dest.write_text(fragment,encoding='utf-8')
(ROOT/'output/preview-qa.html').write_text('<!doctype html><meta charset="utf-8">'+fragment,encoding='utf-8')
print('INTEGRATED INLINE',len(fragment.encode()),'bytes')
