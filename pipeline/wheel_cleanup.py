"""Restrained rim edge tones and material-restricted tyre normal smoothing."""
import base64,io,json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'.runtime'))
import numpy as np
from PIL import Image,ImageDraw

def decode(url): return np.array(Image.open(io.BytesIO(base64.b64decode(url.split(',')[1]))))
def encode(a):
    b=io.BytesIO();Image.fromarray(a).save(b,'PNG',optimize=True)
    return 'data:image/png;base64,'+base64.b64encode(b.getvalue()).decode()
def blocks(a):
    return np.moveaxis(a.reshape(128,4,128,4,*a.shape[2:]),1,2).reshape(128,128,16,*a.shape[2:])
def neighbour(a,dy,dx):
    p=np.pad(a,((1,1),(1,1))+((0,0),)*(a.ndim-2),mode='edge')
    return p[1+dy:129+dy,1+dx:129+dx]
def components(mask):
    seen=np.zeros_like(mask)
    for y,x in zip(*np.where(mask)):
        if seen[y,x]: continue
        todo=[(y,x)];seen[y,x]=True;points=[]
        while todo:
            cy,cx=todo.pop();points.append((cy,cx))
            for dy,dx in ((-1,0),(1,0),(0,-1),(0,1)):
                yy,xx=cy+dy,cx+dx
                if 0<=yy<128 and 0<=xx<128 and mask[yy,xx] and not seen[yy,xx]:
                    seen[yy,xx]=True;todo.append((yy,xx))
        if len(points)>=10: yield np.array(points)

