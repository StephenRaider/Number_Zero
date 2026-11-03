import sys,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'.runtime'))
import numpy as np
from PIL import Image, ImageDraw
sheet=Image.new('RGB',(768,512),(30,34,44));draw=ImageDraw.Draw(sheet)
for row,car in enumerate(('silvia','civic')):
    assets=json.loads((ROOT/'output'/car/'assets.json').read_text())
    assert len(assets['data'])==2
    for col,view in enumerate(('front','back')):
        assert len(assets['data'][view+'_down'])==9
        p=ROOT/'output'/car/'sprites'
        im=Image.open(p/f'{view}_down_f0_albedo.png'); a=np.array(im)
        assert im.size==(128,128)
        opaque=a[:,:,3]>0
        ys,xs=np.where(opaque)
        assert xs.min()>0 and xs.max()<127 and ys.min()>0 and ys.max()<127, 'Sprite cropped'
        white=opaque & (a[:,:,:3].min(-1)==255)
        grey=opaque & (np.abs(a[:,:,:3].astype(int)-[66,69,75]).max(-1)<6)
        assert white.sum()>=3, 'White plates missing'
        assert grey.sum()>40, 'Dark grey glass missing'
        assert set(np.unique(a[:,:,3]))<=set((0,255))
        f1=np.array(Image.open(p/f'{view}_down_f1_albedo.png'))
        assert np.any(a!=f1), 'Wheel spin did not change'
        raw=np.array(Image.open(ROOT/'output'/car/f'{view}_f0_depth.png'))
        far=np.array(Image.open(ROOT/'output'/car/f'{view}_f0_far.png'))[:,::-1]
        hi=np.array(Image.open(ROOT/'output'/car/f'{view}_f0_albedo.png'))[:,:,3]>128
        assert np.quantile((raw.astype(float)-far)[hi],.75)>1000,'Depth thickness missing'
        if car=='silvia' and view=='back':
            for frame in range(9):
                em=np.array(Image.open(p/f'back_down_f{frame}_emit.png'))
                assert np.count_nonzero(em[78:93,80:102,2])>=2, 'Near rear indicator missing'
                assert np.count_nonzero(em[72:86,103:119,2])>=1, 'Far rear indicator missing'
                assert not np.any((em[:,:,1]>128)&(em[:,:,2]>128)), 'Tail and indicator masks overlap'
        scaled=im.resize((384,384),Image.Resampling.NEAREST)
        # Crop transparent padding for a compact contact sheet.
        scaled=scaled.crop((0,70,384,326))
        sheet.paste(scaled,(col*384,row*256),scaled)
        draw.text((col*384+12,row*256+8),f'{car.upper()} / {view.upper()}',fill=(235,235,235))
        print(car,view,'PASS','white pixels',int(white.sum()),'grey glass',int(grey.sum()),'bounds',[int(xs.min()),int(ys.min()),int(xs.max()),int(ys.max())])
sheet.save(ROOT/'output/car-sprites.png')
