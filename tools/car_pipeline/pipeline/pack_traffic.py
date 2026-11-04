"""Material-aware supersample resolve and selective traffic runtime export."""
from pathlib import Path
import json,hashlib,sys
import numpy as np
from PIL import Image
sys.path.insert(0,str(Path(__file__).resolve().parent))
from traffic_models import MODELS
import sys as _path_sys; _path_sys.path.insert(0,str(Path(__file__).resolve().parent))
from settings import WORKSPACE as ROOT, CODE, GAME, SOURCES;EXPORT=ROOT/'output/traffic-runtime';N=256;SS=3
def blocks(a):return np.moveaxis(a.reshape(N,SS,N,SS,*a.shape[2:]),1,2).reshape(N,N,SS*SS,*a.shape[2:])
catalog=[];sheet=Image.new('RGB',(5*384,384),'#252c36')
for col,(key,name,length,width) in enumerate(MODELS):
 src=ROOT/'output/traffic'/key;dest=EXPORT/key;dest.mkdir(parents=True,exist_ok=True);meta=json.loads((src/'meta.json').read_text());hashes={}
 for pose in ['left','straight','right']:
  for frame in range(4):
   prefix=f'{pose}_f{frame}'
   def read(k):return np.array(Image.open(src/f'{prefix}_{k}.png'))
   alb=read('albedo');coverage=blocks(alb[...,3]>0);alpha=coverage.mean(2)>=.5
   finish=blocks(read('finish')[...,0]);winner=np.zeros((N,N),np.uint8)
   for value in [0,128,255]:
    score=((finish==value)&coverage).sum(2)
    if value==0:best=score
    else:
     take=score>best;winner[take]=value;best=np.maximum(best,score)
   chosen=coverage&(finish==winner[...,None]);den=np.maximum(chosen.sum(2),1)
   high=blocks(alb[...,:3]).astype(float);colour=np.round((high*chosen[...,None]).sum(2)/den[...,None]).astype(np.uint8)
   # Quantise gently inside materials; never average across glass, lamps or paint.
   colour=(np.round(colour.astype(float)/4)*4).clip(0,255).astype(np.uint8)
   def avg(a):return (blocks(a)*chosen).sum(2)/den
   nx=avg(read('nx')/65535*2-1);ny=avg(read('ny')/65535*2-1);nz=np.sqrt(np.maximum(0,1-nx*nx-ny*ny));normal=np.stack((nx,ny,nz),-1);normal/=np.maximum(np.linalg.norm(normal,axis=-1,keepdims=True),1e-8)
   depth=avg(read('depth')/65535);far=avg(read('far')[:,::-1]/65535);far=np.minimum(depth,far)
   for _ in range(2):
    total=normal*4;weight=np.full((N,N),4.)
    for dy,dx in [(-1,0),(1,0),(0,-1),(0,1)]:
     other=np.roll(normal,(dy,dx),(0,1));same=winner==np.roll(winner,(dy,dx),(0,1));dd=np.abs(depth-np.roll(depth,(dy,dx),(0,1)))*12
     ok=same&alpha&np.roll(alpha,(dy,dx),(0,1))&((normal*other).sum(-1)>.94)&(dd<.12);ok[[0,-1],:]=False;ok[:,[0,-1]]=False
     total+=other*ok[...,None];weight+=ok
    normal=total/weight[...,None];normal/=np.maximum(np.linalg.norm(normal,axis=-1,keepdims=True),1e-8)
   # One outward side tag per pixel; all maps are mirrored into the NE driving view.
   emhi=blocks(read('emit')[...,:3]).astype(float)/255;em=(emhi*chosen[...,None]).sum(2)/den[...,None];em=np.eye(3)[em.argmax(-1)]*em.max(-1)[...,None];em[...,0]*=.4
   yy,xx=np.mgrid[:N,:N];cx=(xx+.5-128)/26;cy=(170-yy-.5)/26;cz=(depth-.5)*12;localx=-(2**-.5*cx+2**-.5*.5*cy-2**-.5*(3**.5/2)*cz)
   def u16(a):return np.round(np.clip(a,0,1)*65535).astype(np.uint16)
   xn=u16((-normal[...,0][:,::-1]+1)/2);yn=u16((normal[...,1][:,::-1]+1)/2);d=u16(depth[:,::-1]);f=u16(far[:,::-1]);a=alpha[:,::-1].astype(np.uint8)*255
   maps={
    'albedo':np.dstack((colour[:,::-1],a)),
    'normal':np.dstack((xn>>8,xn&255,yn>>8,a)).astype(np.uint8),
    'pack':np.dstack((d>>8,d&255,np.full((N,N),166),winner[:,::-1])).astype(np.uint8),
    'pack2':np.dstack((yn&255,f>>8,f&255,a)).astype(np.uint8),
    'emit':np.dstack((np.clip(em[:,::-1]*255,0,255),(localx[:,::-1]>0)*255)).astype(np.uint8),
   }
   for kind,data in maps.items():
    data[~alpha[:,::-1]]=0;p=dest/f'{prefix}_{kind}.png';Image.fromarray(data).save(p,optimize=True);hashes[p.name]=hashlib.sha256(p.read_bytes()).hexdigest()
   if pose=='straight' and frame==0:
    im=Image.fromarray(maps['albedo']);bg=Image.new('RGBA',im.size,'#252c36');bg.alpha_composite(im);sheet.paste(bg.resize((384,384),Image.Resampling.NEAREST).convert('RGB'),(col*384,0))
   box=Image.fromarray(maps['albedo']).getchannel('A').getbbox();assert box and box[0]>0 and box[1]>0 and box[2]<N and box[3]<N,(key,box)
 meta['files']=hashes;(dest/'manifest.json').write_text(json.dumps(meta,indent=2)+'\n',newline='\n');catalog.append({'id':key,'manifest':f'{key}/manifest.json'})
 print('PACKED TRAFFIC',key,len(hashes),'native maps',flush=True)
(EXPORT/'catalog.json').write_text(json.dumps({'cars':catalog},indent=2)+'\n',newline='\n');sheet.save(ROOT/'output/traffic/preview.png')
print('All five traffic models exported with native masks, lamp channels, four wheel phases and three steering views.')
