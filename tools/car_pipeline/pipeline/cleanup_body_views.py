"""Material/depth-guided cleanup for every neutral and suspension body view.

Preserves silhouettes, paint masks, emission masks, plates and glazing exactly.
Keeps original packed exports as inputs, so rerunning does not compound filters.
"""
from pathlib import Path
import sys,json,hashlib,argparse
import sys as _path_sys; _path_sys.path.insert(0,str(Path(__file__).resolve().parent))
from settings import WORKSPACE as ROOT, CODE, GAME, SOURCES;sys.path.append(str(ROOT/'.runtime'))
import numpy as np
from PIL import Image
GAME=GAME

def blocks(a):
    return np.moveaxis(a.reshape(128,4,128,4,*a.shape[2:]),1,2).reshape(128,128,16,*a.shape[2:])
def neighbour(a,dy,dx):
    a=np.pad(a,((1,1),(1,1))+((0,0),)*(a.ndim-2),mode='edge');return a[1+dy:129+dy,1+dx:129+dx]

parser=argparse.ArgumentParser();parser.add_argument('--car',choices=['silvia','civic','testarossa','supra']);args=parser.parse_args()
pairs=[('silvia','nissan_silvia_s13'),('civic','honda_civic_ek9')] if not args.car else [(args.car,{'silvia':'nissan_silvia_s13','civic':'honda_civic_ek9','testarossa':'ferrari_testarossa','supra':'toyota_supra_a80_sz'}[args.car])]
stats={}
for key,car in pairs:
    target=GAME/'assets/cars'/car;manifest=json.loads((target/'manifest.json').read_text());record=[]
    for sample in ['zero','m2','m1','p1','p2']:
        folder=ROOT/'output'/(key+'-modular'+('' if sample=='zero' else '-pitch_'+sample))
        ids=json.loads((folder/'material_ids.json').read_text())
        paint={0:ids.get('body_paint',ids.get('hM_CarPaint_Max1')),1:ids.get('body_paint2'),2:ids.get('Custom bonnet paint')}
        for view in ['front','back']:
            for pose in ['left','straight','right']:
                label=f'{view}_{pose}_body'
                maps={kind:np.array(Image.open(folder/'sprites'/f'{label}_{kind}.png')) for kind in ['albedo','normal','pack','pack2','emit','mask']}
                old={k:v.copy() for k,v in maps.items()};a=maps['albedo'][...,3]>127
                hi=np.array(Image.open(folder/f'{label}_f0_albedo.png'));covered=blocks(hi[...,3]>0)
                material=np.array(Image.open(folder/f'{label}_f0_material.png'))[...,0]
                materials=blocks(material);counts=np.stack([((materials==i)&covered).sum(-1) for i in range(max(ids.values())+1)],-1)
                mid=counts.argmax(-1)
                for channel,idx in paint.items():
                    if idx is not None:mid[maps['mask'][...,channel]>127]=idx
                chosen=covered&(materials==mid[...,None]);weight=chosen.astype(float);denom=np.maximum(weight.sum(-1),1)
                def average(raw):return (blocks(raw)*weight).sum(-1)/denom
                def read(kind):return np.array(Image.open(folder/f'{label}_f0_{kind}.png')).astype(float)/65535
                n=np.stack((average(read('nx')*2-1),average(read('ny')*2-1)),axis=-1)
                n=np.dstack((n,np.sqrt(np.maximum(0,1-(n*n).sum(-1)))));n/=np.maximum(np.linalg.norm(n,axis=-1,keepdims=True),1e-8)
                depth=average(read('depth'))
                # Paint surfaces only: never smooth lamp, glass, badge, trim or grille details.
                surface=(maps['mask'][...,:3].max(-1)>127)&a
                normal=n.copy()
                for iteration in range(2):
                    accum=normal*8;total=np.full((128,128),8.)
                    for dy,dx in [(-1,0),(1,0),(0,-1),(0,1),(-1,-1),(1,1),(-1,1),(1,-1)]:
                        other=neighbour(normal,dy,dx);dot=(normal*other).sum(-1);dd=np.abs(neighbour(depth,dy,dx)-depth)*5.2
                        valid=surface&neighbour(surface,dy,dx)&(mid==neighbour(mid,dy,dx))&(dot>.90)&(dd<.16)
                        w=valid*np.exp(-dd*dd/.008)*np.clip((dot-.90)/.10,0,1)*(1 if dy and dx else 2)
                        accum+=other*w[...,None];total+=w
                    normal=accum/total[...,None];normal/=np.maximum(np.linalg.norm(normal,axis=-1,keepdims=True),1e-8)
                normal=.55*n+.45*normal;normal/=np.maximum(np.linalg.norm(normal,axis=-1,keepdims=True),1e-8)
                # Remove rare low-contrast paint speckles only with overwhelming same-panel support.
                alb=maps['albedo'][...,:3].copy();colour_changes=0
                for y,x in np.argwhere(surface):
                    if y in (0,127) or x in (0,127):continue
                    valid=surface[y-1:y+2,x-1:x+2]&(mid[y-1:y+2,x-1:x+2]==mid[y,x]);valid=valid.copy();valid[1,1]=False
                    values=alb[y-1:y+2,x-1:x+2][valid]
                    if len(values)<6:continue
                    colours,frequency=np.unique(values,axis=0,return_counts=True);winner=frequency.argmax();current=alb[y,x]
                    own=np.count_nonzero(np.all(values==current,axis=-1));difference=np.abs(colours[winner].astype(int)-current).sum()
                    if own<=1 and frequency[winner]>=5 and 0<difference<=42:maps['albedo'][y,x,:3]=colours[winner];colour_changes+=1
                for axis in (0,1):
                    encoded=np.clip(np.round((normal[...,axis]+1)*.5*65535),0,65535).astype(np.uint16)
                    if axis==0:maps['normal'][...,0][surface]=(encoded>>8)[surface];maps['normal'][...,1][surface]=(encoded&255)[surface]
                    else:maps['normal'][...,2][surface]=(encoded>>8)[surface];maps['pack2'][...,0][surface]=(encoded&255)[surface]
                front=np.clip(np.round(depth*65535),0,65535).astype(np.uint16)
                old_far=maps['pack2'][...,1].astype(np.uint16)*256+maps['pack2'][...,2];far=np.minimum(old_far,front)
                maps['pack'][...,0][surface]=(front>>8)[surface];maps['pack'][...,1][surface]=(front&255)[surface]
                maps['pack2'][...,1][surface]=(far>>8)[surface];maps['pack2'][...,2][surface]=(far&255)[surface]
                assert np.array_equal(maps['mask'],old['mask']) and np.array_equal(maps['emit'],old['emit'])
                assert all(np.array_equal(m[~surface],old[k][~surface]) for k,m in maps.items())
                assert np.array_equal(maps['albedo'][...,3],old['albedo'][...,3])
                out=folder/'sprites-surface-clean';out.mkdir(exist_ok=True)
                prefix=label if sample=='zero' else label+'_pitch_'+sample
                for kind in ['albedo','normal','pack','pack2']:
                    export=out/f'{label}_{kind}.png';Image.fromarray(maps[kind]).save(export,optimize=True)
                    rel=f'sprites/{prefix}_{kind}.png';dst=target/rel
                    if dst.exists():assert hashlib.sha256(dst.read_bytes()).hexdigest()==manifest['files'][rel],'Modified runtime map'
                    dst.write_bytes(export.read_bytes());manifest['files'][rel]=hashlib.sha256(dst.read_bytes()).hexdigest()
                record.append({'view':label,'pitch':sample,'colour_pixels':colour_changes,'surface_pixels':int(surface.sum())})
    manifest['cleanup']='Dominant-material depth/normals, crease-aware surface filtering, supported paint-speckle cleanup; masks/emission/silhouettes unchanged.'
    (target/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');stats[key]=record
    print('CLEANED',key,len(record),'body views;',sum(r['colour_pixels'] for r in record),'supported colour speckles; non-paint details preserved',flush=True)
(ROOT/'output/body-surface-cleanup-stats.json').write_text(json.dumps(stats,indent=2))
