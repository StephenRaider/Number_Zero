"""Pack detached body / complete-wheel layers, paint masks and 16-bit depth."""
import sys,json,base64
from pathlib import Path
import sys as _path_sys; _path_sys.path.insert(0,str(Path(__file__).resolve().parent))
from settings import WORKSPACE as ROOT, CODE, GAME, SOURCES
sys.path.append(str(ROOT/'.runtime'))
import numpy as np
from PIL import Image

helpers=(CODE/'pack_helpers.py').read_text()
helpers=helpers.replace('(blocks(em[...,c])>0.5)','blocks(em[...,c])')
exec(compile(helpers,'pack_helpers','exec'))

def encode(x):
    a=x['a'];alpha=a.astype(np.uint8)*255
    def rgba(rgb):
        out=np.dstack((rgb,alpha)).astype(np.uint8);out[~a]=0;return b64(out)
    nx=enc16((x['nx']+1)/2);ny=enc16((x['ny']+1)/2);d=enc16(x['d']);f=enc16(x['f'])
    return {'albedo':rgba(np.clip(x['alb']+.5,0,255)),
        'normal':rgba(np.stack((nx>>8,nx&255,ny>>8),-1)),
        'pack':rgba(np.stack((d>>8,d&255,np.clip(x['sp']*255,0,255)),-1)),
        'pack2':rgba(np.stack((ny&255,f>>8,f&255),-1)),
        'emit':rgba(np.clip(x['em']*255,0,255))}

