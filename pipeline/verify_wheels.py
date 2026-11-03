import sys,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'.runtime'))
import numpy as np
from PIL import Image,ImageDraw
from wheel_cleanup import decode,blocks,components

sheet=Image.new('RGB',(768,768),(30,34,44));draw=ImageDraw.Draw(sheet)
for ci,car in enumerate(('silvia','civic')):
    folder=ROOT/'output'/car
    before=json.loads((folder/'assets.improved.json').read_text())
    after=json.loads((folder/'assets.wheels.json').read_text())
    ids=json.loads((folder/'material_ids.json').read_text())
    wheel_ids=[v for k,v in ids.items() if any(s in k.lower() for s in ('wheel','rim','tire'))]
    total=0
    for vi,view in enumerate(('front','back')):
        union=np.zeros((128,128),bool);regions=[]
        for fi in range(8):
            hi=np.array(Image.open(folder/f'{view}_f{fi}_albedo.png'))
            material=blocks(np.array(Image.open(folder/f'{view}_f{fi}_material.png')))
            wheel=(np.isin(material,wheel_ids)&blocks(hi[:,:,3]>0)).mean(-1)>=.5
            wheel&=decode(before['data'][view+'_down'][fi]['albedo'])[:,:,3]>0
            region=np.zeros_like(wheel)
            for pts in components(wheel):region[pts[:,0],pts[:,1]]=True
            union|=region;regions.append(region)
        regions.append(union)
        for fi,(old,new,region) in enumerate(zip(before['data'][view+'_down'],after['data'][view+'_down'],regions)):
            a=decode(old['albedo']);b=decode(new['albedo'])
            assert np.array_equal(a[:,:,3],b[:,:,3]),'Wheel silhouette changed'
            assert old['emit']==new['emit'],'Lights changed'
            assert old['pack']==new['pack'],'Depth or specular changed'
            assert np.array_equal(decode(old['pack2'])[:,:,1:],decode(new['pack2'])[:,:,1:]),'Far depth changed'
            for kind in ('albedo','normal','pack2'):
                assert np.array_equal(decode(old[kind])[~region],decode(new[kind])[~region]),'Body pixels changed'
            if fi<8:
                gaps=(a[:,:,:3].max(-1)<=90)&region
                assert np.array_equal(a[gaps],b[gaps]),'Dark spoke gaps changed'
                changed=np.any(a!=b,axis=-1)
                assert changed.sum()<region.sum()*.35,'Too much wheel detail changed'
                assert np.abs(a[:,:,:3].astype(float)-b[:,:,:3]).max()<=96,'Edge adjustment too strong'
                total+=int(changed.sum())
            n=decode(new['normal']).astype(float);q=decode(new['pack2']).astype(float)
            nx=(n[:,:,0]*256+n[:,:,1])/65535*2-1;ny=(n[:,:,2]*256+q[:,:,0])/65535*2-1
            assert np.max((nx*nx+ny*ny)[b[:,:,3]>0])<1.001,'Invalid normals'
        # Magnify the wheel pixels, with exactly the same crop on both sides.
        fi=0;a=decode(before['data'][view+'_down'][fi]['albedo']);region=regions[fi]
        pts=max(list(components(region)),key=len);ys,xs=pts[:,0],pts[:,1]
        box=(int(xs.min())-2,int(ys.min())-2,int(xs.max())+3,int(ys.max())+3)
        row=ci*2+vi
        for col,data in enumerate((before,after)):
            im=Image.fromarray(decode(data['data'][view+'_down'][0]['albedo'])).crop(box)
            im.thumbnail((144,144));im=im.resize((im.width*8,im.height*8),Image.Resampling.NEAREST)
            sheet.paste(im,(col*384+100,row*192+34),im)
            draw.text((col*384+12,row*192+8),f'{car.upper()} {view} / wheels '+('OFF' if col==0 else 'ON'),fill=(235,235,235))
    assert total>0,'No visible wheel refinement'
    print('PASS',car,total,'edge pixels refined across 16 spin frames; body, dark gaps, lights, alpha and depth preserved')
sheet.save(ROOT/'output/wheel-comparison.png')
