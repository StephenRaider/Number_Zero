"""Adapt the supplied packer while retaining its 16-bit map and JSON contract."""
import sys, json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'.runtime'))
KEY=sys.argv[1]
OUT=ROOT/'output'/KEY
source=(ROOT/'pipeline/pack4.py').read_text()
source=source.replace('P="/tmp/work/out_v3/iso2to1_"', 'P='+repr(str(OUT)+'/'))
source=source.replace('f"{P}{v}_{s}_128_f{f}_{k}.png"','f"{P}{v}_f{f}_{k}.png"')
source=source.replace('meta=json.load(open("/tmp/work/out/iso2to1_meta.json"))','meta=json.load(open('+repr(str(OUT/'meta.json'))+'))')
source=source.replace('open("/tmp/work/assets4.json","w")','open('+repr(str(OUT/'assets.json'))+',"w")')
# A 4x4 block can straddle red and amber lens sections. Pick the dominant
# coverage channel before boosting small lamps, so hazards cannot drive red
# brake pixels and the two controls remain independent at sprite resolution.
source=source.replace('/ws*3,0,1)', '/ws,0,1)')
if KEY.endswith('-detailed'):
    # Detailed lamps encode reflector intensity, rather than a binary lens ID.
    source=source.replace('(blocks(em[...,c])>0.5)', 'blocks(em[...,c])')
source=source.replace('    ALB=mode_colour', '    EM=np.eye(3,dtype=np.float32)[EM.argmax(-1)]*np.clip(EM.max(-1)[...,None]*3,0,1)\n    ALB=mode_colour')
if KEY.endswith('-detailed'):
    # The viewer adds a multi-pixel halo as well as direct lamp emission.
    # Keep bright reflector cores legible after that additive accumulation.
    source=source.replace('    ALB=mode_colour', '    EM[...,0]*=.45\n    ALB=mode_colour')
    source=source.replace('em.max(-1)>0.05','em.max(-1)>0.005')
# Keep mandatory white plates and grey glass out of colour merging and outlines.
source=source.replace('    # 2. merge', '    prot |= (A.min(-1)>248) | (np.abs(A-np.array([66,69,75])).max(-1)<6)\n    # 2. merge')
# No popup-headlight animation exists in either supplied model. Use one physical state.
source=source.replace('for s in ("down","up"):', 'for s in ("down",):')
source=source.replace('data={}; total=0', 'data={}; alt={}; total=0')
source=source.replace('        al=np.stack', '        alt[f"{v}_{s}"]=[b64(np.dstack((x["alb"].astype(np.uint8),x["a"].astype(np.uint8)*255))) for x in fr]\n        alt[f"{v}_{s}"].append(alt[f"{v}_{s}"][0])\n        al=np.stack')
source=source.replace('{"data":data,"meta":meta}', '{"data":data,"meta":meta,"alt":alt}')
exec(compile(source,'adapted_pack4.py','exec'))
# Export individual images for game integration as well as the embedded viewer data.
import base64
assets=json.loads((OUT/'assets.json').read_text())
sprites=OUT/'sprites'; sprites.mkdir(exist_ok=True)
for state,frames in assets['data'].items():
    for fi,packed_frame in enumerate(frames):
        for kind,url in packed_frame.items():
            (sprites/f'{state}_f{fi}_{kind}.png').write_bytes(base64.b64decode(url.split(',')[1]))
print('PACKED',KEY,flush=True)
from improved_cleanup import build_improved
comparison_before=OUT/'assets.before.json'
if not comparison_before.exists() or KEY.endswith('-detailed'):
    comparison_before.write_text(json.dumps(assets,separators=(',',':')))
build_improved(KEY,json.loads(comparison_before.read_text()),globals())
if (OUT/'assets.improved.json').exists():
    from wheel_cleanup import build_wheels
    build_wheels(KEY)
