"""Regression checks for dark-paint coverage and the full-shader integration."""
import sys,json,base64,io,re,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'.runtime'))
import numpy as np
from PIL import Image
def image(url):return np.array(Image.open(io.BytesIO(base64.b64decode(url.split(',')[1]))))
for car in ['silvia','civic']:
    assets=json.loads((ROOT/'output'/(car+'-modular')/'assets.json').read_text())
    assert assets['wheels']['ride_body_z_m'][-1]>-.09,'Excessive lowering'
    for label,entry in assets['data'].items():
        a=image(entry['body']['albedo']);m=image(entry['body']['mask']);alpha=a[...,3]>0
        paint=m[...,:3].max(-1)>127
        source=np.array([255,234,186] if car=='silvia' else [219,229,237])
        source_like=(np.abs(a[...,:3].astype(int)-source).max(-1)<2)&alpha
        assert not (source_like&~paint).any(),f'{car} {label}: unmasked original paint pixels'
        # The original pale paint cannot leave bright islands after repainting.
        black=a.copy().astype(float)
        for ch,base in enumerate([source,[174,174,174] if car=='silvia' else source,source]):
            selected=m[...,ch]>127;black[selected,:3]=16*a[selected,:3]/base
        assert black[paint,:3].max()<18,f'{car} {label}: black paint fringe'
        frames=assets['animation'][label];assert len(frames)==9
        assert frames[0]['albedo']!=frames[1]['albedo'],'Wheel animation missing'
        for frame in frames:
            assert not image(frame['mask'])[...,:3].any(),'Wheel paint contamination'
    # Regression for the S13 curved front lip omitted by the former height cut.
    if car=='silvia':
        hood=image(assets['data']['front_straight']['body']['mask'])[...,2]>127
        assert hood.sum()>650,'Silvia hood nose is still clipped'
html=(ROOT/'output/relight-lab.html').read_text(encoding='utf-8')
assert 'function shadowed(' in html and 'const GLOW=' in html and 'const modularDecodeCache=' in html
for control in ['bodyPaint','lowerPaint','blackHood','pose','rideHeight','tailon','headon','brake','hazard','partView']:
    assert f'id="{control}"' in html
assert 'Lowest - 3 px' not in html
assert 'P.carlights && (P.tailon || P.brake)' in html
assert 'emR = P.brake ? 3.2 : P.tailon ? 1.5 : 0' in html
for name,content in [('integrated',html),('inline',(ROOT/'output/preview-qa.html').read_text(encoding='utf-8'))]:
    script=re.findall(r'<script>([\s\S]*?)</script>',content)[-1]
    target=ROOT/'output'/f'{name}-check.js';target.write_text(script,encoding='utf-8')
    subprocess.run(['node','--check',str(target)],check=True)
inline=Path('C:/Users/jyoti/.codex/visualizations/2026/11/02/01a19b50-a458-76c3-94ca-fe4b51dc33ea/car-relighting.html').read_bytes()
assert len(inline)<1000000
assert b'const modularDecodeCache=' in inline and b'function shadowed(' in inline
print('PASS: paint coverage, black fringes, hood nose, 0/1/2 px heights, 108 animated wheel maps, full shader, JS syntax, inline size')