def build_wheels(key):
    folder=ROOT/'output'/key
    source=json.loads((folder/'assets.improved.json').read_text())
    result=json.loads(json.dumps(source))
    ids=json.loads((folder/'material_ids.json').read_text())
    wheel_ids=[v for k,v in ids.items() if any(s in k.lower() for s in ('wheel','rim','tire'))]
    hub_ids=[v for k,v in ids.items() if any(s in k.lower() for s in ('bolt','rimbadge'))]
    rim_ids=[v for k,v in ids.items() if 'rim' in k.lower() or 'bolt' in k.lower()]
    stats={}
    for view in ('front','back'):
        frames=[];masks=[];records=[]
        for fi in range(8):
            entry=source['data'][view+'_down'][fi]
            a=decode(entry['albedo']);n=decode(entry['normal']);q=decode(entry['pack2']);p=decode(entry['pack'])
            hi=np.array(Image.open(folder/f'{view}_f{fi}_albedo.png'))
            material=blocks(np.array(Image.open(folder/f'{view}_f{fi}_material.png')))
            samples=blocks(hi[:,:,:3]).astype(float)
            covered=blocks(hi[:,:,3]>0)
            wheel_samples=np.isin(material,wheel_ids)&covered
            fraction=wheel_samples.mean(-1)
            wheel=(fraction>=.5)&(a[:,:,3]>0)
            rho=np.zeros((128,128),float)
            region=np.zeros_like(wheel)
            # Fit only a coordinate frame, never a replacement circular shape.
            # This keeps the model's tyre profile and original sprite mask.
            for pts in components(wheel):
                coords=pts[:,::-1].astype(float)
                centre=coords.mean(0);cov=np.cov(coords.T)+np.eye(2)*.15
                delta=coords-centre
                radius=.5*np.sqrt(np.einsum('ni,ij,nj->n',delta,np.linalg.inv(cov),delta))
                rho[pts[:,0],pts[:,1]]=radius;region[pts[:,0],pts[:,1]]=True
            hub=region&(rho<.42)
            if hub_ids: hub|=(np.isin(material,hub_ids)&covered).sum(-1)>=2
            # Preserve each spoke's opaque bright core and dark gap. Only the
            # outer bright boundary receives a small number of edge tones.
            denom=np.maximum(wheel_samples.sum(-1),1)
            average=(samples*wheel_samples[:,:,:,None]).sum(2)/denom[:,:,None]
            low=np.where(wheel_samples[:,:,:,None],samples,255).min(2)
            high=np.where(wheel_samples[:,:,:,None],samples,0).max(2)
            mixed=(high-low).max(-1)>32
            edge=region&~hub&(rho>.42)&mixed&(a[:,:,:3].max(-1)>90)
            # Quantize only the adjustment, keeping solid rims and their palette.
            delta=np.round((average-a[:,:,:3])*.35/8)*8
            new_colour=np.clip(a[:,:,:3].astype(float)+delta,0,255).astype(np.uint8)
            cleaned=a.copy();cleaned[:,:,:3][edge]=new_colour[edge]
            nx=(n[:,:,0].astype(float)*256+n[:,:,1])/65535*2-1
            ny=(n[:,:,2].astype(float)*256+q[:,:,0])/65535*2-1
            normal=np.stack((nx,ny,np.sqrt(np.maximum(0,1-nx*nx-ny*ny))),-1)
            depth=(p[:,:,0].astype(float)*256+p[:,:,1])/65535*5.2
            rim=(np.isin(material,rim_ids)&covered).sum(-1)>=4
            accum=normal*8;weight=np.full((128,128),8.)
            for dy,dx in ((-1,0),(1,0),(0,-1),(0,1),(-1,-1),(1,1),(-1,1),(1,-1)):
                other=neighbour(normal,dy,dx);dot=(normal*other).sum(-1)
                valid=region&neighbour(region,dy,dx)&(rim==neighbour(rim,dy,dx))&(dot>.93)&(np.abs(depth-neighbour(depth,dy,dx))<.12)
                w=valid*(1 if dy and dx else 2)
                accum+=other*w[:,:,None];weight+=w
            filtered=accum/weight[:,:,None]
            filtered=.7*normal+.3*filtered
            filtered/=np.maximum(np.linalg.norm(filtered,axis=-1,keepdims=True),1e-8)
            smooth=region&~hub
            xx=np.clip(np.round((filtered[:,:,0]+1)*.5*65535),0,65535).astype(np.uint16)
            yy=np.clip(np.round((filtered[:,:,1]+1)*.5*65535),0,65535).astype(np.uint16)
            newn=n.copy();newq=q.copy()
            newn[:,:,0][smooth]=(xx>>8)[smooth];newn[:,:,1][smooth]=(xx&255)[smooth]
            newn[:,:,2][smooth]=(yy>>8)[smooth];newq[:,:,0][smooth]=(yy&255)[smooth]
            updated=dict(entry,albedo=encode(cleaned),normal=encode(newn),pack2=encode(newq))
            frames.append(updated);masks.append(region)
            changed=np.any(cleaned!=a,axis=-1)
            assert not np.any(changed&~region),'Wheel colour changes escaped wheel region'
            assert np.array_equal(cleaned[hub],a[hub]),'Hub detail changed'
            records.append({'colour_pixels':int(changed.sum()),'normal_pixels':int(np.any(n!=newn,axis=-1).sum()),'wheel_pixels':int(region.sum())})
        # Rebuild the speed blur from the cleaned spin frames, preserving alpha.
        original=source['data'][view+'_down'][8]
        blur=dict(original)
        mask=np.any(masks,axis=0)
        for kind in ('albedo',):
            old=decode(original[kind]);imgs=np.stack([decode(f[kind]) for f in frames])
            weights=imgs[:,:,:,3]>0;total=np.maximum(weights.sum(0),1)
            average=(imgs[:,:,:,:3]*weights[:,:,:,None]).sum(0)/total[:,:,None]
            old[:,:,:3][mask]=np.clip(average[mask]+.5,0,255)
            blur[kind]=encode(old)
        # Vector averaging avoids averaging packed byte values across byte carries.
        ns=[]
        for f in frames:
            nn=decode(f['normal']).astype(float);qq=decode(f['pack2']).astype(float)
            ns.append(np.stack(((nn[:,:,0]*256+nn[:,:,1])/65535*2-1,(nn[:,:,2]*256+qq[:,:,0])/65535*2-1),-1))
        coverage=np.stack([decode(f['albedo'])[:,:,3]>0 for f in frames])
        mean=(np.stack(ns)*coverage[:,:,:,None]).sum(0)/np.maximum(coverage.sum(0),1)[:,:,None]
        length=np.linalg.norm(mean,axis=-1,keepdims=True);mean/=np.maximum(length,1.)
        xx=np.clip(np.round((mean[:,:,0]+1)*.5*65535),0,65535).astype(np.uint16)
        yy=np.clip(np.round((mean[:,:,1]+1)*.5*65535),0,65535).astype(np.uint16)
        nn=decode(original['normal']);qq=decode(original['pack2'])
        nn[:,:,0][mask]=(xx>>8)[mask];nn[:,:,1][mask]=(xx&255)[mask];nn[:,:,2][mask]=(yy>>8)[mask];qq[:,:,0][mask]=(yy&255)[mask]
        blur.update(normal=encode(nn),pack2=encode(qq));frames.append(blur)
        result['data'][view+'_down']=frames;result['alt'][view+'_down']=[f['albedo'] for f in frames]
        stats[view]=records
    result['meta']['wheel_cleanup']='35% quantized rim-edge tone correction; 30% material-restricted normal filter; hubs, gaps, alpha preserved'
    destination=folder/'sprites-wheels';destination.mkdir(exist_ok=True)
    for state,frames in result['data'].items():
        for fi,f in enumerate(frames):
            for kind,url in f.items(): (destination/f'{state}_f{fi}_{kind}.png').write_bytes(base64.b64decode(url.split(',')[1]))
    (folder/'assets.wheels.json').write_text(json.dumps(result,separators=(',',':')))
    (folder/'wheel-cleanup-stats.json').write_text(json.dumps(stats,indent=2))
    print('WHEEL CLEANUP',key,stats,flush=True)

if __name__=='__main__':
    for key in sys.argv[1:] or ('silvia','civic'): build_wheels(key)
