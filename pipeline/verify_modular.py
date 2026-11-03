"""Integration checks for masks, detached wheels, depth and steering output."""
import sys,json,io,base64
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'.runtime'))
import numpy as np
from PIL import Image,ImageDraw
def image(url):return np.array(Image.open(io.BytesIO(base64.b64decode(url.split(',')[1]))))
def depth(p):return (p[...,0].astype(float)*256+p[...,1])/65535*5.2-2.6
sheet=Image.new('RGB',(128*6,128*2),(42,46,56))
for row,key in enumerate(['silvia','civic']):
    assets=json.loads((ROOT/'output'/(key+'-modular')/'assets.json').read_text())
    data=assets['data'];assert len(data)==6
    assert len(assets['wheels']['centers'])==4
    for label,layers in data.items():
        for layer,entry in layers.items():
            maps={k:image(v) for k,v in entry.items()};a=maps['albedo'];mask=maps['mask'];alpha=a[...,3]>127
            assert a.shape==(128,128,4)
            assert not alpha[[0,-1],:].any() and not alpha[:,[0,-1]].any(),'Cropped sprite'
            for m in maps.values():assert np.array_equal(m[...,3],a[...,3]),'Map alignment'
            assert (mask[...,:3]>0).sum(-1).max()<=1,'Paint masks overlap'
            n=maps['normal'];q=maps['pack2']
            nx=(n[...,0].astype(float)*256+n[...,1])/65535*2-1;ny=(n[...,2].astype(float)*256+q[...,0])/65535*2-1
            assert (nx[alpha]**2+ny[alpha]**2).max()<1.002,'Invalid normal'
            if layer=='wheels':assert not mask[...,:3].any(),'Wheels recoloured by body mask'
            else:
                assert (mask[...,2]>0).sum()>200,'Bonnet mask missing'
                grey=(np.abs(a[...,:3].astype(int)-[66,69,75]).max(-1)<6)&alpha
                white=(a[...,:3].min(-1)>248)&alpha
                assert grey.sum()>40 and white.sum()>2,'Glass/white plate missing'
                assert not mask[grey,:3].any() and not mask[white,:3].any(),'Protected surface paint spill'
                assert (maps['emit'][...,:3]>0).any()
                if key=='silvia':assert (mask[...,1]>0).sum()>30,'Two-tone paint missing'
                else:assert not mask[...,1].any()
    # Both body and front wheels actually change pose maps.
    for layer in ['body','wheels']:
        assert data['front_left'][layer]['normal']!=data['front_right'][layer]['normal']
        assert data['front_left'][layer]['albedo']!=data['front_straight'][layer]['albedo']
    for col,(view,pose,ride) in enumerate([('front','left',0),('front','straight',0),('front','right',0),('back','straight',0),('front','straight',1),('front','straight',2)]):
        en=data[view+'_'+pose];b={k:image(v) for k,v in en['body'].items()};w={k:image(v) for k,v in en['wheels'].items()}
        drop=ride;a=np.zeros_like(b['albedo']);bd=np.full((128,128),-99.);hood=np.zeros((128,128),bool)
        length=128-drop;a[drop:]=b['albedo'][:length];bd[drop:]=depth(b['pack'])[:length]-drop/(26*np.cos(np.pi/6))*.5;hood[drop:]=b['mask'][:length,:,2]>0
        wd=depth(w['pack']);take=(w['albedo'][...,3]>0)&((a[...,3]==0)|(wd>bd));a[hood,:3]=[29,32,36];a[take]=w['albedo'][take]
        assert np.count_nonzero(take)>40,'All wheels hidden'
        # Lowering must not move tyre contact pixels.
        if ride:
            wheel_bottom=np.nonzero(w['albedo'][...,3])[0].max()
            assert a[wheel_bottom,:,3].max()>0
        im=Image.fromarray(a);sheet.paste(im,(col*128,row*128),im)
sheet.resize((1536,512),Image.Resampling.NEAREST).save(ROOT/'output/customization-verification.png')
fragment=Path('C:/Users/jyoti/.codex/visualizations/2026/11/02/01a19b50-a458-76c3-94ca-fe4b51dc33ea/car-customization.html').read_bytes()
assert len(fragment)<1000000
assert b'Before</button>' not in fragment and b'After</button>' not in fragment
assert b'fetch(' not in fragment
print('PASS: 24 layers, six poses per car, paint/hood protection, steering, lowering alignment, inline size')
