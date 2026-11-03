"""Material-guided pixel clustering and surface-preserving normal filtering."""
import base64, io, json
from pathlib import Path
import numpy as np
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]

def decode(url):
    return np.array(Image.open(io.BytesIO(base64.b64decode(url.split(',')[1]))))

def neighbour(a,dy,dx):
    p=np.pad(a,((1,1),(1,1))+((0,0),)*(a.ndim-2),mode='edge')
    return p[1+dy:129+dy,1+dx:129+dx]

def build_improved(key,baseline,helpers):
    out=ROOT/'output'/key
    if not (out/'front_f0_material.png').exists():
        return
    ids=json.loads((out/'material_ids.json').read_text())
    protected_ids=[i for n,i in ids.items() if any(s in n.lower() for s in ('plate','wheel','rim','tire','brake','badge','logo','light','blinker')) or n=='Material.004']
    block=helpers['blocks']; b64=helpers['b64']; enc16=helpers['enc16']
    result={'data':{},'alt':{},'meta':dict(baseline['meta'])}
    result['meta']['cleanup']='material-guided clusters, dominant-surface maps, bilateral normals'
    stats={}
    for view in ('front','back'):
        frames=[]; changed=[]
        for fi in range(8):
            raw=helpers['frame'](view,'down',fi)
            old=baseline['data'][view+'_down'][fi]
            oldalb=decode(old['albedo']); oldn=decode(old['normal']); oldq=decode(old['pack2']); oldp=decode(old['pack'])
            hi=helpers['rd'](view,'down',fi,'albedo')
            covered=block(hi[:,:,3]>0)
            labels=block(helpers['rd'](view,'down',fi,'material')).astype(int)
            label=np.zeros((128,128),dtype=np.int32)
            counts=np.zeros((128,128,len(ids)+1),dtype=np.uint8)
            for i in ids.values(): counts[:,:,i]=((labels==i)&covered).sum(-1)
            label=counts.argmax(-1)
            protected=np.isin(label,protected_ids)|(raw['em'].max(-1)>.05)
            # Correct only tiny ambiguous notches: a replacement material must
            # already occupy at least 4 of the 16 high-resolution samples.
            votes=np.zeros_like(counts)
            for dy in (-1,0,1):
                for dx in (-1,0,1):
                    if not (dy or dx): continue
                    other=neighbour(label,dy,dx)
                    for i in ids.values(): votes[:,:,i]+=(other==i)
            target=votes.argmax(-1)
            own=np.take_along_axis(votes,label[:,:,None],axis=2)[:,:,0]
            support=np.take_along_axis(votes,target[:,:,None],axis=2)[:,:,0]
            coverage=np.take_along_axis(counts,target[:,:,None],axis=2)[:,:,0]
            fix=raw['a']&~protected&(own<=2)&(support>=6)&(coverage>=4)
            label=np.where(fix,target,label)
            chosen=covered&(labels==label[:,:,None]); weight=chosen.astype(np.float32)
            denom=np.maximum(weight.sum(-1),1)
            def avg(q): return (block(q)*weight).sum(-1)/denom
            alb=helpers['mode_colour'](block(hi[:,:,:3]),chosen).astype(np.float32)
            nx=avg(helpers['rd'](view,'down',fi,'nx').astype(np.float32)/65535*2-1)
            ny=avg(helpers['rd'](view,'down',fi,'ny').astype(np.float32)/65535*2-1)
            nz=np.sqrt(np.maximum(0,1-nx*nx-ny*ny))
            normals=np.stack((nx,ny,nz),-1)
            normals/=np.maximum(np.linalg.norm(normals,axis=-1,keepdims=True),1e-6)
            depth=avg(helpers['rd'](view,'down',fi,'depth').astype(np.float32)/65535)
            spec=avg(helpers['rd'](view,'down',fi,'spec').astype(np.float32)/255)
            # Explicit material IDs stop filtering at trim, glass, lamps and
            # body boundaries; normal/depth tests retain physical creases.
            original=normals.copy()
            for iteration in range(2):
                accum=normals*8; total=np.full((128,128),8.,dtype=np.float32)
                for dy in (-1,0,1):
                    for dx in (-1,0,1):
                        if not (dy or dx): continue
                        other=neighbour(normals,dy,dx)
                        dot=(other*normals).sum(-1)
                        dd=np.abs(neighbour(depth,dy,dx)-depth)*5.2
                        valid=(neighbour(label,dy,dx)==label)&neighbour(raw['a'],dy,dx)&(dot>.90)&(dd<.16)
                        w=valid*np.exp(-dd*dd/.008)*np.clip((dot-.90)/.10,0,1)*(2 if dx==0 or dy==0 else 1)
                        accum+=other*w[:,:,None]; total+=w
                normals=accum/total[:,:,None]
                normals/=np.maximum(np.linalg.norm(normals,axis=-1,keepdims=True),1e-6)
            normals=.55*original+.45*normals
            normals/=np.maximum(np.linalg.norm(normals,axis=-1,keepdims=True),1e-6)
            # Keep crisp original palette colours. Use a lighter silhouette
            # outline to avoid the old dark band around light body panels.
            a=raw['a']; edge=a&~(neighbour(a,-1,0)&neighbour(a,1,0)&neighbour(a,0,-1)&neighbour(a,0,1))
            alb[edge&~protected]*=.80
            alb[protected]=oldalb[:,:,:3][protected]
            nxold=(oldn[:,:,0].astype(float)*256+oldn[:,:,1])/65535*2-1
            nyold=(oldn[:,:,2].astype(float)*256+oldq[:,:,0])/65535*2-1
            normals[:,:,0][protected]=nxold[protected]; normals[:,:,1][protected]=nyold[protected]
            depth[protected]=raw['d'][protected]; spec[protected]=raw['sp'][protected]
            far=np.minimum(raw['f'],depth)
            frames.append(dict(a=a,alb=alb,nx=normals[:,:,0],ny=normals[:,:,1],d=depth,f=far,sp=spec,em=raw['em']))
            changed.append(int(np.any(oldalb[:,:,:3]!=np.clip(alb+.5,0,255).astype(np.uint8),axis=-1)[a].sum()))
        masks=np.stack([f['a'] for f in frames]); total=np.maximum(masks.sum(0),1)
        def mean(name):
            values=np.stack([f[name] for f in frames]); w=masks[:,:,:,None] if values.ndim==4 else masks
            return (values*w).sum(0)/(total[:,:,None] if values.ndim==4 else total)
        blur={name:mean(name) for name in ('alb','nx','ny','d','f','sp')}
        blur.update(a=masks.any(0),em=frames[0]['em']); frames.append(blur)
        encoded=[]
        folder=out/'sprites-improved'; folder.mkdir(exist_ok=True)
        for fi,f in enumerate(frames):
            a=np.where(f['a'],255,0).astype(np.uint8)
            alb=np.dstack((np.clip(f['alb']+.5,0,255).astype(np.uint8),a)); alb[a==0]=0
            nx=enc16((f['nx']+1)/2); ny=enc16((f['ny']+1)/2); depth=enc16(f['d']); far=enc16(f['f'])
            normal=np.dstack((nx>>8,nx&255,ny>>8,a)).astype(np.uint8)
            pack=np.dstack((depth>>8,depth&255,np.clip(f['sp']*255+.5,0,255),a)).astype(np.uint8)
            pack2=np.dstack((ny&255,far>>8,far&255,a)).astype(np.uint8)
            for image in (normal,pack,pack2): image[a==0]=0
            entry={'albedo':b64(alb),'normal':b64(normal),'pack':b64(pack),'pack2':b64(pack2),'emit':baseline['data'][view+'_down'][fi]['emit']}
            for name,url in entry.items(): (folder/f'{view}_down_f{fi}_{name}.png').write_bytes(base64.b64decode(url.split(',')[1]))
            encoded.append(entry)
        result['data'][view+'_down']=encoded
        result['alt'][view+'_down']=[f['albedo'] for f in encoded]
        stats[view]={'albedo_pixels_changed_per_frame':changed}
    (out/'assets.improved.json').write_text(json.dumps(result,separators=(',',':')))
    (out/'meta.improved.json').write_text(json.dumps(result['meta'],indent=2))
    (out/'cleanup-stats.json').write_text(json.dumps(stats,indent=2))
    print('IMPROVED',key,stats,flush=True)
