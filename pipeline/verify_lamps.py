"""Check detailed lens baking and independent Civic amber indicators."""
import sys,json,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'.runtime'))
import numpy as np
from PIL import Image,ImageDraw
from wheel_cleanup import decode
sheet=Image.new('RGB',(768,512),(30,34,44));draw=ImageDraw.Draw(sheet)
for row,car in enumerate(('silvia','civic')):
    folder=ROOT/'output'/(car+'-detailed')
    variants=[json.loads((folder/name).read_text()) for name in ('assets.json','assets.improved.json','assets.wheels.json')]
    for dataset in variants:
        assert dataset['meta']['detailed_lamps']
        for view,frames in dataset['data'].items():
            assert len(frames)==9
            for fi,entry in enumerate(frames):
                a=decode(entry['albedo']);em=decode(entry['emit'])
                assert a.shape==(128,128,4)
                assert np.array_equal(a[:,:,3],em[:,:,3])
                assert np.count_nonzero((a[:,:,:3].min(-1)==255)&(a[:,:,3]>0))>=3,'Plate white lost'
                assert np.count_nonzero((np.abs(a[:,:,:3].astype(int)-[66,69,75]).max(-1)<6)&(a[:,:,3]>0))>40,'Window grey lost'
                assert entry['emit']==variants[0]['data'][view][fi]['emit'],'Cleanup changed lamps'
                assert not np.any((em[:,:,0]>0)&(em[:,:,2]>0)),'Headlamp and indicator channels overlap'
                if view=='front_down':
                    assert 0<em[:,:,0].max()<160,'Headlight detail will wash out'
                    if car=='civic':
                        assert np.count_nonzero(em[80:92,78:95,2])>=1,'Civic front amber indicator missing'
                        assert len(np.unique(em[:,:,0][em[:,:,0]>0]))>=3,'Reflector brightness detail missing'
                if car=='silvia' and view=='back_down':
                    assert np.count_nonzero(em[78:93,80:102,2])>=2,'Near rear hazard lost'
                    assert np.count_nonzero(em[72:86,103:119,2])>=1,'Far rear hazard lost'
    old=ROOT/'output'/car/'front_f0_albedo.png'
    new=folder/'front_f0_albedo.png'
    for col,path in enumerate((old,new)):
        # Near-side headlamp crop at the native 4x render scale.
        im=Image.open(path).crop((304,312,412,384)).resize((324,216),Image.Resampling.NEAREST)
        sheet.paste(im,(col*384+30,row*256+30),im)
        draw.text((col*384+12,row*256+8),car.upper()+' / '+('OPAQUE LENS' if col==0 else 'DETAILED LENS'),fill=(235,235,235))
    print('PASS',car,'all 54 detailed frames preserve plates/windows and lamp masks; front glow restrained')
collected=Path('D:/Cars for game/2000_honda_civic_type_r_ek9.glb')
current=ROOT/'assets/civic/source/2000 Honda Civic Type-R (EK9).glb'
assert hashlib.sha256(collected.read_bytes()).digest()==hashlib.sha256(current.read_bytes()).digest()
print('ASSET CHECK: collection EK9 is identical to the current source; 1999 Civic Si inspected as coupe')
sheet.save(ROOT/'output/headlight-comparison.png')