for supplied in sys.argv[1:] or ['silvia','civic']:
    key=supplied.split('-')[0]
    folder=ROOT/'output'/(supplied if '-modular' in supplied else key+'-modular')
    ids=json.loads((folder/'material_ids.json').read_text());names={v:k for k,v in ids.items()}
    def rd(v,s,f,k):
        image=np.array(Image.open(folder/f'{v}_f{f}_{k}.png'))
        return image[...,0] if k=='material' and image.ndim==3 else image
    data={}
    for view in ['front','back']:
        for pose in ['left','straight','right']:
            entry={}
            for layer in ['body','wheels']:
                label=f'{view}_{pose}_{layer}';x=frame(label,'',0)
                material=rd(label,'',0,'material');covered=blocks(rd(label,'',0,'albedo')[...,3]>0)
                mb=blocks(material);mid=np.zeros((128,128),np.uint8)
                for y in range(128):
                    for xx in range(128):
                        vals=mb[y,xx][covered[y,xx]]
                        if len(vals):mid[y,xx]=np.bincount(vals).argmax()
                # Repair a one-pixel paint/bonnet notch only when the target panel
                # already covers subpixels here. Never expand paint over lamps/trim.
                paint_ids={v for k,v in ids.items() if k in ['body_paint','body_paint2','hM_CarPaint_Max1','Custom bonnet paint']}
                for _ in range(2):
                    fixed=mid.copy()
                    for y in range(1,127):
                        for xx in range(1,127):
                            current=mid[y,xx]
                            if current not in paint_ids:continue
                            region=mid[y-1:y+2,xx-1:xx+2].ravel();counts=np.bincount(region,minlength=256);counts[current]-=1;target=counts.argmax()
                            if target in paint_ids and target!=current and counts[target]>=6 and counts[current]<=1 and np.count_nonzero(mb[y,xx]==target)>=3:fixed[y,xx]=target
                    mid=fixed
                # Choose colour from exactly the winning material samples. Independent
                # colour/material votes left cream islands on black paint and hood edges.
                same=covered&(mb==mid[...,None])
                x['alb']=mode_colour(blocks(rd(label,'',0,'albedo')[...,:3]),same).astype(float)
                # Alpha-edge samples can straddle several materials. If the chosen
                # visible colour is exactly stock paint, require actual paint
                # coverage in this block before correcting its categorical vote.
                original=np.array([255,234,186] if key=='silvia' else [219,229,237])
                near_paint=(np.abs(x['alb']-original).max(-1)<2)&~np.isin(mid,list(paint_ids))
                for y,xx in np.argwhere(near_paint):
                    counts={idx:np.count_nonzero(mb[y,xx]==idx) for idx in paint_ids};target=max(counts,key=counts.get)
                    if counts[target]>=3:mid[y,xx]=target
                # Smooth normals within a material and similar surface direction only.
                n=np.stack((x['nx'],x['ny'],np.sqrt(np.maximum(0,1-x['nx']**2-x['ny']**2))),-1)
                for _ in range(2):
                    total=n.copy();weight=np.ones((128,128))
                    for dy,dx in [(-1,0),(1,0),(0,-1),(0,1)]:
                        nn=np.roll(n,(dy,dx),(0,1));mm=np.roll(mid,(dy,dx),(0,1));dd=np.roll(x['d'],(dy,dx),(0,1))
                        ok=(mid==mm)&x['a']&(np.sum(n*nn,-1)>.94)&(abs(x['d']-dd)*5.2<.12)
                        if dy:ok[[0,-1],:]=False
                        if dx:ok[:,[0,-1]]=False
                        total+=nn*ok[...,None]*.22;weight+=ok*.22
                    n=total/weight[...,None];n/=np.maximum(np.linalg.norm(n,axis=-1,keepdims=True),1e-6)
                x['nx']=n[...,0];x['ny']=n[...,1]
                # Same-material supersampling only at rim edges; never flood spoke gaps.
                if layer=='wheels':
                    hi=blocks(rd(label,'',0,'albedo')[...,:3]);avg=(hi*covered[...,None]).sum(2)/np.maximum(covered.sum(2)[...,None],1)
                    rim=np.isin(mid,[v for k,v in ids.items() if 'rim' in k.lower()]);mix=(hi.max(2)-hi.min(2)).max(-1)>24
                    ok=rim&mix&(x['alb'].max(-1)>90)
                    x['alb'][ok]=.65*x['alb'][ok]+.35*(np.round(avg[ok]/8)*8)
                em=x['em'];winner=np.eye(3)[em.argmax(-1)];x['em']=winner*em.max(-1)[...,None];x['em'][...,0]*=.45
                entry[layer]=encode(x)
                mask=np.zeros((128,128,4),np.uint8);mask[...,3]=x['a']*255
                for name,idx in ids.items():
                    channel=2 if name=='Custom bonnet paint' else 1 if name=='body_paint2' else 0 if name in ['body_paint','hM_CarPaint_Max1'] else None
                    if channel is not None:mask[...,channel][mid==idx]=255
                # At a supersampled boundary, modal colour and modal material can
                # disagree. Preserve the visible glass/plate/lamp pixel in that case.
                visible=np.clip(x['alb']+.5,0,255).astype(int)
                # Protection follows material identity, never brightness: white paint
                # is still paint and must recolour completely.
                entry[layer]['mask']=b64(mask)
                if layer=='body':
                    masks=folder/'masks';masks.mkdir(exist_ok=True)
                    for channel,name in enumerate(['paint-primary','paint-secondary','hood']):
                        Image.fromarray(mask[...,channel]).save(masks/f'{view}_{pose}_{name}.png')
            data[f'{view}_{pose}']=entry
    animation={}
    for view in ['front','back']:
        for pose in ['left','straight','right']:
            label=f'{view}_{pose}';rawlabel=label+'_wheels'
            frames=[data[label]['wheels']];rawframes=[frame(rawlabel,'',0)]
            for fi in range(1,8):
                if not (folder/f'{rawlabel}_f{fi}_albedo.png').exists():break
                x=frame(rawlabel,'',fi);rawframes.append(x);packed=encode(x)
                mask=np.zeros((128,128,4),np.uint8);mask[...,3]=x['a']*255;packed['mask']=b64(mask);frames.append(packed)
            if len(frames)==8:
                weights=np.maximum(sum(x['a'] for x in rawframes),1)
                def mean(k):
                    sample=rawframes[0][k];w=weights[...,None] if sample.ndim==3 else weights
                    return sum(x[k]*(x['a'][...,None] if sample.ndim==3 else x['a']) for x in rawframes)/w
                blur={k:mean(k) for k in ['alb','nx','ny','d','f','sp','em']};blur['a']=np.any([x['a'] for x in rawframes],axis=0)
                blur['nx']=np.clip(blur['nx'],-.999,.999);blur['ny']=np.clip(blur['ny'],-.999,.999)
                packed=encode(blur);mask=np.zeros((128,128,4),np.uint8);mask[...,3]=blur['a']*255;packed['mask']=b64(mask);frames.append(packed)
            animation[label]=frames
    assets={'data':data,'animation':animation,'meta':json.loads((folder/'meta.json').read_text()),'wheels':json.loads((folder/'wheels.json').read_text())}
    (folder/'assets.json').write_text(json.dumps(assets,separators=(',',':')))
    sprites=folder/'sprites';sprites.mkdir(exist_ok=True)
    for label,entry in data.items():
        for layer,maps in entry.items():
            for kind,url in maps.items():(sprites/f'{label}_{layer}_{kind}.png').write_bytes(base64.b64decode(url.split(',')[1]))
    for label,frames in animation.items():
        for fi,maps in enumerate(frames):
            for kind,url in maps.items():(sprites/f'{label}_wheels_f{fi}_{kind}.png').write_bytes(base64.b64decode(url.split(',')[1]))
    print('PACKED MODULAR',key,flush=True)
